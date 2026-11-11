#ifndef I_TOOL_HPP
#define I_TOOL_HPP

#include "Interfaces/IID.hpp"
#include "Utils/Event.hpp"

constexpr static const IID IID_TOOL = 2;

using ToolID = uint64_t;

class ITool
{
public:
    ITool()          = default;
    virtual ~ITool() = default;

    ITool(const ITool&)            = delete;
    ITool(ITool&&)                 = delete;
    ITool& operator=(const ITool&) = delete;
    ITool& operator=(ITool&&)      = delete;

    virtual ToolID ID() const                       = 0;
    virtual bool   processEvent(const Event& event) = 0;
};

#endif
