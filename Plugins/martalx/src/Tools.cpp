#include "Tools.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IToolBar.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Interfaces/Package.hpp"
#include "Shapes.hpp"
#include "ToolBar.hpp"
#include <memory>

/* InitPackage */

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
	IToolManager *tool_manager = nullptr;
	ApplicationCore::Error error = ApplicationCore::instance().queryInterface(IID_TOOL_MANAGER, reinterpret_cast<void **>(&tool_manager));
	if(error != ApplicationCore::Error::SUCCESS)
	{
		ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Failed to requesting for ToolManager: %s", ApplicationCore::strError(error));
		return false;
	}

	tool_manager->addTool(std::make_unique<RectangleTool>());
	return true;
}

/* RectangleTool */

RectangleTool::RectangleTool()
{
	IToolBar *toolbar = nullptr;
	ApplicationCore::Error error = ApplicationCore::instance().queryInterface(IID_TOOL_BAR, reinterpret_cast<void **>(&toolbar));
	assert(error == ApplicationCore::Error::SUCCESS);

	toolbar->addTool(ID_TOOL_RECTANGLE, Icon("./images/rectangle.png").texture());
}

RectangleTool::ID RectangleTool::id() const
{
	return ID_TOOL_RECTANGLE;
}

bool RectangleTool::processEvent(const IInputPump::Event &event)
{
	if(!std::holds_alternative<IInputPump::EventMouse>(event))
	{
		return false;
	}

	const auto &event_mouse = std::get<IInputPump::EventMouse>(event);

	if(event_mouse.type() == IInputPump::EventMouse::Type::LEFT_BUTTON_PRESSING)
	{
		auto rectangle = std::make_unique<Rectangle>();
		rectangle->setScaling(INITIAL_SIZE);
		return true;
	}

	return false;
}

/* EllipseTool */

//EllipseTool::ID EllipseTool::id() const
//{
//	return TOOL_ID;
//}
//
//// FIXME: stub
//bool EllipseTool::processEvent(const IInputPump::Event &event)
//{
//	ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "FIXME: I don't know why %s is needed", __func__);
//	return false;
//}
//
