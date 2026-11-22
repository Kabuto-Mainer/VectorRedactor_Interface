#include "UIFW.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <variant>

using namespace MartAlx::UIFW;

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

BlankWindow::BlankWindow(const BoxF &box)
{
	setBox(box);
}

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

Button::Button(const BoxF &box)
{
	setBox(box);
}

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
						/* NOTE:
						 * If the button has just been hovered,
						 * the previously hovered button should
						 * go back to State::NORMAL, so it's
						 * necessary to return false in this case
						 */
						return false;

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

StandardButton::StandardButton(const BoxF &box, const Icon &icon)
	: icon_(icon)
{
	setBox(box);
}

StandardButton::StandardButton(const Icon &icon)
	: StandardButton(BoxF {}, icon)
{ }

void StandardButton::render()
{
	static constexpr auto NORMAL_TONE  = IRenderer::Color { .r = 0,   .g = 0,   .b = 0,   .a = 0   };
	static constexpr auto HOVERED_TONE = IRenderer::Color { .r = 255, .g = 255, .b = 0,   .a = 50  };
	static constexpr auto PRESSED_TONE = IRenderer::Color { .r = 255, .g = 0,   .b = 0,   .a = 50  };

	IRenderer::Color tone = IRenderer::Color::TRANSPARENT;
	switch(state())
	{
		case State::NORMAL:  tone = NORMAL_TONE;  break;
		case State::HOVERED: tone = HOVERED_TONE; break;
		case State::PRESSED: tone = PRESSED_TONE; break;

		default:
				     assert(0 && "Unreachable");
	}

	icon_.render(::globalBox(*this));

	IRenderer *renderer = nullptr;
	ApplicationCore::Error error = ApplicationCore::instance().queryInterface(IID_RENDERER, reinterpret_cast<void **>(&renderer));
	assert(error == ApplicationCore::Error::SUCCESS);

	renderer->renderRect(tone, ::globalBox(*this));
}

/* ToggleButton */

ToggleButton::ToggleButton(const BoxF &box, const Icon &icon)
	: StandardButton(icon), is_toggled_([](){ return false; })
{
	setBox(box);
}

ToggleButton::ToggleButton(const Icon &icon)
	: ToggleButton(BoxF {}, icon)
{ }

void ToggleButton::connectIsToggled(std::function<bool ()> callback)
{
	is_toggled_ = callback;
}

void ToggleButton::render()
{
	static constexpr auto TOGGLED_TONE = IRenderer::Color { .r = 0, .g = 0, .b = 255, .a = 50 };

	if(is_toggled_())
	{
		IRenderer *renderer = nullptr;
		ApplicationCore::Error error = ApplicationCore::instance().queryInterface(IID_RENDERER, reinterpret_cast<void **>(&renderer));
		assert(error == ApplicationCore::Error::SUCCESS);

		renderer->renderRect(TOGGLED_TONE, ::globalBox(*this));
	}

	StandardButton::render();
}

/* ASwitcher */

void ASwitcher::connectOnSwitch(std::function<void (ID)> callback)
{
	on_switch_.subscribe(callback);
}

void ASwitcher::render()
{
	renderChildren();
}

bool ASwitcher::processEvent(const IInputPump::Event &event)
{
	return processEventChildren(event);
}

/* HorizontalSwitcher */

HorizontalSwitcher::HorizontalSwitcher(const BoxF &top_left_button_box)
	: button_size_(top_left_button_box.size())
{
	setBox(BoxF { top_left_button_box.topLeft(), top_left_button_box.bottomLeft() });
}

Button::ID HorizontalSwitcher::addButton(const Icon &icon)
{
	auto button = std::make_unique<ToggleButton>(BoxF { box().topRight(), box().topRight() + button_size_ }, icon);
	const auto button_id = button->id();

	auto new_box = box();
	new_box.setRight(box().right() + button_size_.x());
	setBox(std::move(new_box));

	auto on_press = [this, button_id]() -> void
	{
		toggled_button_ = button_id;
		on_switch_(button_id);
	};
	button->connectOnPress(on_press);

	auto is_toggled = [this, button_id]() -> bool
	{
		return button_id == toggled_button_;
	};
	button->connectIsToggled(is_toggled);

	addChild(std::move(button));
	return button_id;
}

/* VerticalSwitcher */

VerticalSwitcher::VerticalSwitcher(const BoxF &top_left_button_box)
	: button_size_(top_left_button_box.size())
{
	setBox(BoxF { top_left_button_box.topLeft(), top_left_button_box.topRight() });
}

Button::ID VerticalSwitcher::addButton(const Icon &icon)
{
	auto button = std::make_unique<ToggleButton>(BoxF { box().bottomLeft(), box().bottomLeft() + button_size_ }, icon);
	const auto button_id = button->id();

	auto new_box = box();
	new_box.setBottom(box().bottom() + button_size_.y());
	setBox(std::move(new_box));

	auto on_press = [this, button_id]() -> void
	{
		toggled_button_ = button_id;
		on_switch_(button_id);
	};
	button->connectOnPress(on_press);

	auto is_toggled = [this, button_id]() -> bool
	{
		return button_id == toggled_button_;
	};
	button->connectIsToggled(is_toggled);

	addChild(std::move(button));
	return button_id;
}

/* TexturedButton */

TexturedButton::TexturedButton(const BoxF &box, const Icons &icons)
	: icons_(icons)
{
	setBox(box);
}

TexturedButton::TexturedButton(const Icons& icons)
	: TexturedButton(BoxF {}, icons)
{ }

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

//CloseableWindow::CloseableWindow()
//{
//	auto close_button = std::make_unique<StandardButton>(Icon(CLOSE_BUTTON_FILENAME));
//
//}

