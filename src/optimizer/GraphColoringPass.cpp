#include "thermolang/optimizer/GraphColoringPass.h"
#include <iostream>
#include <algorithm>
#include <set>

namespace thermolang::optimizer
{
    bool GraphColoringPass::run(ir::FunctionIR &function_ir)
    {
        bool modified = false;
        for (auto &block : function_ir.basic_blocks)
        {
            for (auto &instr : block->instructions)
            {
                if (auto *ebm = dynamic_cast<ir::DiscreteEBMInstr *>(instr.get()))
                {
                    if (ebm->color_groups.empty())
                    {
                        std::cout << "  GraphColoringPass: Coloring graph for EBM with "
                                  << ebm->spins.size() << " spins." << std::endl;
                        ebm->color_groups = color_graph(ebm->J_matrix);
                        modified = true;
                    }
                }
            }
        }
        return modified;
    }

    std::vector<std::vector<int>> GraphColoringPass::color_graph(const std::vector<std::vector<double>> &J)
    {
        size_t n = J.size();
        std::vector<int> result_colors(n, -1);

        // 1. Calculate degrees for Welsh-Powell
        std::vector<std::pair<int, int>> degrees(n);
        for (size_t i = 0; i < n; ++i)
        {
            int deg = 0;
            for (size_t j = 0; j < n; ++j)
            {
                if (std::abs(J[i][j]) > 1e-9)
                    deg++;
            }
            degrees[i] = {deg, static_cast<int>(i)};
        }

        // Sort by degree descending
        std::sort(degrees.rbegin(), degrees.rend());

        // 2. Assign colors
        for (const auto &p : degrees)
        {
            int u = p.second;
            std::set<int> neighbor_colors;

            for (size_t v = 0; v < n; ++v)
            {
                if (std::abs(J[u][v]) > 1e-9 && result_colors[v] != -1)
                {
                    neighbor_colors.insert(result_colors[v]);
                }
            }

            int color = 0;
            while (neighbor_colors.count(color))
                color++;
            result_colors[u] = color;
        }

        // 3. Group indices by color
        int max_color = 0;
        for (int c : result_colors)
            if (c > max_color)
                max_color = c;

        std::vector<std::vector<int>> groups(max_color + 1);
        for (size_t i = 0; i < n; ++i)
        {
            groups[result_colors[i]].push_back(i);
        }
        std::cout << "  GraphColoringPass: Colored " << n << " spins into " << groups.size() << " independent sets." << std::endl;
        return groups;
    }
}