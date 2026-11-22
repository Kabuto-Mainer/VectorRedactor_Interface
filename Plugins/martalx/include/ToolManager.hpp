#pragma once

#include "Interfaces/IToolManager.hpp"
#include <unordered_map>
#include <memory>

class ToolManager final : public IToolManager
{
	private:
		ToolManager() = default;

		std::unordered_map<ATool::ID, std::unique_ptr<ATool>> tools_;
		std::optional<ATool::ID> active_tool_;

	public:
		ToolManager(const ToolManager &) = delete;
		ToolManager(ToolManager &&) = delete;
		ToolManager &operator=(const ToolManager &) = delete;
		ToolManager &operator=(ToolManager &&) = delete;

		static ToolManager &instance();

		Error addTool(std::unique_ptr<ATool> tool) override;
		Error removeTool(ATool::ID tool_id) override;
		Error setActiveTool(ATool::ID tool_id) override;

		bool processEvent(const IInputPump::Event& event) override;
};

