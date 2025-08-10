#include "thermolang/semantics/TypeChecker.h"

namespace thermolang
{

    // TypeResolver implementation
    TypeResolver::TypeResolver(SymbolTable &symbols) : symbols_(symbols), result_(nullptr) {}

    std::shared_ptr<Type> TypeResolver::resolve(const TypeExpr &type_expr)
    {
        // Reset the result
        result_ = nullptr;

        // Visit the type expression
        const_cast<TypeExpr &>(type_expr).accept(*this);

        return result_;
    }

    void TypeResolver::visit(const NameTypeExpr &expr)
    {
        result_ = resolve_primitive(expr.name.get_lexeme());
        if (!result_)
        {
            // Check if it's a user-defined type
            auto symbol_opt = symbols_.get(expr.name.get_lexeme());
            if (symbol_opt && symbol_opt->type)
            {
                result_ = symbol_opt->type;
            }
            else
            {
                std::cerr << "Type Error: Unknown type '" << expr.name.get_lexeme() << "'" << std::endl;
                result_ = Type::void_type(); // Use void as a fallback
            }
        }
    }

    void TypeResolver::visit(const DistributionTypeExpr &expr)
    {
        auto element_type = resolve(*expr.element_type);

        std::optional<double> variance;
        if (expr.variance)
        {
            // Extract variance value from token
            if (expr.variance->get_type() == TokenType::FLOAT_LITERAL)
            {
                variance = std::get<double>(expr.variance->get_literal());
            }
        }

        result_ = std::make_shared<DistributionType>(element_type, variance);
    }

    void TypeResolver::visit(const FunctionTypeExpr &expr)
    {
        // Changed from unique_ptr to shared_ptr
        std::vector<std::shared_ptr<Type>> param_types;
        for (const auto &param_type_expr : expr.param_types)
        {
            auto param_type = resolve(*param_type_expr);
            param_types.push_back(param_type); // Now this works with shared_ptr
        }

        auto return_type = resolve(*expr.return_type);

        result_ = std::make_shared<FunctionType>(std::move(param_types), return_type);
    }

    void TypeResolver::visit(const EnergyTypeExpr &expr)
    {
        // Changed from unique_ptr to shared_ptr
        std::vector<std::shared_ptr<Type>> var_types;
        for (const auto &var_type_expr : expr.var_types)
        {
            auto var_type = resolve(*var_type_expr);
            var_types.push_back(var_type); // Now this works with shared_ptr
        }

        result_ = std::make_shared<EnergyType>(std::move(var_types));
    }

    void TypeResolver::visit(const CircuitTypeExpr &expr)
    {
        // Extract node count from token
        int nodes = 0;
        if (expr.nodes.get_type() == TokenType::INTEGER_LITERAL)
        {
            nodes = static_cast<int>(std::get<int64_t>(expr.nodes.get_literal()));
        }

        result_ = std::make_shared<CircuitType>(nodes, expr.couplings.get_lexeme());
    }

    std::shared_ptr<Type> TypeResolver::resolve_primitive(const std::string &name)
    {
        if (name == "int")
            return std::make_shared<PrimitiveType>(PrimitiveType::Kind::INT);
        if (name == "float")
            return std::make_shared<PrimitiveType>(PrimitiveType::Kind::FLOAT);
        if (name == "bool")
            return std::make_shared<PrimitiveType>(PrimitiveType::Kind::BOOL);
        if (name == "string")
            return std::make_shared<PrimitiveType>(PrimitiveType::Kind::STRING);
        if (name == "void")
            return std::make_shared<PrimitiveType>(PrimitiveType::Kind::VOID);
        return nullptr;
    }

    class PrePassVisitor : public StmtVisitor
    {
    public:
        PrePassVisitor(SymbolTable &symbols, TypeResolver &resolver)
            : symbols_(symbols), resolver_(resolver), current_pass_(PASS_TYPES), had_error_(false) {}

        bool had_error() const { return had_error_; }

        void run(const std::vector<std::unique_ptr<Stmt>> &statements)
        {
            // Pass 1: Find all type aliases
            current_pass_ = PASS_TYPES;
            for (const auto &stmt : statements)
            {
                if (stmt)
                    stmt->accept(*this);
            }

            // Pass 2: Find all function signatures
            current_pass_ = PASS_FUNCTIONS;
            for (const auto &stmt : statements)
            {
                if (stmt)
                    stmt->accept(*this);
            }
        }

        void visit(const TypeStmt &stmt) override
        {
            if (current_pass_ != PASS_TYPES)
                return;
            auto resolved_type = resolver_.resolve(*stmt.type_expr);
            if (!symbols_.define(stmt.name.get_lexeme(), resolved_type))
            {
                // Error already handled by semantic analyzer, just skip
            }
        }

        void visit(const FunctionStmt &stmt) override
        {
            if (current_pass_ != PASS_FUNCTIONS)
                return;
            register_function(stmt);
        }
        void visit(const StochasticStmt &stmt) override
        {
            if (current_pass_ != PASS_FUNCTIONS)
                return;
            register_function(*stmt.function, true);
        }
        void visit(const EnergyStmt &stmt) override
        {
            if (current_pass_ != PASS_FUNCTIONS)
                return;
            register_function(*stmt.function, false, true);
        }

        // Recurse into blocks to find nested declarations
        void visit(const BlockStmt &stmt) override
        {
            for (const auto &s : stmt.statements)
            {
                s->accept(*this);
            }
        }
        void visit(const IfStmt &stmt) override
        {
            stmt.then_branch->accept(*this);
            if (stmt.else_branch)
                stmt.else_branch->accept(*this);
        }
        void visit(const WhileStmt &stmt) override { stmt.body->accept(*this); }
        void visit(const AnnotationStmt &stmt) override { stmt.statement->accept(*this); }

        // Ignore all other statements during pre-pass
        void visit(const LetStmt &stmt) override {}
        void visit(const ExprStmt &stmt) override {}
        void visit(const ReturnStmt &stmt) override {}
        void visit(const ThermalStmt &stmt) override { stmt.block->accept(*this); }
        void visit(const ParallelStmt &stmt) override { stmt.block->accept(*this); }

    private:
        bool had_error_;
        enum PassType
        {
            PASS_TYPES,
            PASS_FUNCTIONS
        };
        PassType current_pass_;
        SymbolTable &symbols_;
        TypeResolver &resolver_;

        void register_function(const FunctionStmt &stmt, bool is_stochastic = false, bool is_energy = false)
        {
            if (symbols_.is_defined_in_current_scope(stmt.name.get_lexeme()))
                return;
            std::vector<std::shared_ptr<Type>> param_types;
            for (const auto &param : stmt.params)
            {
                param_types.push_back(resolver_.resolve(*param.type));
            }
            auto return_type = resolver_.resolve(*stmt.return_type);
            std::shared_ptr<Type> final_func_type;
            if (is_energy)
            {
                final_func_type = std::make_shared<EnergyType>(std::move(param_types));
                // We also check here if the declared return type is float, a core rule for energy funcs
                if (auto rt_prim = std::dynamic_pointer_cast<PrimitiveType>(return_type))
                {
                    if (rt_prim->get_kind() != PrimitiveType::Kind::FLOAT)
                    {
                        std::cerr << "Type Error: Energy function '" << stmt.name.get_lexeme() << "' must be declared to return 'float'." << std::endl;
                        had_error_ = true; // Set the flag
                    }
                }
                else
                {
                    std::cerr << "Type Error: Energy function '" << stmt.name.get_lexeme() << "' must return a primitive 'float' type." << std::endl;
                    had_error_ = true; // Set the flag
                }
            }
            else
            {
                final_func_type = std::make_shared<FunctionType>(std::move(param_types), return_type);
            }

            symbols_.define(stmt.name.get_lexeme(), final_func_type, false, true);
        }
    };

    // TypeChecker implementation
    TypeChecker::TypeChecker(SymbolTable &symbols)
        : symbols_(symbols), resolver_(symbols), current_function_return_type_(nullptr) {}

    bool TypeChecker::check(const std::vector<std::unique_ptr<Stmt>> &statements)
    {
        had_error_ = false;
        symbols_.load_builtins(); // Load built-ins first

        // Run the new pre-pass to register all user-defined types and functions
        PrePassVisitor pre_pass(symbols_, resolver_);
        pre_pass.run(statements);

        if (pre_pass.had_error())
        {
            return false;
        }

        // Main pass: do full type checking on everything
        for (const auto &stmt : statements)
        {
            if (stmt)
                check(*stmt);
        }
        return !had_error_;
    }

    void TypeChecker::pre_register_types(const std::vector<std::unique_ptr<Stmt>> &statements)
    {
        for (const auto &stmt : statements)
        {
            // We only care about TypeStmt in this pass
            if (auto *type_stmt = dynamic_cast<const TypeStmt *>(stmt.get()))
            {
                visit(*type_stmt);
            }
        }
    }

    void TypeChecker::pre_register_functions(const std::vector<std::unique_ptr<Stmt>> &statements)
    {
        for (const auto &stmt : statements)
        {
            pre_register_function_stmt(*stmt);
        }
    }

    void TypeChecker::pre_register_function_stmt(const Stmt &stmt)
    {
        if (auto *func_stmt = dynamic_cast<const FunctionStmt *>(&stmt))
        {
            register_function(*func_stmt);
        }
        else if (auto *stoc_stmt = dynamic_cast<const StochasticStmt *>(&stmt))
        {
            register_function(*stoc_stmt->function, true);
        }
        else if (auto *energy_stmt = dynamic_cast<const EnergyStmt *>(&stmt))
        {
            register_function(*energy_stmt->function, false, true);
        }
        else if (auto *block_stmt = dynamic_cast<const BlockStmt *>(&stmt))
        {
            // Look for function declarations in blocks
            for (const auto &s : block_stmt->statements)
            {
                pre_register_function_stmt(*s);
            }
        }
        else if (auto *annot_stmt = dynamic_cast<const AnnotationStmt *>(&stmt))
        {
            // Check annotated statement for function declarations
            pre_register_function_stmt(*annot_stmt->statement);
        }
    }

    void TypeChecker::register_function(const FunctionStmt &stmt, bool is_stochastic, bool is_energy)
    {
        // Resolve return type
        auto return_type = resolver_.resolve(*stmt.return_type);
        if (!return_type)
        {
            std::cerr << "Error: Could not resolve return type for function "
                      << stmt.name.get_lexeme() << std::endl;
            had_error_ = true;
            return;
        }

        // Build parameter types
        std::vector<std::shared_ptr<Type>> param_types;
        for (const auto &param : stmt.params)
        {
            auto param_type = resolver_.resolve(*param.type);
            if (!param_type)
            {
                std::cerr << "Error: Could not resolve parameter type in function "
                          << stmt.name.get_lexeme() << std::endl;
                had_error_ = true;
                return;
            }
            param_types.push_back(param_type);
        }

        // Create function type
        auto func_type = std::make_shared<FunctionType>(std::move(param_types), return_type);

        // Register function in symbol table, marking it as a function
        if (!symbols_.define(stmt.name.get_lexeme(), func_type, false, true))
        {
            std::cerr << "Error: Function " << stmt.name.get_lexeme()
                      << " already defined" << std::endl;
            had_error_ = true;
        }
    }

    void TypeChecker::check(const Stmt &stmt)
    {
        const_cast<Stmt &>(stmt).accept(*this);
    }

    void TypeChecker::check(const Expr &expr)
    {
        const_cast<Expr &>(expr).accept(*this);
    }

    bool TypeChecker::check_compatibility(const Type &expected, const Type &actual, const std::string &context)
    {
        if (!expected.is_compatible_with(actual))
        {
            std::cerr << "Type Error: " << context << " - Expected '"
                      << expected.to_string() << "' but got '"
                      << actual.to_string() << "'" << std::endl;
            had_error_ = true;
            return false;
        }
        return true;
    }

    std::shared_ptr<Type> TypeChecker::infer_literal_type(const Token &token)
    {
        switch (token.get_type())
        {
        case TokenType::INTEGER_LITERAL:
            return std::make_shared<PrimitiveType>(PrimitiveType::Kind::INT);
        case TokenType::FLOAT_LITERAL:
            return std::make_shared<PrimitiveType>(PrimitiveType::Kind::FLOAT);
        case TokenType::STRING_LITERAL:
            return std::make_shared<PrimitiveType>(PrimitiveType::Kind::STRING);
        case TokenType::BOOL_LITERAL:
            return std::make_shared<PrimitiveType>(PrimitiveType::Kind::BOOL);
        default:
            return std::make_shared<PrimitiveType>(PrimitiveType::Kind::VOID);
        }
    }

    // Statement visitors
    void TypeChecker::visit(const LetStmt &stmt)
    {
        // Check the initializer if it exists
        if (stmt.initializer)
        {
            check(*stmt.initializer);
        }

        // Determine the variable's type
        std::shared_ptr<Type> var_type;

        // If there's a type annotation, use that
        if (stmt.type_annotation)
        {
            var_type = resolver_.resolve(*stmt.type_annotation);
        }
        // Otherwise, infer from the initializer
        else if (stmt.initializer && stmt.initializer->type)
        {
            var_type = stmt.initializer->type;
        }
        // If no type annotation and no initializer (or initializer with no type), we have a problem
        else
        {
            std::cerr << "Type Error: Cannot determine type for variable '"
                      << stmt.name.get_lexeme() << "'" << std::endl;
            had_error_ = true;
            symbols_.define(stmt.name.get_lexeme(), Type::void_type());
            return;
        }

        // If both type annotation and initializer, check compatibility
        if (stmt.type_annotation && stmt.initializer && stmt.initializer->type)
        {
            check_compatibility(*var_type, *stmt.initializer->type,
                                "Initializer for variable '" + stmt.name.get_lexeme() + "'");
        }

        // Define variable in symbol table with its type
        if (!symbols_.define(stmt.name.get_lexeme(), var_type))
        {
            // This case should be caught by the semantic analyzer, but we check again for safety.
            std::cerr << "Type Error: Variable '" << stmt.name.get_lexeme()
                      << "' already declared in this scope." << std::endl;
            had_error_ = true;
        }
    }

    void TypeChecker::visit(const ExprStmt &stmt)
    {
        if (stmt.expression)
        {
            check(*stmt.expression);
        }
    }

    void TypeChecker::visit(const BlockStmt &stmt)
    {
        symbols_.enter_scope();

        for (const auto &s : stmt.statements)
        {
            check(*s);
        }

        symbols_.exit_scope();
    }

    void TypeChecker::visit(const FunctionStmt &stmt)
    {
        // The function is already registered. Get its type from the symbol table.
        auto symbol_opt = symbols_.get(stmt.name.get_lexeme());
        if (!symbol_opt || !symbol_opt->type || !symbol_opt->is_function)
        {
            std::cerr << "Internal Error: Function '" << stmt.name.get_lexeme()
                      << "' was not found during the check phase." << std::endl;
            had_error_ = true;
            return;
        }

        auto func_type = std::dynamic_pointer_cast<const FunctionType>(symbol_opt->type);
        if (!func_type)
        {
            // This should not happen if pre-registration worked.
            return;
        }

        // Set up for checking the function body
        auto previous_return_type = current_function_return_type_;
        current_function_return_type_ = func_type->get_return_type(); // Directly assign the shared_ptr

        symbols_.enter_scope();

        // Define parameters in the new scope
        for (size_t i = 0; i < stmt.params.size(); ++i)
        {
            symbols_.define(stmt.params[i].name.get_lexeme(), func_type->param_types()[i]);
        }

        // Analyze the body
        check(*stmt.body);

        symbols_.exit_scope();

        // Restore previous state
        current_function_return_type_ = previous_return_type;
    }

    void TypeChecker::visit(const IfStmt &stmt)
    {
        // Type check condition - should be boolean
        check(*stmt.condition);

        auto condition_type = resolve_type(stmt.condition);
        if (condition_type && !is_boolean_type(*condition_type))
        {
            report_error("If condition must be a boolean expression.");
        }

        // Type check the then branch
        check(*stmt.then_branch);

        // If there's an else branch, check that too
        if (stmt.else_branch)
        {
            check(*stmt.else_branch);
        }
    }

    void TypeChecker::visit(const WhileStmt &stmt)
    {
        // Type check condition - should be boolean
        check(*stmt.condition);

        auto condition_type = resolve_type(stmt.condition);
        if (condition_type && !is_boolean_type(*condition_type))
        {
            report_error("While loop condition must be a boolean expression.");
        }

        // Type check the body
        check(*stmt.body);
    }

    std::shared_ptr<Type> TypeChecker::resolve_type(const std::unique_ptr<Expr> &expr)
    {
        if (!expr->type)
        {
            return Type::void_type(); // Assuming this returns a shared_ptr<Type>
        }
        return expr->type; // This already seems to be a shared_ptr<Type>
    }

    // Helper method to check if a type is boolean
    bool TypeChecker::is_boolean_type(const Type &type)
    {
        const PrimitiveType *prim_type = dynamic_cast<const PrimitiveType *>(&type);
        return prim_type && prim_type->get_kind() == PrimitiveType::Kind::BOOL;
    }

    // Helper method for error reporting
    void TypeChecker::report_error(const std::string &message)
    {
        std::cerr << "Type Error: " << message << std::endl;
        had_error_ = true;
    }

    void TypeChecker::visit(const ReturnStmt &stmt)
    {
        // If no current function, that's an error
        if (!current_function_return_type_)
        {
            std::cerr << "Type Error: Return statement outside function" << std::endl;
            had_error_ = true;
            return;
        }

        // Check return value type
        if (stmt.value)
        {
            check(*stmt.value);
            if (stmt.value->type)
            {
                check_compatibility(*current_function_return_type_, *stmt.value->type, "Return value");
            }
        }
        else
        {
            // Void return
            auto void_type = std::make_shared<PrimitiveType>(PrimitiveType::Kind::VOID);
            check_compatibility(*current_function_return_type_, *void_type, "Return value");
        }
    }

    void TypeChecker::visit(const StochasticStmt &stmt)
    {
        // The function should already be pre-registered
        check(*stmt.function);
    }

    void TypeChecker::visit(const EnergyStmt &stmt)
    {
        // The function should already be pre-registered
        auto symbol_opt = symbols_.get(stmt.function->name.get_lexeme());
        if (symbol_opt && symbol_opt->type)
        {
            if (auto func_type = std::dynamic_pointer_cast<const FunctionType>(symbol_opt->type))
            {
                // THE FIX: Enforce that the return type MUST be a float.
                if (auto return_prim = dynamic_cast<const PrimitiveType *>(&func_type->return_type()))
                {
                    if (return_prim->get_kind() != PrimitiveType::Kind::FLOAT)
                    {
                        std::cerr << "Type Error: Energy function '" << stmt.function->name.get_lexeme()
                                  << "' must return type 'float', but returns '" << return_prim->to_string()
                                  << "'." << std::endl;
                        had_error_ = true;
                    }
                }
                else
                {
                    std::cerr << "Type Error: Energy function '" << stmt.function->name.get_lexeme()
                              << "' must return a primitive 'float' type." << std::endl;
                    had_error_ = true;
                }
            }
        }
        // Then, continue to check the function's body as before.
        check(*stmt.function);
    }

    void TypeChecker::visit(const ThermalStmt &stmt)
    {
        check(*stmt.block);
    }

    void TypeChecker::visit(const ParallelStmt &stmt)
    {
        check(*stmt.block);
    }

    void TypeChecker::visit(const TypeStmt &stmt)
    {
        if (symbols_.get(stmt.name.get_lexeme()))
        {
            return;
        }
        // Resolve the type expression
        auto resolved_type = resolver_.resolve(*stmt.type_expr);

        // Register the type in the symbol table
        symbols_.define(stmt.name.get_lexeme(), resolved_type);
    }

    void TypeChecker::visit(const AnnotationStmt &stmt)
    {
        check(*stmt.statement);
    }

    // Expression visitors
    void TypeChecker::visit(const BinaryExpr &expr)
    {
        // Check operands
        check(*expr.left);
        check(*expr.right);

        // Infer result type based on operator and operands
        if (!expr.left->type || !expr.right->type)
        {
            expr.type = std::make_shared<PrimitiveType>(PrimitiveType::Kind::VOID);
            return;
        }

        // Handle arithmetic operators
        if (expr.op.get_type() == TokenType::PLUS ||
            expr.op.get_type() == TokenType::MINUS ||
            expr.op.get_type() == TokenType::STAR ||
            expr.op.get_type() == TokenType::SLASH)
        {

            // Check operand compatibility
            if (auto *left_prim = dynamic_cast<const PrimitiveType *>(expr.left->type.get()))
            {
                if (auto *right_prim = dynamic_cast<const PrimitiveType *>(expr.right->type.get()))
                {
                    // Both are primitive types
                    if (left_prim->get_kind() == PrimitiveType::Kind::INT &&
                        right_prim->get_kind() == PrimitiveType::Kind::INT)
                    {
                        expr.type = std::make_shared<PrimitiveType>(PrimitiveType::Kind::INT);
                        return;
                    }

                    if ((left_prim->get_kind() == PrimitiveType::Kind::INT ||
                         left_prim->get_kind() == PrimitiveType::Kind::FLOAT) &&
                        (right_prim->get_kind() == PrimitiveType::Kind::INT ||
                         right_prim->get_kind() == PrimitiveType::Kind::FLOAT))
                    {
                        expr.type = std::make_shared<PrimitiveType>(PrimitiveType::Kind::FLOAT);
                        return;
                    }

                    // String concatenation
                    if (expr.op.get_type() == TokenType::PLUS &&
                        left_prim->get_kind() == PrimitiveType::Kind::STRING &&
                        right_prim->get_kind() == PrimitiveType::Kind::STRING)
                    {
                        expr.type = std::make_shared<PrimitiveType>(PrimitiveType::Kind::STRING);
                        return;
                    }
                }
            }

            // Types not compatible for arithmetic
            std::cerr << "Type Error: Operator '" << expr.op.get_lexeme()
                      << "' not defined for types '" << expr.left->type->to_string()
                      << "' and '" << expr.right->type->to_string() << "'" << std::endl;
            had_error_ = true;
            expr.type = std::make_shared<PrimitiveType>(PrimitiveType::Kind::VOID);
        }
        // Handle comparison operators
        else if (expr.op.get_type() == TokenType::EQUAL_EQUAL ||
                 expr.op.get_type() == TokenType::BANG_EQUAL ||
                 expr.op.get_type() == TokenType::LESS ||
                 expr.op.get_type() == TokenType::LESS_EQUAL ||
                 expr.op.get_type() == TokenType::GREATER ||
                 expr.op.get_type() == TokenType::GREATER_EQUAL)
        {

            // Check if operands are comparable
            if (!expr.left->type->is_compatible_with(*expr.right->type))
            {
                std::cerr << "Type Error: Cannot compare types '" << expr.left->type->to_string()
                          << "' and '" << expr.right->type->to_string() << "'" << std::endl;
                had_error_ = true;
            }

            // Comparison operators always yield boolean
            expr.type = std::make_shared<PrimitiveType>(PrimitiveType::Kind::BOOL);
        }
        // Handle assignment
        else if (expr.op.get_type() == TokenType::EQUAL)
        {
            // Check if left is a variable
            if (auto *var_expr = dynamic_cast<const VariableExpr *>(expr.left.get()))
            {
                // Check if the variable is defined
                auto symbol_opt = symbols_.get(var_expr->name.get_lexeme());
                if (!symbol_opt)
                {
                    std::cerr << "Type Error: Undefined variable '" << var_expr->name.get_lexeme() << "'" << std::endl;
                    had_error_ = true;
                    expr.type = std::make_shared<PrimitiveType>(PrimitiveType::Kind::VOID);
                    return;
                }

                // Check if the types are compatible
                check_compatibility(*symbol_opt->type, *expr.right->type,
                                    "Assignment to variable '" + var_expr->name.get_lexeme() + "'");

                // Assignment expressions return the assigned value
                expr.type = symbol_opt->type;
            }
            else
            {
                std::cerr << "Type Error: Left-hand side of assignment must be a variable" << std::endl;
                had_error_ = true;
                expr.type = std::make_shared<PrimitiveType>(PrimitiveType::Kind::VOID);
            }
        }
    }

    void TypeChecker::visit(const UnaryExpr &expr)
    {
        // Check operand
        check(*expr.right);

        if (!expr.right->type)
        {
            expr.type = std::make_shared<PrimitiveType>(PrimitiveType::Kind::VOID);
            return;
        }

        // Handle negation
        if (expr.op.get_type() == TokenType::MINUS)
        {
            if (auto *prim = dynamic_cast<const PrimitiveType *>(expr.right->type.get()))
            {
                if (prim->get_kind() == PrimitiveType::Kind::INT ||
                    prim->get_kind() == PrimitiveType::Kind::FLOAT)
                {
                    expr.type = expr.right->type;
                    return;
                }
            }

            std::cerr << "Type Error: Unary '-' requires numeric operand, got '"
                      << expr.right->type->to_string() << "'" << std::endl;
            had_error_ = true;
            expr.type = std::make_shared<PrimitiveType>(PrimitiveType::Kind::VOID);
        }
        // Handle logical not
        else if (expr.op.get_type() == TokenType::BANG)
        {
            if (auto *prim = dynamic_cast<const PrimitiveType *>(expr.right->type.get()))
            {
                if (prim->get_kind() == PrimitiveType::Kind::BOOL)
                {
                    expr.type = expr.right->type;
                    return;
                }
            }

            std::cerr << "Type Error: Unary '!' requires boolean operand, got '"
                      << expr.right->type->to_string() << "'" << std::endl;
            had_error_ = true;
            expr.type = std::make_shared<PrimitiveType>(PrimitiveType::Kind::VOID);
        }
    }

    void TypeChecker::visit(const LiteralExpr &expr)
    {
        expr.type = infer_literal_type(expr.value);
    }

    void TypeChecker::visit(const VariableExpr &expr)
    {
        auto symbol_opt = symbols_.get(expr.name.get_lexeme());
        if (!symbol_opt)
        {
            std::cerr << "Type Error: Undefined variable '" << expr.name.get_lexeme() << "'" << std::endl;
            had_error_ = true;
            expr.type = std::make_shared<PrimitiveType>(PrimitiveType::Kind::VOID);
            return;
        }

        expr.type = symbol_opt->type;
    }

    void TypeChecker::visit(const CallExpr &expr)
    {
        // Check callee
        check(*expr.callee);

        if (auto *var_expr = dynamic_cast<const VariableExpr *>(expr.callee.get()))
        {
            if (var_expr->name.get_lexeme() == "thermal_anneal")
            {
                // Rule 1: Must have exactly 4 arguments
                if (expr.arguments.size() != 4)
                {
                    std::cerr << "Type Error: 'thermal_anneal' expects 4 arguments (energy_func, initial_temp, cooling_rate, steps), but got "
                              << expr.arguments.size() << "." << std::endl;
                    had_error_ = true;
                    expr.type = Type::void_type(); // Set error type and exit
                    return;
                }

                // Rule 2: First argument must be *any* EnergyType
                check(*expr.arguments[0]); // Analyze the argument
                if (!dynamic_cast<const EnergyType *>(expr.arguments[0]->type.get()))
                {
                    std::cerr << "Type Error: Argument 1 of 'thermal_anneal' must be an energy function, but got '"
                              << expr.arguments[0]->type->to_string() << "'." << std::endl;
                    had_error_ = true;
                }

                // Rule 3: Check the types of the schedule parameters
                check(*expr.arguments[1]); // initial_temp
                check_compatibility(*Type::float_type(), *expr.arguments[1]->type, "Argument 2 of 'thermal_anneal' (initial_temp)");

                check(*expr.arguments[2]); // cooling_rate
                check_compatibility(*Type::float_type(), *expr.arguments[2]->type, "Argument 3 of 'thermal_anneal' (cooling_rate)");

                check(*expr.arguments[3]); // steps
                check_compatibility(*Type::int_type(), *expr.arguments[3]->type, "Argument 4 of 'thermal_anneal' (steps)");

                // The result of annealing is currently a placeholder float. This can be refined later.
                expr.type = Type::float_type();
                return; // Exit, as we have handled this special call.
            }
        }

        // Check arguments
        for (const auto &arg : expr.arguments)
        {
            check(*arg);
        }

        // Check if callee is a function type
        if (!expr.callee->type)
        {
            expr.type = std::make_shared<PrimitiveType>(PrimitiveType::Kind::VOID);
            return;
        }

        const FunctionType *func_type = dynamic_cast<const FunctionType *>(expr.callee->type.get());
        if (!func_type)
        {
            std::cerr << "Type Error: Called object is not a function, got '"
                      << expr.callee->type->to_string() << "'" << std::endl;
            had_error_ = true;
            expr.type = std::make_shared<PrimitiveType>(PrimitiveType::Kind::VOID);
            return;
        }

        // Check argument count
        if (expr.arguments.size() != func_type->param_types().size())
        {
            std::cerr << "Type Error: Function call with wrong number of arguments, expected "
                      << func_type->param_types().size() << " but got "
                      << expr.arguments.size() << std::endl;
            had_error_ = true;
            expr.type = std::make_shared<PrimitiveType>(PrimitiveType::Kind::VOID);
            return;
        }

        // Check argument types
        for (size_t i = 0; i < expr.arguments.size(); i++)
        {
            if (!expr.arguments[i]->type)
                continue;

            check_compatibility(*func_type->param_types()[i], *expr.arguments[i]->type,
                                "Function argument " + std::to_string(i + 1));
        }

        // Create a copy of the return type
        // if (auto *prim_return = dynamic_cast<const PrimitiveType *>(&func_type->return_type()))
        // {
        //     expr.type = std::make_shared<PrimitiveType>(prim_return->get_kind());
        // }
        // else if (auto *dist_return = dynamic_cast<const DistributionType *>(&func_type->return_type()))
        // {
        //     expr.type = std::make_shared<DistributionType>(
        //         std::dynamic_pointer_cast<Type>(
        //             std::make_shared<PrimitiveType>(
        //                 dynamic_cast<const PrimitiveType &>(dist_return->element_type()).get_kind())),
        //         dist_return->variance());
        // }
        // else
        // {
        //     // For other types, create a safe default
        //     expr.type = Type::void_type();
        // }
        expr.type = func_type->get_return_type();
    }

} // namespace thermolang