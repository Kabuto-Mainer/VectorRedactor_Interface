#pragma once

#include "Interfaces/IID.hpp"
#include "Interfaces/Package.hpp"

constexpr static const IID IID_MAIN_PLUGIN = 100;

extern "C" InitPackageFunc INIT_PACKAGE_FUNC_NAME;

class IMainPlugin
{
private:
    IMainPlugin()  = default;
    ~IMainPlugin() = default;

public:
    IMainPlugin(const IMainPlugin&)            = delete;
    IMainPlugin(IMainPlugin&&)                 = delete;
    IMainPlugin& operator=(const IMainPlugin&) = delete;
    IMainPlugin& operator=(IMainPlugin&&)      = delete;

    static IMainPlugin& instance();
};
