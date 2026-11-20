#pragma once

#include "Interfaces/IToolManager.hpp"
#include "Interfaces/IInputPump.hpp"

static constexpr IToolManager::ATool::ID ID_TOOL_RECTANGLE = 0;

class RectangleTool final : public IToolManager::ATool
{
	private:
		inline static const Vec2F INITIAL_SIZE = { 100.0F, 100.0F };

	public:
		RectangleTool();

		ID id() const override;
		bool processEvent(const IInputPump::Event& event) override;
};

//class EllipseTool final : public IToolManager::ATool
//{
//	private:
//		static constexpr ID TOOL_ID = 1;
//
//	public:
//		ID id() const override;
//		bool processEvent(const IInputPump::Event& event) override;
//};

