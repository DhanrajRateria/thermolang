// thermolang/codegen/ThrmlCodeGenerator.cpp
#include "thermolang/codegen/ThrmlCodeGenerator.h"
#include "thermolang/ir/ThermoIR.h"

#include <iomanip>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <cmath>

namespace thermolang::codegen
{
    // --- Helper Functions ---

    // Helper to get a constant value from an operand by tracing back to its definition.
    template <typename T>
    T get_const_val(const ir::Operand &op, const std::vector<std::unique_ptr<ir::FunctionIR>> &program)
    {
        if (const T *val = std::get_if<T>(&op))
        {
            return *val; // It's already a literal value
        }
        if (const std::string *reg_name = std::get_if<std::string>(&op))
        {
            // It's a register name, find its defining LoadConstInstr
            for (const auto &func : program)
            {
                for (const auto &block : func->basic_blocks)
                {
                    for (const auto &instr : block->instructions)
                    {
                        if (auto *load = dynamic_cast<ir::LoadConstInstr *>(instr.get()))
                        {
                            if (load->result_reg == *reg_name)
                            {
                                if (const T *loaded_val = std::get_if<T>(&load->value))
                                {
                                    return *loaded_val;
                                }
                            }
                        }
                    }
                }
            }
        }
        throw std::runtime_error("Could not resolve constant value from IR for thrml codegen.");
    }

    // Helper to format a vector of doubles into a Python list string
    std::string ThrmlCodeGenerator::format_python_list(const std::vector<double> &vec)
    {
        std::stringstream out;
        out << "[";
        for (size_t i = 0; i < vec.size(); ++i)
        {
            out << vec[i] << (i == vec.size() - 1 ? "" : ", ");
        }
        out << "]";
        return out.str();
    }

    // Helper to format a vector of integers into a Python list string (used for color groups)
    std::string ThrmlCodeGenerator::format_python_list_int(const std::vector<int> &vec)
    {
        std::stringstream out;
        out << "[";
        for (size_t i = 0; i < vec.size(); ++i)
        {
            out << vec[i] << (i == vec.size() - 1 ? "" : ", ");
        }
        out << "]";
        return out.str();
    }

    // --- Main Generation Logic ---

    std::string ThrmlCodeGenerator::generate(const std::vector<std::unique_ptr<ir::FunctionIR>> &program)
    {
        std::stringstream ss;

        const ir::DenoiseInstr *denoise_instr = nullptr;
        const ir::CallInstr *anneal_call = nullptr;
        const ir::DiscreteEBMInstr *ebm_instr = nullptr;
        const ir::FunctionIR *main_func = nullptr;

        // 1) Find main
        for (const auto &func : program)
        {
            if (func->name == "main")
            {
                main_func = func.get();
                break;
            }
        }
        if (!main_func)
            return "# ERROR: Could not find main function in IR.\n";

        // 2) Scan main for either denoise or thermal_anneal
        for (const auto &block : main_func->basic_blocks)
        {
            for (const auto &instr : block->instructions)
            {
                if (auto *d = dynamic_cast<ir::DenoiseInstr *>(instr.get()))
                {
                    denoise_instr = d;
                    break;
                }
                else if (auto *call = dynamic_cast<ir::CallInstr *>(instr.get()))
                {
                    if (call->callee_name == "thermal_anneal")
                    {
                        anneal_call = call;
                        break;
                    }
                }
            }
            if (denoise_instr || anneal_call)
                break;
        }

        if (!denoise_instr && !anneal_call)
        {
            return "# ERROR: No 'thermal_anneal' or 'thermal_denoise' operation found in main.\n";
        }

        // 3) Determine energy function name
        std::string energy_func_name;
        if (denoise_instr)
        {
            energy_func_name = denoise_instr->target_energy_func;
        }
        else
        {
            if (const std::string *name = std::get_if<std::string>(&anneal_call->args[0]))
            {
                energy_func_name = *name;
            }
            else
            {
                energy_func_name = get_const_val<std::string>(anneal_call->args[0], program);
            }
        }

        // 4) Find DiscreteEBMInstr inside energy function
        for (const auto &func : program)
        {
            if (func->name == energy_func_name)
            {
                if (!func->basic_blocks.empty() && !func->basic_blocks[0]->instructions.empty())
                {
                    ebm_instr = dynamic_cast<ir::DiscreteEBMInstr *>(func->basic_blocks[0]->instructions[0].get());
                }
                break;
            }
        }
        if (!ebm_instr)
            return "# ERROR: Could not find optimized DiscreteEBMInstr for energy function '" + energy_func_name + "'.\n";

        // --- Start Python Code Generation ---
        ss << "# Generated by ThermoLang Compiler for the 'thrml' backend\n";
        ss << "# This script targets Extropic AI's thrml library using JAX.\n";
        ss << "import jax\n";
        ss << "import jax.numpy as jnp\n";
        ss << "import os\n";
        ss << "import numpy as np\n";
        ss << "from thrml import SpinNode, Block, SamplingSchedule, sample_states\n";
        ss << "from thrml.models import IsingEBM, IsingSamplingProgram, hinton_init\n\n";

        ss << "def main():\n";
        ss << "    print('--- Initializing ThermoLang/thrml Runtime ---')\n";

        // 5) Nodes
        const int num_spins = static_cast<int>(ebm_instr->spins.size());
        ss << "    # 1. Define Topology (" << num_spins << " spins)\n";
        ss << "    nodes = [SpinNode() for _ in range(" << num_spins << ")]\n";

        const bool has_local_temperatures =
            ebm_instr->local_temperatures.size() == static_cast<size_t>(num_spins) &&
            !ebm_instr->local_temperatures.empty();

        // 6) Biases
        ss << "    biases = jnp.array(" << format_python_list(ebm_instr->h_vector) << ", dtype=jnp.float32)\n";

        // 7) Edges/weights (sparse)
        ss << "    # Reconstructing graph from J matrix\n";
        ss << "    edges = []\n";
        ss << "    weights_list = []\n";
        ss << "    edge_indices_list = []\n";

        const auto &J = ebm_instr->J_matrix;
        bool has_edges = false;
        for (size_t i = 0; i < J.size(); ++i)
        {
            for (size_t j = i + 1; j < J[i].size(); ++j)
            {
                if (std::abs(J[i][j]) > 1e-9)
                {
                    ss << "    edges.append((nodes[" << i << "], nodes[" << j << "]))\n";
                    ss << "    weights_list.append(" << J[i][j] << ")\n";
                    ss << "    edge_indices_list.append((" << i << ", " << j << "))\n";
                    has_edges = true;
                }
            }
        }

        if (!has_edges && num_spins > 1)
        {
            ss << "    # WARNING: No couplings found. Adding dummy edge to prevent runtime crash.\n";
            ss << "    edges.append((nodes[0], nodes[1]))\n";
            ss << "    weights_list.append(0.0)\n";
            ss << "    edge_indices_list.append((0, 1))\n";
        }

        ss << "    weights = jnp.array(weights_list, dtype=jnp.float32)\n\n";

        // 8) Coloring blocks
        ss << "    # 2. Block Coloring (Derived from ThermoLang GraphColoringPass)\n";
        if (ebm_instr->color_groups.empty())
        {
            ss << "    # Warning: No coloring info found in IR. Defaulting to sequential blocks (slow).\n";
            ss << "    free_blocks = [Block([n]) for n in nodes]\n";
        }
        else
        {
            ss << "    # Found " << ebm_instr->color_groups.size() << " independent sets (chromatic number).\n";
            ss << "    free_blocks = [\n";
            for (const auto &group : ebm_instr->color_groups)
            {
                ss << "        Block([nodes[i] for i in " << format_python_list_int(group) << "]),\n";
            }
            ss << "    ]\n";
        }

        // 9) Resolve schedule inputs from IR
        double initial_temp_scalar = 1.0;
        int64_t steps = 0;

        if (anneal_call)
        {
            initial_temp_scalar = get_const_val<double>(anneal_call->args[1], program);
            steps = get_const_val<int64_t>(anneal_call->args[3], program);
        }
        else if (denoise_instr)
        {
            initial_temp_scalar = 1.0; // denoise uses beta ~ 1.0
            steps = get_const_val<int64_t>(denoise_instr->steps, program);
        }

        if (steps <= 0)
        {
            // Defensive fallback; should not happen in valid IR.
            steps = 1;
        }

        ss << "    # 3. Temperature configuration\n";
        ss << "    # Base beta from compiler schedule (kept consistent with previous generator)\n";
        ss << "    beta = jnp.array(1.0 / " << initial_temp_scalar << ", dtype=jnp.float32)\n";

        if (has_local_temperatures)
        {
            ss << "    # [INFO] Compiler has baked Noise Shaping (local temperatures) into weights.\n";
            ss << "    # Telemetry only:\n";
            ss << "    local_temperatures_ref = jnp.array(" << format_python_list(ebm_instr->local_temperatures)
               << ", dtype=jnp.float32)\n";
        }

        ss << "    model = IsingEBM(nodes, edges, biases, weights, beta)\n";
        ss << "    program = IsingSamplingProgram(model, free_blocks, clamped_blocks=[])\n\n";

        // --- Schedules (consistent + supports profiling without changing normal behavior) ---
        ss << "    # 3b. Schedules\n";
        ss << "    # Normal schedule: matches the previous behavior closely for anneal (warmup then one sample)\n";
        ss << "    # Profiling schedule: used ONLY to estimate per-spin variances robustly\n";
        ss << "    PROFILE_VARIANCE = os.getenv('THERMOLANG_PROFILE_VARIANCE', '0') == '1'\n";

        if (denoise_instr)
        {
            // denoise: keep as-is (single sample after full steps)
            ss << "    normal_schedule = SamplingSchedule(n_warmup=0, n_samples=1, steps_per_sample=" << steps << ")\n";
        }
        else
        {
            // anneal: restore previous split behavior (important for consistency of degree results)
            const int64_t warmup = steps / 2;
            const int64_t per_sample = std::max<int64_t>(1, steps / 2);
            ss << "    normal_schedule = SamplingSchedule(n_warmup=" << warmup
               << ", n_samples=1, steps_per_sample=" << per_sample << ")\n";
        }

        // Profiling schedule: enough samples to estimate variance; we will still select best state for FINAL_ENERGY/STATE.
        ss << "    profile_schedule = SamplingSchedule(n_warmup=2000, n_samples=200, steps_per_sample=200)\n";
        ss << "    schedule = profile_schedule if PROFILE_VARIANCE else normal_schedule\n\n";

        // Seed (patched by patch_seed.py)
        ss << "    key = jax.random.key(0)\n";
        ss << "    rcheck = jax.random.uniform(key, (3,), dtype=jnp.float32)\n";
        ss << "    print(f\"[SEED_CHECK]: {rcheck.tolist()}\")\n";
        ss << "    k_init, k_samp = jax.random.split(key, 2)\n";
        ss << "    init_state = hinton_init(k_init, model, free_blocks, ())\n";

        if (denoise_instr)
        {
            ss << "    print('Running Denoising...')\n";
        }
        else
        {
            ss << "    print('Running Thermal Annealing...')\n";
        }

        ss << "    samples = sample_states(k_samp, program, schedule, init_state, [], [Block(nodes)])\n\n";

        // --- Result processing: select BEST state across all chains/samples (fixes regression) ---
        ss << "    # 4. Result Processing & Energy Metric\n";
        ss << "    # IMPORTANT: choose the best (minimum energy) sample across all chains/samples\n";
        ss << "    # This makes comparisons stable and restores the expected benefit of degree shaping.\n";

        ss << "    samples_arr = jnp.array(samples)\n";
        ss << "    # Flatten to [K, N]\n";
        ss << "    flat = samples_arr.reshape(-1, " << num_spins << ")\n";
        ss << "    # Convert boolean spins -> Ising spins (+1/-1)\n";
        ss << "    s_all = 2 * flat.astype(jnp.int8) - 1\n\n";

        // Build idx_arr once (same indices as edges, used for energy)
        ss << "    # Re-calculating indices for energy computation\n";
        ss << "    idx_list = []\n";
        for (size_t i = 0; i < J.size(); ++i)
        {
            for (size_t j = i + 1; j < J[i].size(); ++j)
            {
                if (std::abs(J[i][j]) > 1e-9)
                {
                    ss << "    idx_list.append((" << i << ", " << j << "))\n";
                }
            }
        }
        ss << "    idx_arr = jnp.array(idx_list)\n\n";

        // Vectorized energies for all samples
        ss << "    # Field energy for each sample: E_field = - sum_i (h_i * s_i)\n";
        ss << "    E_field_all = -jnp.einsum('i,ki->k', biases, s_all.astype(jnp.float32))\n";

        ss << "    # Coupling energy for each sample: E_coupling = - sum_edges (J_ij * s_i * s_j)\n";
        ss << "    if idx_arr.size == 0:\n";
        ss << "        E_coupling_all = jnp.zeros((s_all.shape[0],), dtype=jnp.float32)\n";
        ss << "    else:\n";
        ss << "        s_i = s_all[:, idx_arr[:, 0]].astype(jnp.float32)\n";
        ss << "        s_j = s_all[:, idx_arr[:, 1]].astype(jnp.float32)\n";
        ss << "        E_coupling_all = -jnp.sum(weights[None, :] * s_i * s_j, axis=1)\n";

        ss << "    E_all = E_field_all + E_coupling_all\n";
        ss << "    best_idx = jnp.argmin(E_all)\n";
        ss << "    s = s_all[best_idx]\n";
        ss << "    final_energy = E_all[best_idx]\n\n";

        ss << "    final_state_str = ', '.join(map(str, s.tolist()))\n";
        ss << "    print(f'[FINAL_ENERGY]: {final_energy}')\n";
        ss << "    print(f'[FINAL_STATE]: [{final_state_str}]')\n";

        // Variances telemetry (robust)
        ss << "    try:\n";
        ss << "        float_samples = s_all.astype(jnp.float32)\n";
        ss << "        means = jnp.mean(float_samples, axis=0)\n";
        ss << "        variances = 1.0 - (means ** 2)\n";
        ss << "        var_str = ','.join([f'{v:.4f}' for v in variances.tolist()])\n";
        ss << "        print(f'[VARIANCES]: {var_str}')\n";
        ss << "    except Exception:\n";
        ss << "        print('[VARIANCES]: error')\n";
        ss << "    return s\n\n";

        ss << "if __name__ == '__main__':\n";
        ss << "    main()\n";

        return ss.str();
    }

} // namespace thermolang::codegen
