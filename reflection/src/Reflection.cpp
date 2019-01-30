#include "Reflection.hpp"

namespace mojito {

    bool Reflection::hasType(const std::string& name) const {
        return m_typesMap.find(name) != m_typesMap.end();
    }

    // FIXME: Check order
    bool Reflection::hasType(TypeId typeId) const {
        if (m_baseReflection != nullptr)
            return m_baseReflection->hasType(typeId);
        return hasType(m_typeNameMap.at(typeId));
    }

    const Type& Reflection::getType(const std::string& name) const {
        auto iter = m_typesMap.find(name);
        if (iter != m_typesMap.end())
            return *iter->second;
        if (m_baseReflection != nullptr)
            return m_baseReflection->getType(name);
        throw MojitoException(concat("Type \"", name, "\" is not registered"));
    }

    // FIXME: Check order
    const Type& Reflection::getType(TypeId typeId) const {
        if (m_baseReflection != nullptr)
            return m_baseReflection->getType(typeId);
        return getType(m_typeNameMap.at(typeId));
    }

    bool Reflection::hasFunction(const std::string& name) const {
        return m_functionMap.find(name) != m_functionMap.end();
    }

    const Function& Reflection::getFunction(const std::string& name) const {
        auto iter = m_functionMap.find(name);
        if (iter != m_functionMap.end())
            return *iter->second;
        if (m_baseReflection != nullptr)
            return m_baseReflection->getFunction(name);
        throw MojitoException(concat("Function \"", name, "\" is not registered."));
    }

} // mojito
