#include "BrowseTool.hpp"
#include "Interfaces/IToolBar.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Interfaces/Package.hpp"
#include "Core/ApplicationCore.hpp"
#include "UIFW/UIFW.hpp"

using namespace MartAlx::Tools;

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
	IToolManager *tool_manager = nullptr;
	ApplicationCore::Error error = ApplicationCore::instance().queryInterface(IID_TOOL_MANAGER, reinterpret_cast<void **>(&tool_manager));
	if(error != ApplicationCore::Error::SUCCESS)
	{
		ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Failed to requesting for ToolManager: %s", ApplicationCore::strError(error));
		return false;
	}

	tool_manager->addTool(std::make_unique<Browse>());

	IToolBar *toolbar = nullptr;
	error = ApplicationCore::instance().queryInterface(IID_TOOLBAR, reinterpret_cast<void **>(&toolbar));
	if(error != ApplicationCore::Error::SUCCESS)
	{
		ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Failed to requesting for ToolBar: %s", ApplicationCore::strError(error));
		return false;
	}

	toolbar->addTool(ID_BROWSE, MartAlx::UIFW::Icon("./images/hand.png").texture());

	return true;
}

Browse::ID Browse::id() const
{
	return ID_BROWSE;
}

bool Browse::processEvent([[maybe_unused]] const IInputPump::Event &event)
{
	return false;
}

