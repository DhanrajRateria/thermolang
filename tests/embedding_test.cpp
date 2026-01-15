#include <gtest/gtest.h>
#include "thermolang/ir/ThermoIR.h"
#include "thermolang/optimizer/CircuitTopologyOptimizer.h"

class EmbeddingTest : public ::testing::Test {
protected:
    std::unique_ptr<thermolang::ir::FunctionIR> create_routing_problem() {
        auto func = std::make_unique<thermolang::ir::FunctionIR>();
        func->name = "main";
        
        // 3 logical spins.
        // Naive placement: s0->(0,0), s1->(1,0), s2->(2,0).
        std::vector<thermolang::ir::Operand> spins = {
            std::string("s0"), std::string("s1"), std::string("s2")
        };
        
        // Connect s0 and s2.
        // Direct path is blocked by s1.
        // This forces the optimizer to route around s1 using available grid cells.
        std::vector<std::vector<double>> J(3, std::vector<double>(3, 0.0));
        J[0][2] = -1.0; 
        J[2][0] = -1.0;
        
        std::vector<double> h = {0.0, 0.0, 0.0};
        
        auto block = std::make_unique<thermolang::ir::BasicBlock>("entry");
        block->instructions.push_back(
            std::make_unique<thermolang::ir::DiscreteEBMInstr>("res", spins, J, h)
        );
        func->basic_blocks.push_back(std::move(block));
        
        return func;
    }
};

TEST_F(EmbeddingTest, InsertsChainQubits) {
    auto func = create_routing_problem();
    thermolang::optimizer::CircuitTopologyPass pass;
    
    // Pass returns true if IR was modified (embedding happened)
    bool modified = pass.run(*func);
    
    EXPECT_TRUE(modified) << "Optimizer did not modify the IR to insert routing.";
    
    if (modified) {
        auto* ebm = dynamic_cast<thermolang::ir::DiscreteEBMInstr*>(
            func->basic_blocks[0]->instructions[0].get()
        );
        
        // Logical count: 3.
        // If routing happened, we must have added at least 1 chain qubit.
        // Expected > 3.
        EXPECT_GT(ebm->spins.size(), 3) << "Optimizer did not add physical qubits for routing.";
        std::cout << "[TEST] Logical spins: 3. Physical spins after routing: " << ebm->spins.size() << std::endl;
    }
}