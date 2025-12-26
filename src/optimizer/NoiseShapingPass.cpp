#include "thermolang/optimizer/NoiseShapingPass.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <cstdlib>
#include <string>
#include <sstream>

namespace thermolang::optimizer
{

    namespace
    {
        struct NoiseShapingConfig
        {
            bool enabled = true;
            bool use_degree = true;
            bool use_variance = true;
            double variance_shrink_cap = 0.5; // max fractional beta shrink from variance
            std::string mode_label = "degree+variance";
        };

        NoiseShapingConfig load_config()
        {
            NoiseShapingConfig cfg;

            if (const char *env = std::getenv("NOISE_SHAPING_MODE"))
            {
                std::string mode(env);
                std::transform(mode.begin(), mode.end(), mode.begin(), [](unsigned char c)
                               { return static_cast<char>(std::tolower(c)); });

                if (mode == "off")
                {
                    cfg.enabled = false;
                    cfg.use_degree = false;
                    cfg.use_variance = false;
                    cfg.mode_label = "off";
                }
                else if (mode == "degree")
                {
                    cfg.use_variance = false;
                    cfg.mode_label = "degree";
                }
                else if (mode == "variance")
                {
                    cfg.use_degree = false;
                    cfg.mode_label = "variance";
                }
                else
                {
                    cfg.mode_label = "degree+variance";
                }
            }

            if (const char *env = std::getenv("NOISE_SHAPING_VARIANCE_SHRINK"))
            {
                try
                {
                    double v = std::stod(env);
                    cfg.variance_shrink_cap = std::clamp(v, 0.0, 1.0);
                }
                catch (...)
                {
                    // Keep default on parse failure
                }
            }

            return cfg;
        }
    } // namespace

    bool NoiseShapingPass::run(ir::FunctionIR &function_ir)
    {
        const auto config = load_config();
        if (!config.enabled)
        {
            return false;
        }

        bool modified = false;

        // Pre-scan any variance annotations emitted by VarianceTrackingPass so we can
        // modulate temperatures by uncertainty as well as degree.
        std::unordered_map<std::string, double> variance_by_reg;
        for (const auto &block : function_ir.basic_blocks)
        {
            for (const auto &instr : block->instructions)
            {
                if (auto *track = dynamic_cast<ir::VarianceTrackInstr *>(instr.get()))
                {
                    if (const auto *v = std::get_if<double>(&track->variance))
                    {
                        variance_by_reg[track->result_reg] = *v;
                    }
                    else if (const auto *vi = std::get_if<int64_t>(&track->variance))
                    {
                        variance_by_reg[track->result_reg] = static_cast<double>(*vi);
                    }
                }
            }
        }

        for (auto &block : function_ir.basic_blocks)
        {
            for (auto &instr : block->instructions)
            {
                if (auto *ebm = dynamic_cast<ir::DiscreteEBMInstr *>(instr.get()))
                {
                    const size_t n = ebm->spins.size();
                    if (n == 0 || ebm->J_matrix.size() != n)
                    {
                        continue;
                    }

                    // Ensure each J row has length n to avoid OOB
                    bool bad_rows = false;
                    for (size_t i = 0; i < n; ++i)
                    {
                        if (ebm->J_matrix[i].size() != n)
                        {
                            bad_rows = true;
                            break;
                        }
                    }
                    if (bad_rows)
                    {
                        continue;
                    }

                    if (!config.use_degree && !config.use_variance)
                    {
                        continue;
                    }

                    // 0) Inject external variance data if provided (profile-guided)
                    if (const char *var_str = std::getenv("NOISE_SHAPING_VARIANCES"))
                    {
                        std::stringstream ss(var_str);
                        std::string segment;
                        int idx = 0;
                        while (std::getline(ss, segment, ',') && idx < static_cast<int>(n))
                        {
                            try
                            {
                                double v = std::stod(segment);
                                if (std::holds_alternative<std::string>(ebm->spins[idx]))
                                {
                                    std::string reg = std::get<std::string>(ebm->spins[idx]);
                                    variance_by_reg[reg] = v;
                                }
                            }
                            catch (...)
                            {
                            }
                            idx++;
                        }
                        std::cout << "  NoiseShaping: Injected external variance data for " << std::min<int>(idx, static_cast<int>(n)) << " spins." << std::endl;
                    }

                    // 1) Calculate node degrees from J (if enabled)
                    std::vector<double> degrees(n, 0.0);
                    if (config.use_degree)
                    {
                        for (size_t i = 0; i < n; ++i)
                        {
                            for (size_t j = 0; j < n; ++j)
                            {
                                if (i == j)
                                    continue;
                                if (std::abs(ebm->J_matrix[i][j]) > 1e-9)
                                {
                                    degrees[i] += 1.0;
                                }
                            }
                        }
                    }

                    double max_deg = 0.0;
                    for (double d : degrees)
                        max_deg = std::max(max_deg, d);

                    std::cout << "  NoiseShapingPass: mode=" << config.mode_label
                              << ", shrink_cap=" << config.variance_shrink_cap
                              << " (" << n << " spins)" << std::endl;

                    // 2) Compute beta_i from normalized degree
                    // Heuristic: beta_i = 1 + degree_ratio; T_i = 1 / beta_i (relative temp)
                    std::vector<double> beta(n, 1.0);
                    std::vector<double> local_T(n, 1.0);
                    if (config.use_degree && max_deg > 0.0)
                    {
                        for (size_t i = 0; i < n; ++i)
                        {
                            double degree_ratio = degrees[i] / max_deg; // in [0,1]
                            double b = 1.0 + degree_ratio;              // nominal [1,2]
                            beta[i] = std::clamp(b, 1.0, 2.0);          // clamp for stability
                            local_T[i] = 1.0 / beta[i];                 // in [0.5,1]
                        }
                    }

                    // 2b) Modulate temperatures with variance (higher variance -> warmer / lower beta)
                    if (config.use_variance && !variance_by_reg.empty())
                    {
                        std::vector<double> spin_variances(n, -1.0);
                        double max_var = 0.0;

                        for (size_t i = 0; i < n; ++i)
                        {
                            if (const auto *reg = std::get_if<std::string>(&ebm->spins[i]))
                            {
                                auto it = variance_by_reg.find(*reg);
                                if (it != variance_by_reg.end())
                                {
                                    spin_variances[i] = it->second;
                                    max_var = std::max(max_var, it->second);
                                }
                            }
                        }

                        if (max_var > 0.0)
                        {
                            for (size_t i = 0; i < n; ++i)
                            {
                                if (spin_variances[i] < 0.0)
                                {
                                    continue; // no variance info
                                }
                                double var_ratio = spin_variances[i] / max_var; // [0,1]
                                // Warm high-variance spins by up to variance_shrink_cap
                                double beta_shrink = std::clamp(1.0 - config.variance_shrink_cap * var_ratio, 0.1, 1.0);
                                beta[i] *= beta_shrink;
                            }

                            for (size_t i = 0; i < n; ++i)
                            {
                                local_T[i] = 1.0 / beta[i];
                            }
                        }
                    }

                    // If nothing changed, skip
                    bool active = false;
                    for (size_t i = 0; i < n; ++i)
                    {
                        if (std::abs(beta[i] - 1.0) > 1e-9)
                        {
                            active = true;
                            break;
                        }
                    }
                    if (!active)
                    {
                        continue;
                    }

                    // 3) Scale h and J by beta_i
                    if (ebm->h_vector.size() == n)
                    {
                        for (size_t i = 0; i < n; ++i)
                        {
                            ebm->h_vector[i] *= beta[i];
                        }
                    }

                    for (size_t i = 0; i < n; ++i)
                    {
                        for (size_t j = 0; j < n; ++j)
                        {
                            if (i == j)
                                continue;
                            // Symmetric scaling option: sqrt(beta_i * beta_j)
                            double scale = std::sqrt(beta[i] * beta[j]);
                            ebm->J_matrix[i][j] *= scale;
                        }
                    }

                    // 4) Record local temperatures in IR for downstream backends
                    ebm->local_temperatures = std::move(local_T);

                    modified = true;
                }
            }
        }

        if (modified)
        {
            std::cout << "  NoiseShapingPass: Completed variable-temperature scaling." << std::endl;
        }

        return modified;
    }

} // namespace thermolang::optimizer
