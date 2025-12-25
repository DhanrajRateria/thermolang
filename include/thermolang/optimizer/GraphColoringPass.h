#ifndef THERMOLANG_GRAPH_COLORING_PASS_H
#define THERMOLANG_GRAPH_COLORING_PASS_H

#include "thermolang/optimizer/Passes.h"

namespace thermolang::optimizer
{
    class GraphColoringPass : public IRPass
    {
    public:
        bool run(ir::FunctionIR &function_ir) override;

    private:
        // Greedy coloring algorithm (Welsh-Powell heuristic)
        std::vector<std::vector<int>> color_graph(const std::vector<std::vector<double>>& adj_matrix);
    };
} 
#endif