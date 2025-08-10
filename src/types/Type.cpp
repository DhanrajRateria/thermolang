#include "thermolang/types/Type.h"

namespace thermolang
{

    // Factory methods for common types
    std::shared_ptr<Type> Type::float_type()
    {
        return std::make_shared<PrimitiveType>(PrimitiveType::Kind::FLOAT);
    }

    std::shared_ptr<Type> Type::int_type()
    {
        return std::make_shared<PrimitiveType>(PrimitiveType::Kind::INT);
    }

    std::shared_ptr<Type> Type::bool_type()
    {
        return std::make_shared<PrimitiveType>(PrimitiveType::Kind::BOOL);
    }

    std::shared_ptr<Type> Type::string_type()
    {
        return std::make_shared<PrimitiveType>(PrimitiveType::Kind::STRING);
    }

    std::shared_ptr<Type> Type::void_type()
    {
        return std::make_shared<PrimitiveType>(PrimitiveType::Kind::VOID);
    }

    // PrimitiveType implementation
    PrimitiveType::PrimitiveType(Kind kind) : kind_(kind) {}

    bool PrimitiveType::is_compatible_with(const Type &other) const
    {
        if (const PrimitiveType *other_prim = dynamic_cast<const PrimitiveType *>(&other))
        {
            return kind_ == other_prim->kind_;
        }
        return false;
    }

    std::string PrimitiveType::to_string() const
    {
        switch (kind_)
        {
        case Kind::INT:
            return "int";
        case Kind::FLOAT:
            return "float";
        case Kind::BOOL:
            return "bool";
        case Kind::STRING:
            return "string";
        case Kind::VOID:
            return "void";
        default:
            return "unknown";
        }
    }

    void PrimitiveType::accept(TypeVisitor &visitor) const
    {
        visitor.visit(*this);
    }

    // DistributionType implementation
    DistributionType::DistributionType(std::shared_ptr<Type> element_type, std::optional<double> variance)
        : element_type_(std::move(element_type)), variance_(variance) {}

    bool DistributionType::is_compatible_with(const Type &other) const
    {
        if (const DistributionType *other_dist = dynamic_cast<const DistributionType *>(&other))
        {
            return element_type_->is_compatible_with(other_dist->element_type());
        }
        return false;
    }

    std::string DistributionType::to_string() const
    {
        std::string result = "distribution<" + element_type_->to_string();
        if (variance_.has_value())
        {
            result += ", variance=" + std::to_string(*variance_);
        }
        result += ">";
        return result;
    }

    void DistributionType::accept(TypeVisitor &visitor) const
    {
        visitor.visit(*this);
    }

    // FunctionType implementation
    FunctionType::FunctionType(std::vector<std::shared_ptr<Type>> param_types, std::shared_ptr<Type> return_type)
        : param_types_(std::move(param_types)), return_type_(std::move(return_type)) {}

    bool FunctionType::is_compatible_with(const Type &other) const
    {
        if (const FunctionType *other_func = dynamic_cast<const FunctionType *>(&other))
        {
            // Check return type
            if (!return_type_->is_compatible_with(other_func->return_type()))
            {
                return false;
            }

            // Check parameter types
            if (param_types_.size() != other_func->param_types().size())
            {
                return false;
            }

            for (size_t i = 0; i < param_types_.size(); ++i)
            {
                if (!param_types_[i]->is_compatible_with(*other_func->param_types()[i]))
                {
                    return false;
                }
            }

            return true;
        }
        return false;
    }

    std::string FunctionType::to_string() const
    {
        std::string result = "function<";
        for (size_t i = 0; i < param_types_.size(); ++i)
        {
            if (i > 0)
                result += ", ";
            result += param_types_[i]->to_string();
        }
        result += " -> " + return_type_->to_string() + ">";
        return result;
    }

    void FunctionType::accept(TypeVisitor &visitor) const
    {
        visitor.visit(*this);
    }

    // EnergyType implementation
    EnergyType::EnergyType(std::vector<std::shared_ptr<Type>> var_types)
        : var_types_(std::move(var_types)) {}

    bool EnergyType::is_compatible_with(const Type &other) const
    {
        if (const EnergyType *other_energy = dynamic_cast<const EnergyType *>(&other))
        {
            if (var_types_.size() != other_energy->var_types().size())
            {
                return false;
            }

            for (size_t i = 0; i < var_types_.size(); ++i)
            {
                if (!var_types_[i]->is_compatible_with(*other_energy->var_types()[i]))
                {
                    return false;
                }
            }

            return true;
        }
        if (const FunctionType *other_func = dynamic_cast<const FunctionType *>(&other))
        {
            // Check return type: must be float.
            if (auto return_prim = dynamic_cast<const PrimitiveType *>(&other_func->return_type()))
            {
                if (return_prim->get_kind() != PrimitiveType::Kind::FLOAT)
                {
                    return false;
                }
            }
            else
            {
                return false; // Must return a primitive float
            }

            // Check if parameter types match the energy function's variable types
            if (var_types_.size() != other_func->param_types().size())
            {
                return false;
            }
            for (size_t i = 0; i < var_types_.size(); ++i)
            {
                if (!var_types_[i]->is_compatible_with(*other_func->param_types()[i]))
                {
                    return false;
                }
            }
            // If all checks pass, it's compatible.
            return true;
        }
        
        return false;
    }

    std::string EnergyType::to_string() const
    {
        std::string result = "energy<";
        for (size_t i = 0; i < var_types_.size(); ++i)
        {
            if (i > 0)
                result += ", ";
            result += var_types_[i]->to_string();
        }
        result += ">";
        return result;
    }

    void EnergyType::accept(TypeVisitor &visitor) const
    {
        visitor.visit(*this);
    }

    // CircuitType implementation
    CircuitType::CircuitType(int nodes, std::string coupling_type)
        : nodes_(nodes), coupling_type_(std::move(coupling_type)) {}

    bool CircuitType::is_compatible_with(const Type &other) const
    {
        if (const CircuitType *other_circuit = dynamic_cast<const CircuitType *>(&other))
        {
            return nodes_ == other_circuit->nodes_ && coupling_type_ == other_circuit->coupling_type_;
        }
        return false;
    }

    std::string CircuitType::to_string() const
    {
        return "circuit<nodes=" + std::to_string(nodes_) +
               ", coupling=" + coupling_type_ + ">";
    }

    void CircuitType::accept(TypeVisitor &visitor) const
    {
        visitor.visit(*this);
    }

} // namespace thermolang