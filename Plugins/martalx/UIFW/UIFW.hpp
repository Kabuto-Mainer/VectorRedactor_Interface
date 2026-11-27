#pragma once

#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Signal.hpp"
#include "Utils/TWidget.hpp"
#include "Utils/Vec2F.hpp"

#include <functional>
#include <string>

namespace MartAlx::UIFW
{

class Icon
{
	public:
		explicit Icon(IRenderer::TextureID texture);
		explicit Icon(const std::string& filename);

		IRenderer::TextureID texture() const;
		void render(const BoxF& box) const;

	private:
		IRenderer::TextureID texture_;
};

class BlankWindow : public TWidget
{
	public:
		BlankWindow() = default;
		explicit BlankWindow(const BoxF &box);

		void setBackground(const IRenderer::Color& background);

		bool processEvent(const IInputPump::Event& event) override;
		void render() override;

	private:
		IRenderer::Color background_ = IRenderer::Color::TRANSPARENT;
};

class Button : public TWidget
{
	public:
		enum class State
		{
			NORMAL,
			HOVERED,
			PRESSED,
		};

		Button() = default;
		explicit Button(const BoxF &box);

		State state() const;
		void connectOnPress(std::function<void()> callback);
		void connectOnHover(std::function<void()> callback);

		void render() override = 0;
		bool processEvent(const IInputPump::Event& event) override final;

	private:
		State    state_ = State::NORMAL;
		Signal<> on_press_;
		Signal<> on_hover_;
};

class StandardButton : public Button
{
	public:
		explicit StandardButton(const Icon &icon);
		StandardButton(const BoxF &box, const Icon &icon);

		void render() override;

	private:
		Icon icon_;
};

/* NOTE:
 * ToggleButton is a button that can have another, toggle state
 * that does mean that the button is selected now.
 */
class ToggleButton : public StandardButton
{
	public:
		explicit ToggleButton(const Icon &icon);
		ToggleButton(const BoxF &box, const Icon &icon);

		void connectIsToggled(std::function<bool()> callback);
		void render() override;

	private:
		std::function<bool()> is_toggled_;
};

class ASwitcher : public TWidget
{
	protected:
		Signal<ID> on_switch_;

	public:
		virtual ID addButton(const Icon &icon) = 0;
		void connectOnSwitch(std::function<void(ID)> callback);

		void render() override final;
		bool processEvent(const IInputPump::Event &event) override final;
};

class HorizontalSwitcher : public ASwitcher
{
	private:
		const Vec2F button_size_;
		ID toggled_button_;

	public:
		explicit HorizontalSwitcher(const BoxF &top_left_button_box);

		ID addButton(const Icon &icon) override final;
};

class VerticalSwitcher : public ASwitcher
{
	private:
		const Vec2F button_size_;
		ID toggled_button_;

	public:
		explicit VerticalSwitcher(const BoxF &top_left_button_box);

		ID addButton(const Icon &icon) override final;
};

class TexturedButton : public Button
{
	public:
		struct Icons
		{
			Icon normal, hovered, pressed;
		};

		explicit TexturedButton(const Icons& icons);
		TexturedButton(const BoxF &box, const Icons &icons);

		void render() override final;

	private:
		Icons icons_;
};

//class CloseableWindow : public BlankWindow
//{
//	private:
//		static constexpr const char *CLOSE_BUTTON_FILENAME = "./images/close.png";
//		static constexpr auto CLOSE_BUTTON_SIZE = Vec2F {  }
//
//	public:
//		CloseableWindow();
//};

}

