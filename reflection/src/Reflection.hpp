#pragma once

#include "TypeId.hpp"
#include "Function.hpp"
#include "Type.hpp"

namespace mojito {

class Type;

class Reflection : public std::enable_shared_from_this<Reflection> {
public:
    Reflection(const std::shared_ptr<Reflection>& baseReflection = nullptr)
        : m_baseReflection(baseReflection)
    {}

    template <typename TypeT, typename ... ArgT>
    Type& registerType(const std::string& name, const std::vector<TypeId>& parents, const ArgT&... args) {
        auto typePtr = new Type(getTypeId<TypeT>(), parents, *this);
        m_typesMap.emplace(name, std::shared_ptr<Type>(typePtr));
        m_typeNameMap.emplace(getTypeId<TypeT>(), name);
        return *typePtr;
    }

    template <typename TypeT, typename ... ArgT>
    Type& registerType(const std::string& name, const ArgT&... args) {
        return registerType<TypeT>(name, {}, args...);
    }

    bool hasType(const std::string& name) const;

    bool hasType(TypeId typeId) const;

    const Type& getType(const std::string& name) const;

    const Type& getType(TypeId typeId) const;

    template <typename FunctionT>
    const Function& registerFunction(const std::string& name, FunctionT functionPtr) {
        auto function = std::make_shared<Function>(*this, functionPtr);
        m_functionMap.emplace(name, function);
        return *function;
    }

    bool hasFunction(const std::string& name) const;

    const Function& getFunction(const std::string& name) const;

private:
    std::unordered_map<TypeId, std::string> m_typeNameMap;
    // FIXME: Use unique_ptr
    std::unordered_map<std::string, std::shared_ptr<Type>> m_typesMap;
    // FIXME: Use unique_ptr
    std::unordered_map<std::string, std::shared_ptr<Function>> m_functionMap;
    std::shared_ptr<Reflection> m_baseReflection;
};

} // mojito
