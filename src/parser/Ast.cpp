#include "thermolang/parser/Ast.h"
#include "thermolang/semantics/SemanticAnalyzer.h" // We need the visitor definition

namespace thermolang
{

    // Type expression accept methods
    void NameTypeExpr::accept(TypeExprVisitor &visitor) const { visitor.visit(*this); }
    void DistributionTypeExpr::accept(TypeExprVisitor &visitor) const { visitor.visit(*this); }
    void FunctionTypeExpr::accept(TypeExprVisitor &visitor) const { visitor.visit(*this); }
    void EnergyTypeExpr::accept(TypeExprVisitor &visitor) const { visitor.visit(*this); }
    void CircuitTypeExpr::accept(TypeExprVisitor &visitor) const { visitor.visit(*this); }

    // Type expression to_string methods
    std::string DistributionTypeExpr::to_string() const
    {
        std::string result = "distribution<" + element_type->to_string();
        if (variance)
        {
            result += ", variance=" + variance->get_lexeme();
        }
        result += ">";
        return result;
    }

    std::string FunctionTypeExpr::to_string() const
    {
        std::string result = "function<";
        for (size_t i = 0; i < param_types.size(); ++i)
        {
            if (i > 0)
                result += ", ";
            result += param_types[i]->to_string();
        }
        result += " -> " + return_type->to_string() + ">";
        return result;
    }

    std::string EnergyTypeExpr::to_string() const
    {
        std::string result = "energy<";
        for (size_t i = 0; i < var_types.size(); ++i)
        {
            if (i > 0)
                result += ", ";
            result += var_types[i]->to_string();
        }
        result += ">";
        return result;
    }

    std::string CircuitTypeExpr::to_string() const
    {
        return "circuit<nodes=" + nodes.get_lexeme() +
               ", couplings=" + couplings.get_lexeme() + ">";
    }

    // Expression accept methods
    void CallExpr::accept(ExprVisitor &visitor) const { visitor.visit(*this); }
    void BinaryExpr::accept(ExprVisitor &visitor) const { visitor.visit(*this); }
    void UnaryExpr::accept(ExprVisitor &visitor) const { visitor.visit(*this); }
    void LiteralExpr::accept(ExprVisitor &visitor) const { visitor.visit(*this); }
    void VariableExpr::accept(ExprVisitor &visitor) const { visitor.visit(*this); }

    // Statement accept methods
    void BlockStmt::accept(StmtVisitor &visitor) const { visitor.visit(*this); }
    void FunctionStmt::accept(StmtVisitor &visitor) const { visitor.visit(*this); }
    void ExprStmt::accept(StmtVisitor &visitor) const { visitor.visit(*this); }
    void LetStmt::accept(StmtVisitor &visitor) const { visitor.visit(*this); }
    void ReturnStmt::accept(StmtVisitor &visitor) const { visitor.visit(*this); }
    void StochasticStmt::accept(StmtVisitor &visitor) const { visitor.visit(*this); }
    void EnergyStmt::accept(StmtVisitor &visitor) const { visitor.visit(*this); }
    void ThermalStmt::accept(StmtVisitor &visitor) const { visitor.visit(*this); }
    void ParallelStmt::accept(StmtVisitor &visitor) const { visitor.visit(*this); }
    void TypeStmt::accept(StmtVisitor &visitor) const { visitor.visit(*this); }
    void AnnotationStmt::accept(StmtVisitor &visitor) const { visitor.visit(*this); }

} // namespace thermolang