#include "thermolang/parser/SymbolTable.h"

namespace thermolang
{

    SymbolTable::SymbolTable()
    {
        // Start with one global scope.
        enter_scope();
    }

    void SymbolTable::load_builtins()
    {
        // This method populates the global scope with the signatures of built-in functions
        // that are part of the language's standard library. This allows the TypeChecker
        // to validate calls to these functions without them being defined in the user's source code.

        // --- Stochastic Sampling Functions ---

        // stochastic fn sample_gaussian(mean: float, variance: float) -> distribution<float>
        auto float_dist_type = std::make_shared<DistributionType>(Type::float_type());
        auto bool_dist_type = std::make_shared<DistributionType>(Type::bool_type());

        auto draw_sample_type = std::make_shared<FunctionType>(
            std::vector<std::shared_ptr<Type>>{float_dist_type},
            Type::float_type() // Returns a single float.
        );
        define("draw_sample", draw_sample_type, false, true);

        auto sample_gaussian_type = std::make_shared<FunctionType>(
            std::vector<std::shared_ptr<Type>>{Type::float_type(), Type::float_type()},
            float_dist_type // Correctly returns a distribution object.
        );
        define("sample_gaussian", sample_gaussian_type, false, true);

        // `sample_uniform(low: float, high: float)` -> returns a distribution object.
        auto sample_uniform_type = std::make_shared<FunctionType>(
            std::vector<std::shared_ptr<Type>>{Type::float_type(), Type::float_type()},
            float_dist_type);
        define("sample_uniform", sample_uniform_type, false, true);

        // `sample_bernoulli(p: float)` -> returns a distribution object.
        auto sample_bernoulli_type = std::make_shared<FunctionType>(
            std::vector<std::shared_ptr<Type>>{Type::float_type()},
            bool_dist_type);
        define("sample_bernoulli", sample_bernoulli_type, false, true);

        // --- Thermodynamic Operations ---

        // Note: For simplicity, we assume the built-in functions operate on energy functions
        // that take one float variable. A more advanced implementation could use templates or generics.
        auto single_var_energy_type = std::make_shared<EnergyType>(std::vector<std::shared_ptr<Type>>{Type::float_type()});

        // thermal fn minimize_energy(energy_func: energy<float>, initial_state: float) -> float
        auto minimize_energy_type = std::make_shared<FunctionType>(
            std::vector<std::shared_ptr<Type>>{single_var_energy_type, Type::float_type()},
            Type::float_type());
        define("minimize_energy", minimize_energy_type, false, true);

        // thermal fn thermal_anneal(energy_func: energy<float>, initial_temp: float, cooling_rate: float, steps: int) -> float
        auto thermal_anneal_type = std::make_shared<FunctionType>(
            std::vector<std::shared_ptr<Type>>{single_var_energy_type, Type::float_type(), Type::float_type(), Type::int_type()},
            Type::float_type());
        define("thermal_anneal", thermal_anneal_type, false, true);
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