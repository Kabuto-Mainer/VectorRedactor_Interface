#include "RectangleTool.hpp"
#include "Components/AComponent.hpp"
#include "Components/Color.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolBar.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Interfaces/Package.hpp"
#include "Shapes/RectangleShape.hpp"
#include "Core/ApplicationCore.hpp"
#include "UIFW/UIFW.hpp"
#include <memory>
#include <utility>

using namespace MartAlx::Tools;

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

	tool_manager->addTool(std::make_unique<Rectangle>());

	IToolBar *toolbar = nullptr;
	error = ApplicationCore::instance().queryInterface(IID_TOOLBAR, reinterpret_cast<void **>(&toolbar));
	if(error != ApplicationCore::Error::SUCCESS)
	{
		ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Failed to requesting for ToolBar: %s", ApplicationCore::strError(error));
		return false;
	}

	toolbar->addTool(ID_RECTANGLE, MartAlx::UIFW::Icon("./images/rectangle.png").texture());

	return true;
}

/* Rectangle */

Rectangle::ID Rectangle::id() const
{
	return ID_RECTANGLE;
}

bool Rectangle::processEvent(const IInputPump::Event &event)
{
	if(!std::holds_alternative<IInputPump::EventMouse>(event))
	{
		return false;
	}

	const auto &event_mouse = std::get<IInputPump::EventMouse>(event);

	if(event_mouse.type() == IInputPump::EventMouse::Type::LEFT_BUTTON_PRESSING)
	{
		auto rectangle = std::make_unique<Shapes::Rectangle>();
		rectangle->setScaling(INITIAL_SIZE);
		rectangle->setTranslation(event_mouse.cursorPos());
		Components::AComponent *color = nullptr;
		rectangle->queryComponent(Components::ID_COLOR, &color);
		// FIXME: add pallete
		static_cast<Components::Color *>(color)->setColor(IRenderer::Color::GREEN);

		IDocument *document = nullptr;
		ApplicationCore::Error error = ApplicationCore::instance().queryInterface(IID_IDOCUMENT, reinterpret_cast<void **>(&document));
		assert(error == ApplicationCore::Error::SUCCESS);

		document->addShape(std::move(rectangle));
		ApplicationCore::log(ApplicationCore::LogLevel::OK, "Rectangle: added rectangle");
		return true;
	}

	return false;
}

