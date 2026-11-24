#pragma once

#include "Interfaces/IID.hpp"
#include "Interfaces/IToolManager.hpp"

#include <functional>

namespace Kabuto
{

constexpr static const IID IID_TOOL_MANAGER_PRO = 1'231'242;

class IToolManagerPro
{
public:
    using ID = IToolManager::ATool::ID;

    static constexpr ID NO_TOOL = 0;

    IToolManagerPro()          = default;
    virtual ~IToolManagerPro() = default;

    IToolManagerPro(const IToolManagerPro&)            = delete;
    IToolManagerPro(IToolManagerPro&&)                 = delete;
    IToolManagerPro& operator=(const IToolManagerPro&) = delete;
    IToolManagerPro& operator=(IToolManagerPro&&)      = delete;

    virtual void followAddTool(std::function<void(ID tool_id)>) = 0;
    virtual void followRemoveTool(std::function<void(ID tool_id)>) = 0;
    virtual void followSwitchTool(std::function<void(ID previous, ID current)>) = 0;
}; // class IToolManagerPro

} // namespace Kabuto
