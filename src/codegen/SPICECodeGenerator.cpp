#include "thermolang/codegen/SPICECodeGenerator.h"
#include <iomanip>
#include <cmath>

namespace thermolang::codegen
{
    // Helper to get a constant value from the IR, needed for thermal parameters
    template <typename T>
    T get_const_from_ir(const ir::Operand &op, const std::vector<std::unique_ptr<ir::FunctionIR>> &program)
    {
        if (const T *val = std::get_if<T>(&op))
        {
            return *val;
        }
        if (const std::string *reg_name = std::get_if<std::string>(&op))
        {
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
        return T{};
    }

    std::string SPICECodeGenerator::generate(const std::vector<std::unique_ptr<ir::FunctionIR>> &program)
    {
        ss_.str(""); // Clear the stream

        // --- Find the key IR instructions ---
        const ir::CallInstr *anneal_call = nullptr;
        const ir::IsingHamiltonianInstr *ising_instr = nullptr;
        const ir::FunctionIR *main_func = nullptr;

        for (const auto &func : program)
        {
            if (func->name == "main")
            {
                main_func = func.get();
                break;
            }
        }
        if (!main_func)
            return "* ERROR: Could not find main function.\n";

        // Find the thermal_anneal call in main
        for (const auto &block : main_func->basic_blocks)
        {
            for (const auto &instr : block->instructions)
            {
                if (auto *call = dynamic_cast<ir::CallInstr *>(instr.get()))
                {
                    if (call->callee_name == "thermal_anneal")
                    {
                        anneal_call = call;
                        break;
                    }
                }
            }
            if (anneal_call)
                break;
        }
        if (!anneal_call)
            return "* ERROR: Could not find thermal_anneal call in main.\n";

        // Find the energy function definition from the anneal call
        std::string energy_func_name = std::get<std::string>(anneal_call->args[0]);
        for (const auto &func : program)
        {
            if (func->name == energy_func_name)
            {
                if (func->basic_blocks.size() == 1 && func->basic_blocks[0]->instructions.size() == 2)
                {
                    ising_instr = dynamic_cast<ir::IsingHamiltonianInstr *>(func->basic_blocks[0]->instructions[0].get());
                }
                break;
            }
        }
        if (!ising_instr)
            return "* ERROR: Could not find optimized IsingHamiltonian instruction.\n";

        generate_ising_netlist(*ising_instr, *anneal_call);

        return ss_.str();
    }

    void SPICECodeGenerator::generate_ising_netlist(const ir::IsingHamiltonianInstr &ising_instr, const ir::CallInstr &anneal_call)
    {
        const double R_BASE = 1e3;  // Base resistance of 1kOhm for scaling
        const double I_BASE = 1e-6; // Base current of 1uA for local field
        int num_spins = ising_instr.spins.size();

        ss_ << "* ThermoLang Generated SPICE Netlist for Ising Model\n";
        ss_ << "* Problem: " << std::get<std::string>(anneal_call.args[0]) << "\n";
        ss_ << "* Spins: " << num_spins << "\n\n";

        ss_ << ".title Ising Model Annealing Circuit\n\n";

        // --- Component Definitions ---
        ss_ << "* --- Coupling Resistors (J_ij) --- \n";
        const auto &J_matrix = ising_instr.J_matrix;
        for (int i = 0; i < num_spins; ++i)
        {
            for (int j = i + 1; j < num_spins; ++j)
            {
                if (std::abs(J_matrix[i][j]) > 1e-9)
                { // Only generate for non-zero couplings
                    // Resistance is inversely proportional to coupling strength
                    double resistance = R_BASE / std::abs(J_matrix[i][j]);
                    ss_ << "R" << i << "_" << j << " " << "N" << i << " " << "N" << j << " " << std::fixed << std::setprecision(2) << resistance << "\n";
                }
            }
        }

        ss_ << "\n* --- Local Field Current Sources (h_i) --- \n";
        const auto &h_vector = ising_instr.h_vector;
        for (int i = 0; i < num_spins; ++i)
        {
            if (std::abs(h_vector[i]) > 1e-9)
            {
                // Current source from GND to Node, direction depends on sign of h
                double current = h_vector[i] * I_BASE;
                ss_ << "I" << i << " 0 N" << i << " DC " << std::scientific << std::setprecision(4) << current << "\n";
            }
        }

        ss_ << "\n* --- Node Capacitors (for stability) & Noise Sources --- \n";
        for (int i = 0; i < num_spins; ++i)
        {
            // Add a small capacitor to each node to give it a time constant and prevent floating nodes
            ss_ << "C" << i << " N" << i << " 0 1nF\n";
            // Add a noise voltage source in series with a resistor to model thermal fluctuations
            // The voltage of this source will be controlled by the temperature
            ss_ << "Rnoise" << i << " N" << i << " N_noise_in" << i << " 1\n"; // 1 Ohm resistor
            ss_ << "Vnoise" << i << " N_noise_in" << i << " 0 DC 0\n";
        }

        ss_ << "\n* --- Analysis Directives --- \n";
        // Transient analysis to see the system settle
        ss_ << ".TRAN 1us 100ms UIC\n"; // Run for 100ms, starting from user-initial conditions if any

        // Noise analysis
        ss_ << "* To run noise analysis, uncomment the line below and specify an output node.\n";
        ss_ << "*.NOISE V(N0) Vnoise0 dec 10 1 100MEG\n";

        // Temperature sweep
        // .TEMP is a basic way to model temperature; a real anneal would use a PWL source.
        ss_ << "* A simple temperature sweep from high to low temp.\n";
        ss_ << ".TEMP 100 25 0\n";

        ss_ << "\n.END\n";
    }

} // namespace thermolang::codegen