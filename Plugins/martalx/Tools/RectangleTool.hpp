#pragma once

#include "ATool.hpp"
#include "Interfaces/IInputPump.hpp"

namespace MartAlx::Tools
{

inline constexpr ATool::ID ID_RECTANGLE = 0;

class Rectangle final : public ATool
{
	private:
		inline static const Vec2F INITIAL_SIZE = { 100.0F, 100.0F };

	public:
		ID id() const override;
		bool processEvent(const IInputPump::Event& event) override;
};

}

