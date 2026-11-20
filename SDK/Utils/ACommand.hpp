#pragma once

#include <memory>

class ACommand
{
public:
    virtual ~ACommand() = default;

    virtual void execute() const = 0;
    virtual std::unique_ptr<ACommand> clone() const;
};
