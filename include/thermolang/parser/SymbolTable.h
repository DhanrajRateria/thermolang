#ifndef THERMOLANG_SYMBOL_TABLE_H
#define THERMOLANG_SYMBOL_TABLE_H

#include "thermolang/types/Type.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>
#include <iostream>

namespace thermolang
{

    struct Symbol
    {
        bool is_defined = false;
        std::shared_ptr<Type> type = nullptr;
        bool is_constant = false;
        bool is_function = false;

        Symbol() = default;
        Symbol(bool defined, std::shared_ptr<Type> type, bool constant = false, bool function = false)
            : is_defined(defined), type(std::move(type)), is_constant(constant), is_function(function) {}
    };

    class SymbolTable
    {
    public:
        SymbolTable();

        // Enters a new, nested scope (e.g., inside a function body).
        void enter_scope();

        // Exits the current scope.
        void exit_scope();

        // Defines a new identifier in the current scope.
        // Returns false if it's already defined in this scope.
        bool define(const std::string &name, std::shared_ptr<Type> type = nullptr, bool is_constant = false, bool is_function = false);

        // Checks if an identifier is accessible from the current scope.
        bool resolve(const std::string &name) const;

        // Gets the symbol for an identifier.
        std::optional<Symbol> get(const std::string &name) const;

        // Updates the type of an existing symbol
        bool update_type(const std::string &name, std::shared_ptr<Type> type);

        // Dump contents of symbol table for debugging
        void debug_dump() const;

        bool is_defined_in_current_scope(const std::string &name) const;

    private:
        std::vector<std::unordered_map<std::string, Symbol>> scopes_;
    };

} // namespace thermolang

#endif // THERMOLANG_SYMBOL_TABLE_H