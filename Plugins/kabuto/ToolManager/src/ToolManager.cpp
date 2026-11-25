#include "ToolManager.hpp"

#include "Core/ApplicationCore.hpp"

#include <utility>

namespace Kabuto
{

namespace
{

void logToolWarning(const char* message, IToolManager::ATool::ID tool_id)
{
    ApplicationCore::log(ApplicationCore::LogLevel::WARNING, message, static_cast<unsigned long long>(tool_id));
}

} // namespace

/* ToolManager */

ToolManager& ToolManager::instance()
{
    static ToolManager manager;
    return manager;
}

IToolManager::Error ToolManager::addTool(std::unique_ptr<ATool> tool)
{
    if (!tool)
    {
        return Error::INVALID_ARGS;
    }

    const ATool::ID tool_id = tool->id();
    if (!tools_.try_emplace(tool_id, std::move(tool)).second)
    {
        logToolWarning("Tool %llu is already registered", tool_id);
        return Error::INVALID_ARGS;
    }

    s_add_tool_(tool_id);
    return Error::SUCCESS;
}

IToolManager::Error ToolManager::removeTool(ATool::ID tool_id)
{
    const auto found = tools_.find(tool_id);
    if (found == tools_.end())
    {
        logToolWarning("Attempt to remove unknown tool %llu", tool_id);
        return Error::TOOL_NOT_FOUND;
    }

    const bool was_active = active_tool_id_ == tool_id;

    tools_.erase(found);

    if (was_active)
    {
        active_tool_id_.reset();
        s_switch_tool_(tool_id, IToolManagerPro::NO_TOOL);
    }

    s_remove_tool_(tool_id);
    return Error::SUCCESS;
}

IToolManager::Error ToolManager::setActiveTool(ATool::ID tool_id)
{
    if (!tools_.contains(tool_id))
    {
        logToolWarning("Attempt to activate unknown tool %llu", tool_id);
        return Error::TOOL_NOT_FOUND;
    }

    if (active_tool_id_ == tool_id)
    {
        return Error::SUCCESS;
    }

    const ID previous = active_tool_id_.value_or(IToolManagerPro::NO_TOOL);
    active_tool_id_   = tool_id;
    s_switch_tool_(previous, tool_id);
    return Error::SUCCESS;
}

bool ToolManager::processEvent(const IInputPump::Event& event)
{
    if (!active_tool_id_.has_value())
    {
        return false;
    }

    const auto tool = tools_.find(active_tool_id_.value());
    if (tool == tools_.end() || !tool->second)
    {
        return false;
    }

    return tool->second->processEvent(event);
}

void ToolManager::followAddTool(std::function<void(ID tool_id)> callback)
{
    s_add_tool_.subscribe(std::move(callback));
}

void ToolManager::followRemoveTool(std::function<void(ID tool_id)> callback)
{
    s_remove_tool_.subscribe(std::move(callback));
}

void ToolManager::followSwitchTool(std::function<void(ID previous, ID current)> callback)
{
    s_switch_tool_.subscribe(std::move(callback));
}

std::optional<IToolManager::ATool::ID> ToolManager::activeToolId() const
{
    return active_tool_id_;
}

}; // namespace Kabuto

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    ApplicationCore&     core    = ApplicationCore::instance();
    Kabuto::ToolManager& manager = Kabuto::ToolManager::instance();

    ApplicationCore::Error error =
        core.registerInterface(IID_TOOL_MANAGER, static_cast<void*>(static_cast<IToolManager*>(&manager)));
    if (error != ApplicationCore::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Failed to register IToolManager: %s",
                             ApplicationCore::strError(error));
        return false;
    }

    error = core.registerInterface(Kabuto::IID_TOOL_MANAGER_PRO, static_cast<void*>(static_cast<Kabuto::IToolManagerPro*>(&manager)));
    if (error != ApplicationCore::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Failed to register IToolManagerPro: %s",
                             ApplicationCore::strError(error));
        return false;
    }

    return true;
}
