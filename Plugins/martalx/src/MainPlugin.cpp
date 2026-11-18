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
	blank_window->setBox(BoxF { 0, 0, 100, 100 });
	blank_window->setBackground(IRenderer::Color::RED);

	auto button = std::make_unique<StandardButton>();
	button->setBox(BoxF { 5, 5, 50, 50 });
	blank_window->addChild(std::move(button));

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

