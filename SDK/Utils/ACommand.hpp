#ifndef A_COMMAND_HPP
#define A_COMMAND_HPP

class ACommand
{
public:
    virtual ~ACommand() = 0;

    virtual void execute() const = 0;

protected:
    ACommand();
};

/* ======================== Implementation ======================== */

inline ACommand::ACommand()  = default;
inline ACommand::~ACommand() = default;

#endif
