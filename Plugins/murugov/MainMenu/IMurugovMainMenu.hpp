#pragma once

#include "Interfaces/IID.hpp"

#include <functional>
#include <string>


namespace Murugov
{
    
constexpr static const IID IID_MAIN_MENU = 7;

class IMainMenu
{
public:
    using ActionCallback = std::function<void()>;

    IMainMenu()          = default;
    virtual ~IMainMenu() = default;

    IMainMenu(const IMainMenu&)            = delete;
    IMainMenu(IMainMenu&&)                 = delete;
    IMainMenu& operator=(const IMainMenu&) = delete;
    IMainMenu& operator=(IMainMenu&&)      = delete;

    virtual void addMenuItem(const std::string& category, const std::string& title, ActionCallback callback) = 0;
}; // IMainMenu

} // namespace Murugov