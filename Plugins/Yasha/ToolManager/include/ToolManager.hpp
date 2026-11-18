#pragma once

#include "Interfaces/IToolManager.hpp"
#include "Interfaces/Package.hpp"

#include <memory>
#include <optional>
#include <unordered_map>

extern "C" InitPackageFunc INIT_PACKAGE_FUNC_NAME;

class ToolManager final : public IToolManager
{
private:
    ToolManager()  = default;
    ~ToolManager() = default;

    std::unordered_map<ATool::ID, std::unique_ptr<ATool>> tools_;
    std::optional<ATool::ID>                              active_tool_id_;
    std::uint64_t                                          activation_revision_ = 0;

public:
    ToolManager(const ToolManager&)            = delete;
    ToolManager(ToolManager&&)                 = delete;
    ToolManager& operator=(const ToolManager&) = delete;
    ToolManager& operator=(ToolManager&&)      = delete;

    static ToolManager& instance();

    Error addTool(std::unique_ptr<ATool> tool) override;
    Error removeTool(ATool::ID tool_id) override;
    Error setActiveTool(ATool::ID tool_id) override;

    // clang-format off
    std::optional<ATool::ID> activeToolId() const { return active_tool_id_; }
    std::uint64_t activationRevision() const { return activation_revision_; }
    // clang-format on

    bool processEvent(const IInputPump::Event& event) override;
};
