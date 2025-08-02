#ifndef THERMOLANG_TYPE_H
#define THERMOLANG_TYPE_H

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <optional>

namespace thermolang
{

    // Forward declarations
    class Type;
    class PrimitiveType;
    class DistributionType;
    class FunctionType;
    class EnergyType;
    class CircuitType;

    // Type visitor interface
    class TypeVisitor
    {
    public:
        virtual ~TypeVisitor() = default;
        virtual void visit(const PrimitiveType &type) = 0;
        virtual void visit(const DistributionType &type) = 0;
        virtual void visit(const FunctionType &type) = 0;
        virtual void visit(const EnergyType &type) = 0;
        virtual void visit(const CircuitType &type) = 0;
    };

    // Base class for all types
    class Type
    {
    public:
        virtual ~Type() = default;
        virtual bool is_compatible_with(const Type &other) const = 0;
        virtual std::string to_string() const = 0;
        virtual void accept(TypeVisitor &visitor) const = 0;

        // Factory methods for common types
        static std::shared_ptr<Type> float_type();
        static std::shared_ptr<Type> int_type();
        static std::shared_ptr<Type> bool_type();
        static std::shared_ptr<Type> string_type();
        static std::shared_ptr<Type> void_type();
    };

    // Primitive types (int, float, bool, string)
    class PrimitiveType : public Type
    {
    public:
        enum class Kind
        {
            INT,
            FLOAT,
            BOOL,
            STRING,
            VOID
        };

        explicit PrimitiveType(Kind kind);
        bool is_compatible_with(const Type &other) const override;
        std::string to_string() const override;
        void accept(TypeVisitor &visitor) const override;

        Kind get_kind() const { return kind_; }

    private:
        Kind kind_;
    };

    // Distribution types (e.g., Gaussian, Uniform)
    class DistributionType : public Type
    {
    public:
        DistributionType(std::shared_ptr<Type> element_type, std::optional<double> variance = std::nullopt);
        bool is_compatible_with(const Type &other) const override;
        std::string to_string() const override;
        void accept(TypeVisitor &visitor) const override;

        const Type &element_type() const { return *element_type_; }
        std::optional<double> variance() const { return variance_; }

    private:
        std::shared_ptr<Type> element_type_;
        std::optional<double> variance_;
    };

    // Function types
    class FunctionType : public Type
    {
    public:
        FunctionType(std::vector<std::shared_ptr<Type>> param_types, std::shared_ptr<Type> return_type);
        bool is_compatible_with(const Type &other) const override;
        std::string to_string() const override;
        void accept(TypeVisitor &visitor) const override;

        const std::vector<std::shared_ptr<Type>> &param_types() const { return param_types_; }
        const Type &return_type() const { return *return_type_; }

        std::shared_ptr<Type> get_return_type() const { return return_type_; }

    private:
        std::vector<std::shared_ptr<Type>> param_types_;
        std::shared_ptr<Type> return_type_;
    };

    // Energy function types
    class EnergyType : public Type
    {
    public:
        EnergyType(std::vector<std::shared_ptr<Type>> var_types);
        bool is_compatible_with(const Type &other) const override;
        std::string to_string() const override;
        void accept(TypeVisitor &visitor) const override;

        const std::vector<std::shared_ptr<Type>> &var_types() const { return var_types_; }

    private:
        std::vector<std::shared_ptr<Type>> var_types_;
    };

    // Circuit topology types
    class CircuitType : public Type
    {
    public:
        CircuitType(int nodes, std::string coupling_type);
        bool is_compatible_with(const Type &other) const override;
        std::string to_string() const override;
        void accept(TypeVisitor &visitor) const override;

        int nodes() const { return nodes_; }
        const std::string &coupling_type() const { return coupling_type_; }

    private:
        int nodes_;
        std::string coupling_type_;
    };

} // namespace thermolang

#endif // THERMOLANG_TYPE_H