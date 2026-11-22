#pragma once

#include "Core/ApplicationCore.hpp"

template<typename T>
T* queryInterface(IID iid)
{
    void* raw = nullptr;
    if (ApplicationCore::instance().queryInterface(iid, &raw) != ApplicationCore::Error::SUCCESS)
    {
        return nullptr;
    }
    return static_cast<T*>(raw);
}
