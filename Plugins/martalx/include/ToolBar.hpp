#pragma once

#include "Interfaces/IID.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolBar.hpp"
#include "UIFramework.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"

static constexpr IID IID_TOOL_BAR = 30;

class ToolBar final : public IToolBar
{
	private:
		ToolBar();

		class Node;
		TObject::ID node_id_;

	public:
		ToolBar(const ToolBar &) = delete;
		ToolBar(ToolBar &&) = delete;
		ToolBar &operator=(const ToolBar &) = delete;
		ToolBar &operator=(ToolBar &&) = delete;

		static ToolBar &instance();

		void addTool(IToolManager::ATool::ID tool_id, IRenderer::TextureID texture_id) override;
		void removeTool(IToolManager::ATool::ID tool_id) override;
};

class ToolBar::Node final : public BlankWindow
{
	private:
		static constexpr IRenderer::Color BACKGROUND = IRenderer::Color::GRAY;
		// ISSUE: constexpr Vec2F
		inline static const Vec2F BUTTON_SIZE = { 50.0F, 50.0F };
		inline static const Vec2F TOOLBAR_SIZE = { 50.0F, 800.0F };

		Vec2F next_button_pos_ = BUTTON_SIZE / 2;

	public:
		Node();

		bool processEvent(const IInputPump::Event& event) override;

		void addToolButton(IToolManager::ATool::ID tool_id, IRenderer::TextureID texture_id);
};

