#pragma once

#include "Interfaces/IInputPump.hpp"

#include <deque>
#include <optional>

class RaylibInputPump final : public IInputPump
{
public:
    static RaylibInputPump& instance();

    RaylibInputPump(const RaylibInputPump&) = delete;
    RaylibInputPump(RaylibInputPump&&) = delete;
    RaylibInputPump& operator=(const RaylibInputPump&) = delete;
    RaylibInputPump& operator=(RaylibInputPump&&) = delete;

    std::optional<Event> pollEvent() override;

private:
    RaylibInputPump() = default;
    void collectEvents();

    std::deque<Event> events_;
    Vec2F previous_mouse_pos_{};
    bool quit_emitted_ = false;
};
