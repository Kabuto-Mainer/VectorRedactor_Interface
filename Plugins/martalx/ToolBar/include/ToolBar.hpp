#pragma once

#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolBar.hpp"
#include "UIFW/UIFW.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/TObject.hpp"

namespace MartAlx
{

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

class ToolBar::Node final : public UIFW::BlankWindow
{
	private:
		static constexpr auto BACKGROUND = IRenderer::Color::GRAY;
		// ISSUE: constexpr Vec2F, BoxF
		inline static const auto TOP_LEFT_BUTTON_SIZE = BoxF { 0, 0, 50.0F, 50.0F };

		ID switcher_id_;

	public:
		Node();

		bool processEvent(const IInputPump::Event& event) override;

		void addToolButton(IToolManager::ATool::ID tool_id, IRenderer::TextureID texture_id);
};

}

