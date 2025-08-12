#include "thermolang/codegen/FPGACodeGenerator.h"
#include <fstream>
#include <iomanip>
#include <cmath>
#include <stdexcept>

namespace thermolang::codegen
{
    // Helper to get a constant value from the IR for thermal parameters
    template <typename T>
    T get_const_val(const ir::Operand &op, const std::vector<std::unique_ptr<ir::FunctionIR>> &program)
    {
        if (const T *val = std::get_if<T>(&op)) { return *val; }
        if (const std::string *reg = std::get_if<std::string>(&op)) {
            for (const auto &func : program) {
                for (const auto &block : func->basic_blocks) {
                    for (const auto &instr : block->instructions) {
                        if (auto *load = dynamic_cast<ir::LoadConstInstr *>(instr.get())) {
                            if (load->result_reg == *reg) {
                                if (const T *lval = std::get_if<T>(&load->value)) return *lval;
                            }
                        }
                    }
                }
            }
        }
        throw std::runtime_error("Could not resolve constant value from IR for FPGA codegen.");
    }

    // Convert a floating-point number to a 16-bit signed fixed-point number
    // with a specified number of fractional bits.
    int16_t FPGACodeGenerator::double_to_fixed_point(double value, int fractional_bits) {
        double scale = static_cast<double>(1 << fractional_bits);
        double scaled_value = value * scale;
        // Clamp the value to the range of a 16-bit signed integer
        scaled_value = std::max(-32768.0, std::min(32767.0, scaled_value));
        return static_cast<int16_t>(round(scaled_value));
    }

    bool FPGACodeGenerator::generate(const std::vector<std::unique_ptr<ir::FunctionIR>> &program, const std::string& base_filename)
    {
        // --- 1. Find the essential IR instructions ---
        const ir::CallInstr *anneal_call = nullptr;
        const ir::IsingHamiltonianInstr *ising_instr = nullptr;
        const ir::FunctionIR *main_func = nullptr;

        for (const auto &func : program) {
            if (func->name == "main") main_func = func.get();
        }
        if (!main_func) return false;

        for (const auto &block : main_func->basic_blocks) {
            for (const auto &instr : block->instructions) {
                if (auto *call = dynamic_cast<ir::CallInstr *>(instr.get())) {
                    if (call->callee_name == "thermal_anneal") anneal_call = call;
                }
            }
        }
        if (!anneal_call) return false;

        std::string energy_func_name = std::get<std::string>(anneal_call->args[0]);
        for (const auto &func : program) {
            if (func->name == energy_func_name) {
                if (!func->basic_blocks.empty() && !func->basic_blocks[0]->instructions.empty()) {
                    ising_instr = dynamic_cast<ir::IsingHamiltonianInstr *>(func->basic_blocks[0]->instructions[0].get());
                }
            }
        }
        if (!ising_instr) return false;

        // --- 2. Generate the Problem Configuration File (.mem) ---
        std::ofstream mem_file(base_filename + "_config.mem");
        if (!mem_file.is_open()) return false;

        const int FRAC_BITS = 12; // Must match the hardware parameter in spu_core.v
        const auto& J = ising_instr->J_matrix;
        const auto& h = ising_instr->h_vector;
        int num_spins = ising_instr->spins.size();
        int grid_size = static_cast<int>(sqrt(num_spins));

        mem_file << "// ThermoLang Generated Problem Configuration for a " << grid_size << "x" << grid_size << " grid\n";
        mem_file << "// Format: For each spin (row-major): h, J_north, J_south, J_east, J_west\n";

        for (int r = 0; r < grid_size; ++r) {
            for (int c = 0; c < grid_size; ++c) {
                int i = r * grid_size + c; // Flattened index

                // Define neighbors with wrap-around (torus) topology
                int north = ((r == 0) ? grid_size - 1 : r - 1) * grid_size + c;
                int south = ((r == grid_size - 1) ? 0 : r + 1) * grid_size + c;
                int east  = r * grid_size + ((c == grid_size - 1) ? 0 : c + 1);
                int west  = r * grid_size + ((c == 0) ? grid_size - 1 : c - 1);

                // Convert to fixed-point and write to file in hex format
                mem_file << std::hex << std::setw(4) << std::setfill('0') << (uint16_t)double_to_fixed_point(h[i], FRAC_BITS) << "\n";
                mem_file << std::hex << std::setw(4) << std::setfill('0') << (uint16_t)double_to_fixed_point(J[i][north], FRAC_BITS) << "\n";
                mem_file << std::hex << std::setw(4) << std::setfill('0') << (uint16_t)double_to_fixed_point(J[i][south], FRAC_BITS) << "\n";
                mem_file << std::hex << std::setw(4) << std::setfill('0') << (uint16_t)double_to_fixed_point(J[i][east], FRAC_BITS) << "\n";
                mem_file << std::hex << std::setw(4) << std::setfill('0') << (uint16_t)double_to_fixed_point(J[i][west], FRAC_BITS) << "\n";
            }
        }
        mem_file.close();

        // --- 3. Generate the Annealing Schedule File (.txt) ---
        std::ofstream schedule_file(base_filename + "_schedule.txt");
        if(!schedule_file.is_open()) return false;

        double initial_temp = get_const_val<double>(anneal_call->args[1], program);
        double cooling_rate = get_const_val<double>(anneal_call->args[2], program);
        int64_t steps = get_const_val<int64_t>(anneal_call->args[3], program);

        schedule_file << "// ThermoLang Generated Annealing Schedule\n";
        schedule_file << "initial_temp " << std::fixed << std::setprecision(6) << initial_temp << "\n";
        schedule_file << "cooling_rate " << std::fixed << std::setprecision(6) << cooling_rate << "\n";
        schedule_file << "steps " << steps << "\n";
        schedule_file.close();

        return true;
    }

} // namespace thermolang::codegen