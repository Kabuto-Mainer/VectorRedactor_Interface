#pragma once

#include "Interfaces/IID.hpp"

#include <string>

namespace Murugov
{

constexpr static const IID IID_STATUS_BAR = 8;

class IStatusBar
{
public:
    IStatusBar()          = default;
    virtual ~IStatusBar() = default;

    IStatusBar(const IStatusBar&)            = delete;
    IStatusBar(IStatusBar&&)                 = delete;
    IStatusBar& operator=(const IStatusBar&) = delete;
    IStatusBar& operator=(IStatusBar&&)      = delete;

    virtual void setStatusText(const std::string& text) = 0;
    virtual void setActiveToolName(const std::string& tool_name) = 0;
}; // IStatusBar

} // namespace Murugov
