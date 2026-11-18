#pragma once

#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Signal.hpp"
#include "Utils/TWidget.hpp"

#include <functional>
#include <string>

class Icon
{
	public:
		explicit Icon(const std::string& filename);
		void render(const BoxF& box) const;

	private:
		IRenderer::TextureID texture_;
};

class BlankWindow : public TWidget
{
	public:
		void setBackground(const IRenderer::Color& background);

		bool processEvent(const IInputPump::Event& event) override;
		void render() override;

	private:
		IRenderer::Color background_ = IRenderer::Color::TRANSPARENT;
};

class CloseableWindow : public BlankWindow
{
	public:
		CloseableWindow();
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

		virtual ~Button() = default;

		State state() const;
		void  connectOnPress(std::function<void()> callback);

		void render() override = 0;
		bool processEvent(const IInputPump::Event& event) override final;

	private:
		State    state_ = State::NORMAL;
		Signal<> on_press_;
};

class StandardButton : public Button
{
	public:
		void render() override final;
};

class TexturedButton : public Button
{
	public:
		struct Icons
		{
			Icon normal, hovered, pressed;
		};

		explicit TexturedButton(const Icons& icons);

		void render() override final;

	private:
		Icons icons_;
};

class CloseButton final : public TexturedButton
{
	public:
		CloseButton();

	private:
		static constexpr struct
		{
			const char *normal, *hovered, *pressed;
		}
		TEXTURE_FILENAMES
		{
			// FIXME: specify real files to load
			.normal  = "",
			.hovered = "",
			.pressed = "",
		};
};
