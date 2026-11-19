#include "MainPlugin.hpp"
#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDesktop.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/Package.hpp"
#include "UIFramework.hpp"
#include <memory>

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
	auto blank_window = std::make_unique<BlankWindow>();
	blank_window->setBox(BoxF { 100, 100, 1000, 1000 });
	blank_window->setBackground(IRenderer::Color::WHITE);

	auto button = std::make_unique<StandardButton>();
	button->setBox(BoxF { 5, 5, 50, 50 });
	button->connectOnPress([](){ ApplicationCore::log(ApplicationCore::LogLevel::OK, "Button pressed!"); });
	button->connectOnHover([](){ ApplicationCore::log(ApplicationCore::LogLevel::OK, "Button hovered!"); });

	auto textured_button = std::make_unique<CloseButton>();
	textured_button->setBox(BoxF { 60, 60, 200, 100 });
	textured_button->connectOnPress([](){ ApplicationCore::instance().stop(); });

	blank_window->addChild(std::move(button));
	blank_window->addChild(std::move(textured_button));

	IDesktop *desktop = nullptr;
	ApplicationCore::Error error = ApplicationCore::instance().queryInterface(IID_DESKTOP, reinterpret_cast<void **>(&desktop));

	if(error != ApplicationCore::Error::SUCCESS)
	{
		ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "desktop requesting failed with error: %s", ApplicationCore::strError(error));
		return false;
	}

	desktop->add(std::move(blank_window));

	return true;
}

