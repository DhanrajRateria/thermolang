#ifndef THERMOLANG_AST_PRINTER_H
#define THERMOLANG_AST_PRINTER_H

#include "thermolang/parser/Ast.h"
#include <string>
#include <sstream>
#include <vector>

namespace thermolang {

// Note: This visitor returns a string for easy testing.
class AstPrinter : public ExprVisitor {
public:
    std::string print(const Expr& expr) {
        // A non-const version of accept to allow string return
        const_cast<Expr&>(expr).accept(*this);
        return ss_.str();
    }

    void visit(const BinaryExpr& expr) override {
        parenthesize(expr.op.get_lexeme(), {expr.left.get(), expr.right.get()});
    }

    void visit(const UnaryExpr& expr) override {
        parenthesize(expr.op.get_lexeme(), {expr.right.get()});
    }

    void visit(const LiteralExpr& expr) override {
        ss_ << expr.value.get_lexeme();
    }
    
    void visit(const VariableExpr& expr) override {
        ss_ << expr.name.get_lexeme();
    }

    // Add the missing implementation for CallExpr
    void visit(const CallExpr& expr) override {
        // Get the callee expression
        std::string callee_name = "call";
        if (auto* var = dynamic_cast<const VariableExpr*>(expr.callee.get())) {
            callee_name = var->name.get_lexeme();
        } else {
            // For non-variable callees, we'll use a temporary string
            std::stringstream temp;
            const_cast<Expr*>(expr.callee.get())->accept(*this);
            callee_name = ss_.str();
            ss_.str(""); // Reset the stringstream
        }

        // Collect all arguments
        std::vector<Expr*> args;
        for (const auto& arg : expr.arguments) {
            args.push_back(arg.get());
        }

        // Format the call expression
        parenthesize(callee_name, args);
    }

private:
    void parenthesize(const std::string& name, const std::vector<Expr*>& exprs) {
        ss_ << "(" << name;
        for (const auto& expr : exprs) {
            ss_ << " ";
            const_cast<Expr*>(expr)->accept(*this);
        }
        ss_ << ")";
    }
    
    std::stringstream ss_;
};

} // namespace thermolang

#endif // THERMOLANG_AST_PRINTER_H