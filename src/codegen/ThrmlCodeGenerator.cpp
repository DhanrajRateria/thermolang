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
        // If the value cannot be resolved at compile time, throw an error.
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

        // 1. Find the main function to locate the top-level call.
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

        // 2. Scan main for either Denoise instructions or Thermal Anneal calls.
        //    This determines the execution mode.
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

        // 3. Determine the target energy function name based on the instruction found.
        std::string energy_func_name;
        if (denoise_instr)
        {
            energy_func_name = denoise_instr->target_energy_func;
        }
        else
        {
            // anneal_call arg[0] is the function name/register
            if (const std::string *name = std::get_if<std::string>(&anneal_call->args[0]))
            {
                energy_func_name = *name;
            }
            else
            {
                // If it's a register, we might need to resolve it (simplified here assuming direct name pass for now)
                // In a full compiler, we'd track the LoadConst/CreateEnergyFunc of this register.
                // For this implementation, we assume the IR has the function name directly or resolved.
                energy_func_name = get_const_val<std::string>(anneal_call->args[0], program);
            }
        }

        // 4. Find the definition of the energy function (DiscreteEBMInstr).
        for (const auto &func : program)
        {
            if (func->name == energy_func_name)
            {
                // The optimized energy function should contain a single DiscreteEBMInstr.
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
        ss << "import numpy as np\n";
        ss << "from thrml import SpinNode, Block, SamplingSchedule, sample_states\n";
        ss << "from thrml.models import IsingEBM, IsingSamplingProgram, hinton_init\n\n";

        ss << "def main():\n";
        ss << "    print('--- Initializing ThermoLang/thrml Runtime ---')\n";

        // 5. Generate Topology (Nodes)
        int num_spins = ebm_instr->spins.size();
        ss << "    # 1. Define Topology (" << num_spins << " spins)\n";
        ss << "    nodes = [SpinNode() for _ in range(" << num_spins << ")]\n";

        // 6. Generate Biases (JAX Array)
        ss << "    biases = jnp.array(" << format_python_list(ebm_instr->h_vector) << ", dtype=jnp.float32)\n";

        // 7. Generate Edges and Weights (Sparse Graph Construction)
        ss << "    # Reconstructing graph from J matrix\n";
        ss << "    edges = []\n";
        ss << "    weights_list = []\n";
        const auto &J = ebm_instr->J_matrix;
        for (size_t i = 0; i < J.size(); ++i)
        {
            for (size_t j = i + 1; j < J[i].size(); ++j)
            {
                if (std::abs(J[i][j]) > 1e-9)
                { // Only add non-zero couplings
                    ss << "    edges.append((nodes[" << i << "], nodes[" << j << "]))\n";
                    ss << "    weights_list.append(" << J[i][j] << ")\n";
                }
            }
        }
        ss << "    weights = jnp.array(weights_list, dtype=jnp.float32)\n\n";

        // 8. Generate Block Coloring (Optimization from GraphColoringPass)
        ss << "    # 2. Block Coloring (Derived from ThermoLang GraphColoringPass)\n";
        if (ebm_instr->color_groups.empty())
        {
            // Fallback if pass didn't run or failed
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

        // 9. Generate Logic for Denoising vs Annealing
        if (denoise_instr)
        {
            // DTM Logic
            double sigma = get_const_val<double>(denoise_instr->initial_sigma, program);
            int64_t steps = get_const_val<int64_t>(denoise_instr->steps, program);

            ss << "\n    # 3. Denoising Thermodynamic Model (DTM) Configuration\n";
            ss << "    # Mode: Reverse Diffusion / Denoising\n";

            // In DTM, beta is typically fixed (e.g., 1.0) while the effective noise is managed
            // by the sampling process or a schedule that reduces sigma.
            ss << "    beta = jnp.array(1.0, dtype=jnp.float32)\n";
            ss << "    model = IsingEBM(nodes, edges, biases, weights, beta)\n";
            ss << "    program = IsingSamplingProgram(model, free_blocks, clamped_blocks=[])\n";

            // Mapping sigma/steps to a thrml schedule.
            // We interpret 'steps' as the sampling duration.
            ss << "    # Schedule derived from DenoiseInstr: steps=" << steps << ", initial_sigma=" << sigma << "\n";
            ss << "    schedule = SamplingSchedule(n_warmup=0, n_samples=1, steps_per_sample=" << steps << ")\n";

            ss << "    key = jax.random.key(0)\n";
            ss << "    k_init, k_samp = jax.random.split(key, 2)\n";

            // Initialization: Denoising starts from a high-noise state (random/hinton init)
            ss << "    init_state = hinton_init(k_init, model, free_blocks, ())\n";

            ss << "    print(f'Running Denoising (" << steps << " steps)...')\n";
            ss << "    samples = sample_states(k_samp, program, schedule, init_state, [], [Block(nodes)])\n";
        }
        else
        {
            // Standard Annealing Logic
            double initial_temp = get_const_val<double>(anneal_call->args[1], program);
            // cooling_rate is handled by the iterative loop in a full implementation,
            // or by thrml's internal scheduling if available.
            // For this output, we map to a standard sampling schedule.
            int64_t steps = get_const_val<int64_t>(anneal_call->args[3], program);

            ss << "\n    # 3. Thermal Annealing Configuration\n";
            ss << "    # Mode: Optimization / Annealing\n";
            ss << "    # Converting Temp " << initial_temp << " -> Beta " << (1.0 / initial_temp) << "\n";

            ss << "    beta = jnp.array(1.0 / " << initial_temp << ", dtype=jnp.float32)\n";
            ss << "    model = IsingEBM(nodes, edges, biases, weights, beta)\n";
            ss << "    program = IsingSamplingProgram(model, free_blocks, clamped_blocks=[])\n";

            ss << "    # Schedule: " << steps << " total steps (50% warmup)\n";
            ss << "    schedule = SamplingSchedule(n_warmup=" << steps / 2
               << ", n_samples=1, steps_per_sample=" << steps / 2 << ")\n";

            ss << "    key = jax.random.key(0)\n";
            ss << "    k_init, k_samp = jax.random.split(key, 2)\n";
            ss << "    init_state = hinton_init(k_init, model, free_blocks, ())\n";

            ss << "    print(f'Running Thermal Annealing...')\n";
            ss << "    samples = sample_states(k_samp, program, schedule, init_state, [], [Block(nodes)])\n";
        }

        // 10. Process Output
        ss << "\n    # 4. Result Processing\n";
        ss << "    # Extract final state from samples [n_samples, n_chains, n_nodes]\n";
        ss << "    # Here we take the first sample of the first chain.\n";
        ss << "    final_state = samples[0][0]\n";

        ss << "    # Convert boolean spins (True/False) back to Ising spins (+1/-1)\n";
        ss << "    final_state_int = 2 * final_state.astype(jnp.int8) - 1\n";
        ss << "    final_state_str = ', '.join(map(str, final_state_int.tolist()))\n";

        ss << "    # Standard output format for ThermoLang benchmarks\n";
        ss << "    print(f'[FINAL_STATE]: [{final_state_str}]')\n";
        ss << "    return final_state_int\n\n";

        ss << "if __name__ == '__main__':\n";
        ss << "    main()\n";

        return ss.str();
    }
} // namespace thermolang::codegen