// thermolang/optimizer/NoiseShapingPass.cpp
#include "thermolang/optimizer/NoiseShapingPass.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace thermolang::optimizer
{
    namespace
    {
        static bool env_truthy(const char *s, bool default_val)
        {
            if (!s)
                return default_val;
            std::string v(s);
            std::transform(v.begin(), v.end(), v.begin(),
                           [](unsigned char c)
                           { return static_cast<char>(std::tolower(c)); });
            if (v == "1" || v == "true" || v == "yes" || v == "on")
                return true;
            if (v == "0" || v == "false" || v == "no" || v == "off")
                return false;
            return default_val;
        }

        struct NoiseShapingConfig
        {
            bool enabled = true;
            bool use_degree = true;
            bool use_variance = true;

            // cap parameter used by variance shaping:
            // - "cool" policy: beta *= (1 + cap * var_ratio)  (cool high-variance spins)
            // - "warm" policy: beta *= (1 - cap * var_ratio)  (old behavior)
            double variance_shrink_cap = 0.5;

            // Strength λ: 0 = no shaping, 1 = full shaping
            double strength = 1.0;

            // Default must match old behavior for degree-only:
            // beta = 1 + degree_ratio, clamped [1,2]
            double degree_gain = 1.0;

            // New: how to interpret variance
            // "cool" (default): cool high-variance spins (recommended)
            // "warm": warm high-variance spins (old style)
            std::string variance_policy = "cool";

            // New: keep mean beta unchanged for the spins that have variance data
            bool variance_renorm = true;

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
                }
            }

            if (const char *env = std::getenv("NOISE_SHAPING_DEGREE_GAIN"))
            {
                try
                {
                    double v = std::stod(env);
                    cfg.degree_gain = std::clamp(v, 0.0, 2.0);
                }
                catch (...)
                {
                }
            }

            if (const char *env = std::getenv("NOISE_SHAPING_VARIANCE_POLICY"))
            {
                std::string pol(env);
                std::transform(pol.begin(), pol.end(), pol.begin(),
                               [](unsigned char c)
                               { return static_cast<char>(std::tolower(c)); });
                if (pol == "warm" || pol == "shrink")
                    cfg.variance_policy = "warm";
                else
                    cfg.variance_policy = "cool";
            }

            cfg.variance_renorm = env_truthy(std::getenv("NOISE_SHAPING_VARIANCE_RENORM"), true);

            return cfg;
        }
    } // namespace

    bool NoiseShapingPass::run(ir::FunctionIR &function_ir)
    {
        const auto config = load_config();
        if (!config.enabled)
            return false;

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
                auto *ebm = dynamic_cast<ir::DiscreteEBMInstr *>(instr.get());
                if (!ebm)
                    continue;

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

                // -----------------------------
                // IMPORTANT: avoid double scaling
                // If local temperatures already exist, assume shaping already applied.
                // This prevents repeated multiplicative rescaling when passes re-run.
                // -----------------------------
                if (!ebm->local_temperatures.empty() && ebm->local_temperatures.size() == n)
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
                            if (i == j)
                                continue;
                            if (std::abs(ebm->J_matrix[i][j]) > 1e-9)
                                degrees[i] += 1.0;
                        }
                    }
                }

                double max_deg = 0.0;
                for (double d : degrees)
                    max_deg = std::max(max_deg, d);

                std::cout << "  NoiseShapingPass: mode=" << config.mode_label
                          << ", strength=" << config.strength
                          << ", shrink_cap=" << config.variance_shrink_cap
                          << ", degree_gain=" << config.degree_gain
                          << ", var_policy=" << config.variance_policy
                          << ", var_renorm=" << (config.variance_renorm ? "1" : "0")
                          << " (" << n << " spins)" << std::endl;

                // 2) Compute beta/local_T
                std::vector<double> beta(n, 1.0);
                std::vector<double> local_T(n, 1.0);

                // --- Degree shaping (default matches old test expectation)
                if (config.use_degree && max_deg > 0.0)
                {
                    for (size_t i = 0; i < n; ++i)
                    {
                        double degree_ratio = degrees[i] / max_deg;         // [0,1]
                        double b = 1.0 + config.degree_gain * degree_ratio; // gain=1 => [1,2]
                        b = std::clamp(b, 1.0, 2.0);
                        beta[i] = b;
                    }
                }

                // --- Variance shaping (robust normalization + optional renorm)
                if (config.use_variance && !variance_by_reg.empty() && config.variance_shrink_cap > 0.0)
                {
                    std::vector<int> idx_var;
                    std::vector<double> vvals(n, -1.0);

                    double vmin = 1e300;
                    double vmax = -1e300;

                    for (size_t i = 0; i < n; ++i)
                    {
                        if (const auto *reg = std::get_if<std::string>(&ebm->spins[i]))
                        {
                            auto it = variance_by_reg.find(*reg);
                            if (it != variance_by_reg.end())
                            {
                                double v = it->second;
                                vvals[i] = v;
                                idx_var.push_back(static_cast<int>(i));
                                vmin = std::min(vmin, v);
                                vmax = std::max(vmax, v);
                            }
                        }
                    }

                    const double eps = 1e-12;
                    if (!idx_var.empty() && (vmax - vmin) > eps)
                    {
                        // mean beta before variance (only on the spins that actually have variance)
                        double mean_before = 0.0;
                        for (int i : idx_var)
                            mean_before += beta[static_cast<size_t>(i)];
                        mean_before /= static_cast<double>(idx_var.size());

                        // Apply variance policy
                        for (int ii : idx_var)
                        {
                            const size_t i = static_cast<size_t>(ii);
                            double v = vvals[i];
                            double var_ratio = (v - vmin) / (vmax - vmin); // [0,1]

                            if (config.variance_policy == "warm")
                            {
                                // old style: high variance -> warmer -> lower beta
                                double mult = 1.0 - config.variance_shrink_cap * var_ratio;
                                mult = std::clamp(mult, 0.1, 1.0);
                                beta[i] *= mult;
                            }
                            else
                            {
                                // recommended: high variance -> cooler -> higher beta
                                double mult = 1.0 + config.variance_shrink_cap * var_ratio;
                                mult = std::clamp(mult, 1.0, 10.0);
                                beta[i] *= mult;
                            }
                        }

                        if (config.variance_renorm)
                        {
                            // Renormalize so mean beta on these spins remains unchanged
                            double mean_after = 0.0;
                            for (int i : idx_var)
                                mean_after += beta[static_cast<size_t>(i)];
                            mean_after /= static_cast<double>(idx_var.size());

                            if (mean_after > eps)
                            {
                                double scale = mean_before / mean_after;
                                for (int i : idx_var)
                                    beta[static_cast<size_t>(i)] *= scale;
                            }
                        }
                    }
                }

                // Compute local_T from beta
                for (size_t i = 0; i < n; ++i)
                    local_T[i] = 1.0 / std::max(1e-12, beta[i]);

                // --- Strength blending (λ) in temperature-space (keeps old behavior for λ=1)
                for (size_t i = 0; i < n; ++i)
                {
                    double T_shaped = local_T[i];
                    double T_blend = (1.0 - config.strength) * 1.0 + config.strength * T_shaped;
                    local_T[i] = T_blend;
                    beta[i] = 1.0 / std::max(1e-12, T_blend);
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

                // Scale full matrix symmetrically
                for (size_t i = 0; i < n; ++i)
                {
                    for (size_t j = 0; j < n; ++j)
                    {
                        if (i == j)
                            continue;
                        double scale = std::sqrt(beta[i] * beta[j]);
                        ebm->J_matrix[i][j] *= scale;
                    }
                }

                // 4) Emit local temperatures (marker + telemetry)
                ebm->local_temperatures = std::move(local_T);

                modified = true;
            }
        }

        if (modified)
        {
            std::cout << "  NoiseShapingPass: Completed variable-temperature scaling." << std::endl;
        }

        return modified;
    }

} // namespace thermolang::optimizer
