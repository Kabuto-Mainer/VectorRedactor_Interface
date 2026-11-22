#pragma once

#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Signal.hpp"
#include "Utils/TWidget.hpp"

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

		void render() override final;

	private:
		Icon icon_;
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

}

