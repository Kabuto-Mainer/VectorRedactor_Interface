#include "../include/ToolManager.hpp"
#include "Core/ApplicationCore.hpp"

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    auto err = ApplicationCore::instance().registerInterface(
        IID_TOOL_MANAGER,
        reinterpret_cast<void*>(static_cast<IToolManager*>(&ToolManager::instance()))
    );

    return err == ApplicationCore::Error::SUCCESS;
}

IToolManager::Error ToolManager::addTool(std::unique_ptr<ATool> tool)
{
    if (!tool)
    {
        return Error::INVALID_ARGS;
    }

    const ATool::ID id = tool->id();
    if (tools_.contains(id))
    {
        return Error::INVALID_ARGS;
    }

    tools_.emplace(id, std::move(tool));
    return Error::SUCCESS;
}

IToolManager::Error ToolManager::removeTool(ATool::ID tool_id)
{
    auto it = tools_.find(tool_id);
    if (it == tools_.end())
    {
        return Error::TOOL_NOT_FOUND;
    }

    if (active_tool_id_.has_value() && active_tool_id_.value() == tool_id)
    {
        active_tool_id_.reset();
    }

    tools_.erase(it);
    return Error::SUCCESS;
}

IToolManager::Error ToolManager::setActiveTool(ATool::ID tool_id)
{
    if (!tools_.contains(tool_id))
    {
        return Error::TOOL_NOT_FOUND;
    }

    active_tool_id_ = tool_id;
    return Error::SUCCESS;
}

bool ToolManager::processEvent(const IInputPump::Event& event)
{
    if (!active_tool_id_.has_value())
    {
        return false;
    }

    auto it = tools_.find(active_tool_id_.value());
    if (it == tools_.end() || !it->second)
    {
        return false;
    }

    return it->second->processEvent(event);
}