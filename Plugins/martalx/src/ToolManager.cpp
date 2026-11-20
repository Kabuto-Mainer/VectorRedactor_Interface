#include "ToolManager.hpp"
#include "Core/ApplicationCore.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Interfaces/Package.hpp"
#include <memory>
#include <optional>
#include <variant>

/* InitPackage */

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
	ApplicationCore::Error error = ApplicationCore::instance().registerInterface(IID_TOOL_MANAGER, reinterpret_cast<void **>(&ToolManager::instance()));
	if(error != ApplicationCore::Error::SUCCESS)
	{
		ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Failed to register ToolManager: %s", ApplicationCore::strError(error));
		return false;
	}

	return true;
}

/* ToolManager */

ToolManager &ToolManager::instance()
{
	static ToolManager instance;
	return instance;
}

ToolManager::Error ToolManager::addTool(std::unique_ptr<ATool> tool)
{
	if(!tools_.try_emplace(tool->id(), std::move(tool)).second)
	{
		ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "Tool with id = %d already exists", tool->id());
		return Error::INVALID_ARGS;
	}

	return Error::SUCCESS;
}

ToolManager::Error ToolManager::removeTool(ATool::ID tool_id)
{
	auto tool_to_remove = tools_.find(tool_id);
	if(tool_to_remove == tools_.end())
	{
		ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "Attempt to delete not existing tool with id = %d", tool_id);
		return Error::TOOL_NOT_FOUND;
	}

	if(active_tool_.has_value() && tool_id == *active_tool_)
	{
		active_tool_ = std::nullopt;
	}

	tools_.erase(tool_to_remove);
	return Error::SUCCESS;
}

ToolManager::Error ToolManager::setActiveTool(ATool::ID tool_id)
{
	if(!tools_.contains(tool_id))
	{
		ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "Attempt to set not existing tool with id = %d as active", tool_id);
		return Error::TOOL_NOT_FOUND;
	}

	active_tool_ = tool_id;
	return Error::SUCCESS;
}

bool ToolManager::processEvent(const IInputPump::Event &event)
{
	if(!active_tool_.has_value())
	{
		return false;
	}

	return tools_.at(*active_tool_)->processEvent(event);
}

