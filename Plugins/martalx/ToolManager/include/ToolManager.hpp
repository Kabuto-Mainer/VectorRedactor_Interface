#pragma once

#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Utils/TObject.hpp"
#include <unordered_map>
#include <memory>

namespace MartAlx
{

class ToolManager final : public IToolManager
{
	private:
		ToolManager();

		std::unordered_map<ATool::ID, std::unique_ptr<ATool>> tools_;
		std::optional<ATool::ID> active_tool_;

		class Node;

	public:
		ToolManager(const ToolManager &) = delete;
		ToolManager(ToolManager &&) = delete;
		ToolManager &operator=(const ToolManager &) = delete;
		ToolManager &operator=(ToolManager &&) = delete;

		static ToolManager &instance();

		Error addTool(std::unique_ptr<ATool> tool) override;
		/* NOTE: used as `unsetActiveTool` */
		Error removeTool(ATool::ID tool_id) override;
		Error setActiveTool(ATool::ID tool_id) override;

		bool processEvent(const IInputPump::Event& event) override;
};

class ToolManager::Node final : public TObject
{
	public:
		void render() override;
		bool processEvent(const IInputPump::Event &event) override;

		/* NOTE: stubs */
		Vec2F mapLocalToGlobal(const Vec2F& pos) const override;
		Vec2F mapGlobalToLocal(const Vec2F& pos) const override;
};

}

