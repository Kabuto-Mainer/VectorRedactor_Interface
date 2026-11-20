#pragma once

#include <memory>

class ACommand
{
public:
    virtual ~ACommand() = default;

    virtual void execute() const = 0;

    template<typename Command>
    static std::unique_ptr<Command> clone(const Command& command);
};

/* ===================== Implementation ===================== */

template<typename Command>
std::unique_ptr<Command> ACommand::clone(const Command& command)
{
    return std::make_unique<Command>(command);
}
