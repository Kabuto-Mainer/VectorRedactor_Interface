#include "UIFramework.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Utils/TObject.hpp"

#include <cassert>
#include <string>
#include <variant>

// TODO: mapLocalToGlobal etc move to protected,
// add func below to box class
static BoxF globalBox(const TWidget &object)
{
	return BoxF { object.mapLocalToGlobal(Vec2F {}), object.mapLocalToGlobal(object.box().size()) };
}

/* Icon */

Icon::Icon(IRenderer::TextureID texture)
	: texture_(texture)
{ }

Icon::Icon(const std::string& filename)
{
	IRenderer*             renderer = nullptr;
	ApplicationCore::Error error =
		ApplicationCore::instance().queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer));
	assert(error == ApplicationCore::Error::SUCCESS);

	auto texture = renderer->loadTexture(filename);
	if (!texture.has_value())
	{
		ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Failed to load texture");
		return;
	}

	texture_ = *texture;
}

IRenderer::TextureID Icon::texture() const
{
	return texture_;
}

void Icon::render(const BoxF& box) const
{
	IRenderer*             renderer = nullptr;
	ApplicationCore::Error error =
		ApplicationCore::instance().queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer));
	assert(error == ApplicationCore::Error::SUCCESS);

	renderer->renderTexture(texture_, box);
}

/* BlankWindow */

void BlankWindow::setBackground(const IRenderer::Color& background)
{
	background_ = background;
}

bool BlankWindow::processEvent(const IInputPump::Event& event)
{
	return processEventChildren(event);
}

void BlankWindow::render()
{
	IRenderer*             renderer = nullptr;
	ApplicationCore::Error error =
		ApplicationCore::instance().queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer));
	assert(error == ApplicationCore::Error::SUCCESS);

	BoxF global_box = ::globalBox(*this);
	renderer->renderRect(background_, global_box);
	renderChildren();
}

/* Button */

Button::State Button::state() const
{
	return state_;
}

bool Button::processEvent(const IInputPump::Event& event)
{
	if (processEventChildren(event))
	{
		return true;
	}

	if (!std::holds_alternative<IInputPump::EventMouse>(event))
	{
		return false;
	}

	const auto &event_mouse = std::get<IInputPump::EventMouse>(event);

	// TODO: yack!
	if (box().contains(box().topLeft() + mapGlobalToLocal(event_mouse.cursorPos())))
	{
		switch (event_mouse.type())
		{
			case IInputPump::EventMouse::Type::LEFT_BUTTON_PRESSING:
				switch (state_)
				{
					case State::NORMAL:
					case State::HOVERED:
						state_ = State::PRESSED;
						break;

					case State::PRESSED:
					default:
						assert(0 && "Unreachable");
				}
				break;

			case IInputPump::EventMouse::Type::LEFT_BUTTON_RELEASING:
				switch (state_)
				{
					case State::PRESSED:
						on_press_();
						[[fallthrough]];
					case State::NORMAL:
						state_ = State::HOVERED;
						break;

					case State::HOVERED:
						break;

					default:
						assert(0 && "Unreachable");
				}
				break;

			case IInputPump::EventMouse::Type::MOTION:
				switch (state_)
				{
					case State::NORMAL:
						state_ = State::HOVERED;
						on_hover_();
						break;

					case State::PRESSED:
					case State::HOVERED:
						break;

					default:
						assert(0 && "Unreachable");
				}
				break;

			default:
				break;
		}

		return true;
	}

	state_ = State::NORMAL;
	return false;
}

void Button::connectOnPress(std::function<void()> callback)
{
	on_press_.subscribe(callback);
}

void Button::connectOnHover(std::function<void ()> callback)
{
	on_hover_.subscribe(callback);
}

/* StandardButton */

void StandardButton::render()
{
	IRenderer::Color color = IRenderer::Color::TRANSPARENT;
	switch (state())
	{
		case State::NORMAL:
			color = IRenderer::Color::GRAY;
			break;
		case State::HOVERED:
			color = IRenderer::Color::YELLOW;
			break;
		case State::PRESSED:
			color = IRenderer::Color::RED;
			break;
	}

	IRenderer*             renderer = nullptr;
	ApplicationCore::Error error =
		ApplicationCore::instance().queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer));
	assert(error == ApplicationCore::Error::SUCCESS);

	renderer->renderRect(color, ::globalBox(*this));
	renderChildren();
}

/* TexturedButton */

TexturedButton::TexturedButton(const Icons& icons) : icons_(icons)
{
}

void TexturedButton::render()
{
	IRenderer*             renderer = nullptr;
	ApplicationCore::Error error =
		ApplicationCore::instance().queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer));
	assert(error == ApplicationCore::Error::SUCCESS);

	switch (state())
	{
		case State::NORMAL:
			icons_.normal.render(::globalBox(*this));
			break;
		case State::HOVERED:
			icons_.hovered.render(::globalBox(*this));
			break;
		case State::PRESSED:
			icons_.pressed.render(::globalBox(*this));
			break;

		default:
			assert(0 && "Unreachable");
	}

	renderChildren();
}

/* CloseButton */
CloseButton::CloseButton() :
	TexturedButton(Icons{
			.normal  = Icon(TEXTURE_FILENAMES.normal),
			.hovered = Icon(TEXTURE_FILENAMES.hovered),
			.pressed = Icon(TEXTURE_FILENAMES.pressed),
			})
{ }


