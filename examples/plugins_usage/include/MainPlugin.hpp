#pragma once

#include "Interfaces/IID.hpp"
#include "Interfaces/Package.hpp"
#include "Utils/TObject.hpp"

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

class MainWindow final : public TObject
{
public:
    MainWindow()  = default;
    ~MainWindow() = default;

    void  render() override;
    bool  processEvent(const IInputPump::Event& event) override;
    Vec2F mapLocalToGlobal(const Vec2F& pos) const override;
    Vec2F mapGlobalToLocal(const Vec2F& pos) const override;
};
