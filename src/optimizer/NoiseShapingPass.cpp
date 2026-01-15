// thermolang/optimizer/NoiseShapingPass.cpp
#include "thermolang/optimizer/NoiseShapingPass.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace thermolang::optimizer
{
    namespace
    {
        struct NoiseShapingConfig
        {
            bool enabled = true;
            bool use_degree = true;
            bool use_variance = true;

            // Max fractional beta shrink from variance (higher variance -> warmer -> lower beta)
            double variance_shrink_cap = 0.5;

            // Strength λ: 0 = no shaping, 1 = full shaping
            double strength = 1.0;

            // IMPORTANT: Default must match old behavior (test expects this)
            // Old: beta = 1 + degree_ratio, clamped to [1,2]
            double degree_gain = 1.0;

            std::string mode_label = "degree+variance";
        };

        NoiseShapingConfig load_config()
        {
            NoiseShapingConfig cfg;

            if (const char *env = std::getenv("NOISE_SHAPING_MODE"))
            {
                std::string mode(env);
                std::transform(mode.begin(), mode.end(), mode.begin(),
                               [](unsigned char c)
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
                    // keep default
                }
            }

            if (const char *env = std::getenv("NOISE_SHAPING_STRENGTH"))
            {
                try
                {
                    double v = std::stod(env);
                    cfg.strength = std::clamp(v, 0.0, 1.0);
                }
                catch (...)
                {
                    // keep default
                }
            }

            // Optional: allows tuning while keeping default test-compatible
            if (const char *env = std::getenv("NOISE_SHAPING_DEGREE_GAIN"))
            {
                try
                {
                    double v = std::stod(env);
                    cfg.degree_gain = std::clamp(v, 0.0, 2.0);
                }
                catch (...)
                {
                    // keep default
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

        // Pre-scan variance annotations emitted by VarianceTrackingPass
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
                        continue;

                    // Ensure each J row has length n
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
                        continue;

                    if (!config.use_degree && !config.use_variance)
                        continue;

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
                                // ignore malformed
                            }
                            idx++;
                        }
                        std::cout << "  NoiseShaping: Injected external variance data for "
                                  << std::min<int>(idx, static_cast<int>(n)) << " spins." << std::endl;
                    }

                    // 1) Degree from J
                    std::vector<double> degrees(n, 0.0);
                    if (config.use_degree)
                    {
                        for (size_t i = 0; i < n; ++i)
                        {
                            for (size_t j = 0; j < n; ++j)
                            {
                                if (i == j) continue;
                                if (std::abs(ebm->J_matrix[i][j]) > 1e-9)
                                    degrees[i] += 1.0;
                            }
                        }
                    }

                    double max_deg = 0.0;
                    for (double d : degrees) max_deg = std::max(max_deg, d);

                    std::cout << "  NoiseShapingPass: mode=" << config.mode_label
                              << ", strength=" << config.strength
                              << ", shrink_cap=" << config.variance_shrink_cap
                              << ", degree_gain=" << config.degree_gain
                              << " (" << n << " spins)" << std::endl;

                    // 2) Compute beta/local_T
                    std::vector<double> beta(n, 1.0);
                    std::vector<double> local_T(n, 1.0);

                    // --- Degree shaping (default matches old test expectation)
                    if (config.use_degree && max_deg > 0.0)
                    {
                        for (size_t i = 0; i < n; ++i)
                        {
                            double degree_ratio = degrees[i] / max_deg; // [0,1]
                            double b = 1.0 + config.degree_gain * degree_ratio; // default gain=1 => [1,2]
                            b = std::clamp(b, 1.0, 2.0); // IMPORTANT: test expects 2.0 max
                            beta[i] = b;
                            local_T[i] = 1.0 / beta[i]; // [0.5,1]
                        }
                    }

                    // --- Variance shaping (optional, only if variance exists)
                    if (config.use_variance && !variance_by_reg.empty() && config.variance_shrink_cap > 0.0)
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
                                if (spin_variances[i] < 0.0) continue;

                                double var_ratio = std::clamp(spin_variances[i] / max_var, 0.0, 1.0);
                                double beta_shrink = std::clamp(1.0 - config.variance_shrink_cap * var_ratio, 0.1, 1.0);
                                beta[i] *= beta_shrink;
                            }

                            for (size_t i = 0; i < n; ++i)
                                local_T[i] = 1.0 / beta[i];
                        }
                    }

                    // --- Strength blending (λ)
                    for (size_t i = 0; i < n; ++i)
                    {
                        double T_shaped = local_T[i];
                        double T_blend = (1.0 - config.strength) * 1.0 + config.strength * T_shaped;
                        local_T[i] = T_blend;
                        beta[i] = 1.0 / std::max(1e-9, T_blend);
                    }

                    // Skip if inactive
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
                        continue;

                    // 3) Scale h and J
                    if (ebm->h_vector.size() == n)
                    {
                        for (size_t i = 0; i < n; ++i)
                            ebm->h_vector[i] *= beta[i];
                    }

                    // Scale full matrix symmetrically (safe + matches older expectation)
                    for (size_t i = 0; i < n; ++i)
                    {
                        for (size_t j = 0; j < n; ++j)
                        {
                            if (i == j) continue;
                            double scale = std::sqrt(beta[i] * beta[j]);
                            ebm->J_matrix[i][j] *= scale;
                        }
                    }

                    // 4) Emit local temperatures
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
