#ifndef I_INPUT_PUMP_HPP
#define I_INPUT_PUMP_HPP

#include "Interfaces/IID.hpp"
#include "Utils/Event.hpp"

#include <optional>

constexpr static IID IID_INPUT_PUMP{1};

class IInputPump
{
public:
    virtual ~IInputPump() = 0;

    virtual std::optional<Event> pollEvent() = 0;

protected:
    IInputPump();
};

/* ======================== Implementation ======================== */

inline IInputPump::IInputPump()  = default;
inline IInputPump::~IInputPump() = default;

#endif
