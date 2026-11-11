#ifndef IINPUT_PUMP_HPP
#define IINPUT_PUMP_HPP

#include "Interfaces/IID.hpp"
#include "Utils/Event.hpp"

#include <optional>

constexpr static const IID IID_INPUT_PUMP = 1;

class IInputPump
{
public:
    IInputPump()          = default;
    virtual ~IInputPump() = default;

    virtual std::optional<Event> pollEvent() = 0;
};

#endif
