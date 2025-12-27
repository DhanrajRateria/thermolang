#include "thermolang/optimizer/CircuitTopologyOptimizer.h"
#include <cmath>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

namespace thermolang::optimizer
{
    namespace
    {
        constexpr int HW_WIDTH = 8;
        constexpr int HW_HEIGHT = 8;
        constexpr int MAX_PHYSICAL_QUBITS = HW_WIDTH * HW_HEIGHT;
        constexpr double CHAIN_STRENGTH = -2.0; // Ferromagnetic link strength

        struct Coord
        {
            int x;
            int y;

            bool operator<(const Coord &other) const
            {
                return x < other.x || (x == other.x && y < other.y);
            }

            bool operator==(const Coord &other) const
            {
                return x == other.x && y == other.y;
            }
        };

        std::vector<Coord> neighbors(Coord c)
        {
            // Manhattan neighborhood on the grid
            std::vector<Coord> n;
            if (c.x > 0)
                n.push_back({c.x - 1, c.y});
            if (c.x < HW_WIDTH - 1)
                n.push_back({c.x + 1, c.y});
            if (c.y > 0)
                n.push_back({c.x, c.y - 1});
            if (c.y < HW_HEIGHT - 1)
                n.push_back({c.x, c.y + 1});
            return n;
        }
    } // namespace

    bool CircuitTopologyPass::run(ir::FunctionIR &function_ir)
    {
        bool modified = false;

        for (auto &block : function_ir.basic_blocks)
        {
            for (auto &instr : block->instructions)
            {
                if (auto *ebm = dynamic_cast<ir::DiscreteEBMInstr *>(instr.get()))
                {
                    // Only mutate discrete EBMs; other instructions are left untouched
                    modified = optimize_circuit_topology(function_ir, ebm) || modified;
                }
            }
        }

        return modified;
    }

    bool CircuitTopologyPass::optimize_circuit_topology(ir::FunctionIR &func, ir::DiscreteEBMInstr *ebm)
    {
        (void)func; // Currently unused but kept for future extensions
        const int n_logical = static_cast<int>(ebm->spins.size());
        if (n_logical > MAX_PHYSICAL_QUBITS)
        {
            std::cerr << "Error: Logical problem size (" << n_logical
                      << ") exceeds physical grid (" << MAX_PHYSICAL_QUBITS << ")" << std::endl;
            return false;
        }

        std::cout << "Running Hardware-Aware Embedding for " << n_logical << " spins..." << std::endl;

        // 1. Initial placement: row-major layout
        std::map<int, Coord> placement;
        std::vector<bool> occupied(MAX_PHYSICAL_QUBITS, false);

        for (int i = 0; i < n_logical; ++i)
        {
            placement[i] = {i % HW_WIDTH, i / HW_WIDTH};
            occupied[i] = true;
        }

        std::map<int, double> h_physical;
        std::map<std::pair<int, int>, double> J_physical;

        for (int i = 0; i < n_logical; ++i)
        {
            const int p_idx = placement[i].y * HW_WIDTH + placement[i].x;
            h_physical[p_idx] = ebm->h_vector[i];
        }

        int routing_failures = 0;
        int embeddings_added = 0;

        for (int i = 0; i < n_logical; ++i)
        {
            for (int j = i + 1; j < n_logical; ++j)
            {
                if (std::abs(ebm->J_matrix[i][j]) < 1e-9)
                    continue;

                const Coord c1 = placement[i];
                const Coord c2 = placement[j];
                int p1 = c1.y * HW_WIDTH + c1.x;
                int p2 = c2.y * HW_WIDTH + c2.x;

                const int dist = std::abs(c1.x - c2.x) + std::abs(c1.y - c2.y);

                if (dist == 1)
                {
                    if (p1 > p2)
                        std::swap(p1, p2);
                    J_physical[{p1, p2}] += ebm->J_matrix[i][j];
                }
                else
                {
                    std::cout << "  Routing " << i << " -> " << j << " (dist " << dist << ")... ";

                    std::queue<std::pair<Coord, std::vector<Coord>>> q;
                    q.push({c1, {c1}});
                    std::set<Coord> visited;
                    visited.insert(c1);

                    std::vector<Coord> path_found;

                    while (!q.empty())
                    {
                        auto [curr, path] = q.front();
                        q.pop();

                        if (std::abs(curr.x - c2.x) + std::abs(curr.y - c2.y) == 1)
                        {
                            path_found = path;
                            break;
                        }

                        for (const Coord next : neighbors(curr))
                        {
                            const int next_idx = next.y * HW_WIDTH + next.x;
                            if (!occupied[next_idx] && visited.find(next) == visited.end())
                            {
                                visited.insert(next);
                                auto new_path = path;
                                new_path.push_back(next);
                                q.push({next, new_path});
                            }
                        }
                    }

                    if (path_found.empty())
                    {
                        std::cout << "FAILED (Congestion)" << std::endl;
                        routing_failures++;
                    }
                    else
                    {
                        const double original_J = ebm->J_matrix[i][j];

                        int curr_p = p1;
                        for (size_t k = 1; k < path_found.size(); ++k)
                        {
                            const Coord link = path_found[k];
                            const int link_p = link.y * HW_WIDTH + link.x;

                            occupied[link_p] = true; // Reserve chain qubit

                            const int u = std::min(curr_p, link_p);
                            const int v = std::max(curr_p, link_p);
                            J_physical[{u, v}] += CHAIN_STRENGTH;

                            curr_p = link_p;
                        }

                        const int u = std::min(curr_p, p2);
                        const int v = std::max(curr_p, p2);
                        J_physical[{u, v}] += original_J;

                        embeddings_added += static_cast<int>(path_found.size()) - 1;
                        std::cout << "Success (Added " << path_found.size() - 1 << " chain qubits)" << std::endl;
                    }
                }
            }
        }

        if (embeddings_added == 0)
            return false;

        const int n_physical = MAX_PHYSICAL_QUBITS;
        std::vector<std::vector<double>> new_J(n_physical, std::vector<double>(n_physical, 0.0));
        std::vector<double> new_h(n_physical, 0.0);
        std::vector<ir::Operand> new_spins;
        new_spins.reserve(n_physical);

        for (int i = 0; i < n_physical; ++i)
        {
            if (i < n_logical)
            {
                new_spins.push_back(ebm->spins[i]);
            }
            else
            {
                new_spins.emplace_back("q_ancilla_" + std::to_string(i));
            }
        }

        for (const auto &[idx, val] : h_physical)
        {
            new_h[idx] = val;
        }

        for (const auto &[pair, val] : J_physical)
        {
            new_J[pair.first][pair.second] = val;
            new_J[pair.second][pair.first] = val;
        }

        ebm->J_matrix = new_J;
        ebm->h_vector = new_h;
        ebm->spins = new_spins;
        ebm->color_groups.clear();

        std::cout << "  CircuitTopology: Embedding Complete. Added " << embeddings_added
                  << " physical qubits. Grid utilization: "
                  << static_cast<float>(n_logical + embeddings_added) / MAX_PHYSICAL_QUBITS * 100.0f
                  << "%" << std::endl;

        if (routing_failures > 0)
        {
            std::cout << "  Warning: " << routing_failures << " connections could not be routed." << std::endl;
        }

        return true;
    }

    bool CircuitTopologyPass::optimize_couplings(ir::FunctionIR &)
    {
        return false;
    }

    bool CircuitTopologyPass::partition_computation(ir::FunctionIR &, int)
    {
        return false;
    }

} // namespace thermolang::optimizer