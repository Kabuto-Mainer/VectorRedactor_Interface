#pragma once

#include "ATool.hpp"

namespace MartAlx::Tools
{

inline constexpr ATool::ID ID_BROWSE = 100;

class Browse final : public ATool
{
	public:
		ID id() const override;
		bool processEvent(const IInputPump::Event &event) override;
};

}

