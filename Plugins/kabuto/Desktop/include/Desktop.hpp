#pragma once

#include "Interfaces/IDesktop.hpp"
#include "Interfaces/IID.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/Package.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"

#include <memory>

extern "C" InitPackageFunc INIT_PACKAGE_FUNC_NAME;

namespace Kabuto
{

namespace Desktop
{

class DesktopNode final : public TObject
{
public:
    DesktopNode()  = default;
    ~DesktopNode() = default;

    bool  processEvent(const IInputPump::Event& event) override;
    void  render() override;
    Vec2F mapLocalToGlobal(const Vec2F& pos) const override;
    Vec2F mapGlobalToLocal(const Vec2F& pos) const override;
};

class CDesktop final : public IDesktop
{
public:
    CDesktop(const CDesktop&)            = delete;
    CDesktop(CDesktop&&)                 = delete;
    CDesktop& operator=(const CDesktop&) = delete;
    CDesktop& operator=(CDesktop&&)      = delete;

    static CDesktop& instance();

    bool initialize();

    Error add(std::unique_ptr<TObject> object) override;
    Error remove(TObject::ID object_id) override;

private:
    CDesktop()           = default;
    ~CDesktop() override = default;

    TObject* container() const;

    TObject::ID container_id_ = 0;
};

}; // namespace Desktop

}; // namespace Kabuto
