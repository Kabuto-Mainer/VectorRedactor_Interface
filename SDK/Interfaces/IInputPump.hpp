#ifndef I_INPUT_PUMP_HPP
#define I_INPUT_PUMP_HPP

struct Event;

class IInputPump
{
public:
    virtual const Event* pollEvent() = 0;
}; // class IInputPump

#endif
