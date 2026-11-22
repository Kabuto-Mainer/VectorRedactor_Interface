#include "ToolBar.hpp"
#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDesktop.hpp"
#include "Interfaces/IToolBar.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Interfaces/Package.hpp"
#include "UIFW/UIFW.hpp"
#include "Utils/BoxF.hpp"
#include <cassert>
#include <memory>

using namespace MartAlx;

/* InitPackage */

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
	ApplicationCore::Error error = ApplicationCore::instance().registerInterface(IID_TOOLBAR, reinterpret_cast<void **>(&ToolBar::instance()));
	if(error != ApplicationCore::Error::SUCCESS)
	{
		ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Failed to load ToolBar: %s", ApplicationCore::strError(error));
		return false;
	}

	return true;
}

/* ToolBar */

ToolBar::ToolBar()
{
	IDesktop *desktop = nullptr;
	ApplicationCore::Error error = ApplicationCore::instance().queryInterface(IID_DESKTOP, reinterpret_cast<void **>(&desktop));
	assert(error == ApplicationCore::Error::SUCCESS);

	auto node = std::make_unique<Node>();
	node_id_ = node->id();
	desktop->add(std::move(node));
}

ToolBar &ToolBar::instance()
{
	static ToolBar instance;
	return instance;
}

// FIXME: how to render hovered, pressedbuttons?
void ToolBar::addTool(IToolManager::ATool::ID tool_id, IRenderer::TextureID texture_id)
{
	auto *node = static_cast<Node *>(ApplicationCore::instance().findObject(node_id_));
	assert(node);

	node->addToolButton(tool_id, texture_id);
}

void ToolBar::removeTool(IToolManager::ATool::ID tool_id)
{
	ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "FIXME: %s is a stub", __func__);
}

/* ToolBar::Node */

ToolBar::Node::Node()
{
	setBackground(BACKGROUND);
	setBox(BoxF(TOOLBAR_SIZE));
}

bool ToolBar::Node::processEvent(const IInputPump::Event &event)
{
	return processEventChildren(event);
}

void ToolBar::Node::addToolButton(IToolManager::ATool::ID tool_id, IRenderer::TextureID texture_id)
{
	auto button = std::make_unique<UIFW::StandardButton>(UIFW::Icon(texture_id));

	auto button_box = BoxF(BUTTON_SIZE);
	button_box.setCenter(next_button_pos_);
	next_button_pos_ += BUTTON_SIZE;
	button->setBox(button_box);

	auto button_action = [tool_id]() -> void
	{
		ApplicationCore::log(ApplicationCore::LogLevel::OK, "selected tool with id = %d", tool_id);

		IToolManager *tool_manager = nullptr;
		ApplicationCore::Error error = ApplicationCore::instance().queryInterface(IID_TOOL_MANAGER, reinterpret_cast<void **>(&tool_manager));
		assert(error == ApplicationCore::Error::SUCCESS);

		tool_manager->setActiveTool(tool_id);
	};
	button->connectOnPress(button_action);

	addChild(std::move(button));
}

