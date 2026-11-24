#pragma once

#include "Interfaces/IToolManager.hpp"
#include "export/IToolManagerPro.hpp"
#include "Interfaces/Package.hpp"
#include "Utils/Signal.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>

extern "C" InitPackageFunc INIT_PACKAGE_FUNC_NAME;

namespace Kabuto
{

class ToolManager final : public IToolManager, public IToolManagerPro
{
public:
    ToolManager(const ToolManager&)            = delete;
    ToolManager(ToolManager&&)                 = delete;
    ToolManager& operator=(const ToolManager&) = delete;
    ToolManager& operator=(ToolManager&&)      = delete;

    static ToolManager& instance();

    Error addTool(std::unique_ptr<ATool> tool) override;
    Error removeTool(ATool::ID tool_id) override;
    Error setActiveTool(ATool::ID tool_id) override;

    bool processEvent(const IInputPump::Event& event) override;

    void followAddTool(std::function<void(ID tool_id)>) override;
    void followRemoveTool(std::function<void(ID tool_id)>) override;
    void followSwitchTool(std::function<void(ID previous, ID current)>) override;

    std::optional<ATool::ID> activeToolId() const;

private:
    ToolManager()  = default;
    ~ToolManager() = default;

    std::unordered_map<ATool::ID, std::unique_ptr<ATool>> tools_;
    std::optional<ATool::ID>                              active_tool_id_;

    Signal<ID>     s_add_tool_;
    Signal<ID>     s_remove_tool_;
    Signal<ID, ID> s_switch_tool_;
}; // class ToolManager

}; // namespace Kabuto
