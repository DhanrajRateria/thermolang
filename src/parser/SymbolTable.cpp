#include "thermolang/parser/SymbolTable.h"

namespace thermolang
{

    SymbolTable::SymbolTable()
    {
        // Start with one global scope.
        enter_scope();
    }

    void SymbolTable::enter_scope()
    {
        scopes_.emplace_back();
    }

    void SymbolTable::exit_scope()
    {
        if (!scopes_.empty())
        {
            scopes_.pop_back();
        }
    }

    bool SymbolTable::define(const std::string &name, std::shared_ptr<Type> type, bool is_constant, bool is_function)
    {
        if (scopes_.empty())
            return false; // Should not happen

        // Check if it already exists in the current scope.
        if (scopes_.back().count(name))
            return false;

        scopes_.back()[name] = Symbol(true, std::move(type), is_constant, is_function);
        return true;
    }

    bool SymbolTable::resolve(const std::string &name) const
    {
        // Search from the innermost scope outwards.
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it)
        {
            if (it->count(name))
            {
                return true;
            }
        }
        return false;
    }

    std::optional<Symbol> SymbolTable::get(const std::string &name) const
    {
        // Search from the innermost scope outwards.
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it)
        {
            auto symbol_it = it->find(name);
            if (symbol_it != it->end())
            {
                return symbol_it->second;
            }
        }
        return std::nullopt;
    }

    bool SymbolTable::update_type(const std::string &name, std::shared_ptr<Type> type)
    {
        // Search from the innermost scope outwards.
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it)
        {
            auto symbol_it = it->find(name);
            if (symbol_it != it->end())
            {
                symbol_it->second.type = std::move(type);
                return true;
            }
        }
        return false;
    }

    void SymbolTable::debug_dump() const
    {
        std::cout << "==== Symbol Table Debug Dump ====\n";
        for (size_t i = 0; i < scopes_.size(); i++)
        {
            std::cout << "Scope " << i << ":\n";
            for (const auto &[name, symbol] : scopes_[i])
            {
                std::cout << "  " << name << ": defined=" << symbol.is_defined;
                if (symbol.type)
                {
                    std::cout << ", type=" << symbol.type->to_string();
                }
                else
                {
                    std::cout << ", type=<unknown>";
                }
                std::cout << ", constant=" << symbol.is_constant;
                std::cout << ", function=" << symbol.is_function;
                std::cout << "\n";
            }
        }
        std::cout << "================================\n";
    }
    bool SymbolTable::is_defined_in_current_scope(const std::string &name) const
    {
        // If there are no scopes, it can't be defined. (A robustness check)
        if (scopes_.empty())
        {
            return false;
        }

        // Access the current scope, which is the last one in the vector.
        const auto &current_scope = scopes_.back();

        // Use the map's `count` method to efficiently check if the key exists.
        // It returns 1 if the element is found, and 0 otherwise.
        return current_scope.count(name) > 0;
    }

} // namespace thermolang