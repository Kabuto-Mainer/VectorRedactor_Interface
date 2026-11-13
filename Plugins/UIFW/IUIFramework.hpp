#ifndef I_UI_FRAMEWORK_HPP
#define I_UI_FRAMEWORK_HPP

#include "Interfaces/IID.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Utils/ACommand.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/TWidget.hpp"
#include "Utils/Vec2F.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

constexpr IID IID_IUIFramework = 0x00'06;

class IUIFramework
{
public:
    struct Action;

    struct ButtonDescription;
    struct SwitcherDescription;
    struct ContextMenuDescription;
    struct ButtonPoolDescription;
    struct SwitcherPoolDescription;
    struct WindowMenuDescription;

    class Button;
    class Switcher;
    class ContextMenu;
    class ButtonPool;
    class SwitcherPool;
    class WindowMenu;

    enum class Direction;
    enum class Role;
    using EventHandler = std::function<bool(const IInputPump::Event&)>;

    virtual ~IUIFramework() = default;

    virtual std::unique_ptr<Button>       create(ButtonDescription& description)               = 0;
    virtual std::unique_ptr<Switcher>     create(SwitcherDescription& description)             = 0;
    virtual std::unique_ptr<ContextMenu>  create(ContextMenuDescription& description)          = 0;
    virtual std::unique_ptr<ButtonPool>   create(ButtonPoolDescription& description)           = 0;
    virtual std::unique_ptr<SwitcherPool> create(SwitcherPoolDescription& description)         = 0;
    virtual std::unique_ptr<WindowMenu>   create(WindowMenuDescription& description)           = 0;
    virtual void registryTheme(const std::string& name, const IRenderer::Color& primary,
                               const IRenderer::Color& secondary, const IRenderer::Color& neutral,
                               const IRenderer::Color& accent, const IRenderer::Color& unical) = 0;
    virtual bool setTheme(const std::string& name)                                             = 0;
    virtual bool setFont(const std::string& path)                                              = 0;
}; // class IUIFramework

enum class IUIFramework::Direction
{
    VERTICAL,
    HORIZONTAL,
    MAX,
};

enum class IUIFramework::Role
{
    DEFAULT,
    PRIMARY,
    DANGER,
    MAX,
};

struct IUIFramework::Action
{
    std::vector<std::unique_ptr<ACommand>> actions;
    std::vector<std::function<void(void)>> callbacks;
}; // struct Action

class IUIFramework::Button : public TWidget
{
}; // class Button

class IUIFramework::Switcher : public TWidget
{
}; // class Switcher

class IUIFramework::ContextMenu : public TWidget
{
public:
    virtual void add(std::unique_ptr<Button> button)     = 0;
    virtual void add(std::unique_ptr<Switcher> switcher) = 0;
}; // class ContextMenu

class IUIFramework::ButtonPool : public TWidget
{
public:
    virtual void add(std::unique_ptr<Button> button) = 0;
}; // class ButtonPool

class IUIFramework::SwitcherPool : public TWidget
{
public:
    virtual void add(std::unique_ptr<Switcher> switcher) = 0;
}; // class SwitcherPool

class IUIFramework::WindowMenu : public TWidget
{
public:
    virtual void add(std::unique_ptr<Button> button)     = 0;
    virtual void add(std::unique_ptr<Switcher> switcher) = 0;
    virtual void add(std::unique_ptr<ContextMenu> menu)  = 0;
    virtual void add(std::unique_ptr<ButtonPool> pool)   = 0;
    virtual void add(std::unique_ptr<SwitcherPool> pool) = 0;
}; // class WindowMenu

struct IUIFramework::ButtonDescription
{
    BoxF                       box;
    std::optional<std::string> text;
    std::optional<std::string> icon;
    Action                     on_press;
    Role                       role = Role::DEFAULT;
}; // struct ButtonDescription

struct IUIFramework::SwitcherDescription
{
    BoxF                       box;
    std::optional<std::string> text;
    std::optional<std::string> icon;
    Action                     on_active;
    Action                     on_deactive;
    Role                       role = Role::DEFAULT;
}; // struct SwitcherDescription

struct IUIFramework::ContextMenuDescription
{
    Vec2F                       min_point;
    Direction                   direction = Direction::VERTICAL;
    std::optional<EventHandler> on_event;
    Role                        role = Role::DEFAULT;
}; // struct ContextMenuDescription

struct IUIFramework::ButtonPoolDescription
{
    Vec2F     min_point;
    Vec2F     size;
    Direction direction = Direction::VERTICAL;
    Role      role      = Role::DEFAULT;
}; // struct ButtonPoolDescription

struct IUIFramework::SwitcherPoolDescription
{
    Vec2F     min_point;
    Vec2F     size;
    Direction direction = Direction::VERTICAL;
    Role      role      = Role::DEFAULT;
}; // struct SwitcherPoolDescription

struct IUIFramework::WindowMenuDescription
{
    BoxF                        box;
    std::optional<EventHandler> on_event;
    std::optional<Action>       on_close;
    std::optional<std::string>  label;
    Role                        role = Role::DEFAULT;
}; // struct WindowMenuDescription

#endif // I_UI_FRAMEWORK_HPP
