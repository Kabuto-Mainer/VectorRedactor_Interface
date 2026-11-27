#include "ToolManager.hpp"
#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDesktop.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Interfaces/Package.hpp"
#include <cassert>
#include <memory>
#include <optional>

using namespace MartAlx;

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

ToolManager::ToolManager()
{
	IDesktop *desktop = nullptr;
	ApplicationCore::Error error = ApplicationCore::instance().queryInterface(IID_DESKTOP, reinterpret_cast<void **>(&desktop));
	assert(error == ApplicationCore::Error::SUCCESS);

	desktop->add(std::make_unique<Node>());
}

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
	// ISSUE: why this function is needed
	ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "%s is a stub", __func__);
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

/* ToolManager::Node */

void ToolManager::Node::render()
{ }

bool ToolManager::Node::processEvent(const IInputPump::Event &event)
{
	IToolManager *tool_manager = nullptr;
	ApplicationCore::Error error = ApplicationCore::instance().queryInterface(IID_TOOL_MANAGER, reinterpret_cast<void **>(&tool_manager));
	assert(error == ApplicationCore::Error::SUCCESS);

	return tool_manager->processEvent(event);
}

Vec2F ToolManager::Node::mapLocalToGlobal(const Vec2F& pos) const
{
	assert(0 && "Unreachable");
}

Vec2F ToolManager::Node::mapGlobalToLocal(const Vec2F& pos) const
{
	assert(0 && "Unreachable");
}

