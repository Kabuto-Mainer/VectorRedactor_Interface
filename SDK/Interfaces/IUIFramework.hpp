#ifndef I_UI_FRAMEWORK_HPP
#define I_UI_FRAMEWORK_HPP

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class ACommand;
class AObject;
struct Event;

#include "../Utils/BoxF.hpp"
#include "../Utils/Vec2F.hpp"
#include "IPlugin.hpp"
#include "IRenderer.hpp"

constexpr IID IID_IUIFramework = 0x00'06;

namespace UI
{

class ADescription;

class ADescriptionVisitor
{
public:
    ~ADescriptionVisitor() = default;

    virtual bool doForButton(ADescription& button)      = 0;
    virtual bool doForSwitcher(ADescription& button)    = 0;
    virtual bool doForContextMenu(ADescription& button) = 0;
    virtual bool doForPoolMenu(ADescription& button)    = 0;
    virtual bool doForWindow(ADescription& button)      = 0;
}; // class ADescriptionVisitor

class ADescription
{
public:
    virtual bool accept(ADescriptionVisitor& visitor) = 0;

    ADescription()          = default;
    virtual ~ADescription() = default;

    ADescription(const ADescription&)            = delete;
    ADescription(ADescription&&)                 = delete;
    ADescription& operator=(const ADescription&) = delete;
    ADescription& operator=(ADescription&&)      = delete;
}; // class ADescription

struct Action
{
    std::vector<std::unique_ptr<ACommand>> actions;
    std::vector<std::function<void(void)>> callbacks;
}; // struct Action

using EventHandler = std::function<bool(const Event&)>;

enum class Direct
{
    Vertical,
    Horizontal,
};

enum class Role
{
    Default,
    Primary,
    Danger,
    MAX,
};

struct ButtonDescription : public ADescription
{
    BoxF                       box;
    std::optional<std::string> text;
    std::optional<std::string> icon;
    Action                     on_press;
    std::optional<Role>        role;

    bool accept(ADescriptionVisitor& visitor) override
    {
        return visitor.doForButton(*this);
    }
}; // struct ButtonDescription

struct SwitcherDescription : public ADescription
{
    BoxF                       box;
    std::optional<std::string> text;
    std::optional<std::string> icon;
    Action                     on_active;
    Action                     on_deactive;
    std::optional<Role>        role;

    bool accept(ADescriptionVisitor& visitor) override
    {
        return visitor.doForSwitcher(*this);
    }
}; // struct SwitcherDescription

struct ContextMenu : public ADescription
{
    Vec2F                                      min_point;
    Direct                                     direct;
    std::vector<std::unique_ptr<ADescription>> objects;
    std::optional<EventHandler>                on_event;
    std::optional<Role>                        role;

    bool accept(ADescriptionVisitor& visitor) override
    {
        return visitor.doForContextMenu(*this);
    }
}; // struct ContextMenu

struct PoolMenu : public ADescription
{
    Vec2F                                      min_point;
    Vec2F                                      size;
    Direct                                     direct;
    std::vector<std::unique_ptr<ADescription>> objects;
    std::optional<Role>                        role;

    bool accept(ADescriptionVisitor& visitor) override
    {
        return visitor.doForPoolMenu(*this);
    }
}; // struct PoolMenu

struct Window : public ADescription
{
    BoxF                                       box;
    std::optional<EventHandler>                on_event;
    std::optional<Action>                      on_close;
    std::optional<std::string>                 label;
    std::vector<std::unique_ptr<ADescription>> objects;
    std::optional<Role>                        role;

    bool accept(ADescriptionVisitor& visitor) override
    {
        return visitor.doForWindow(*this);
    }
}; // struct Window

class IFramework : public IPlugin
{
public:
    virtual std::unique_ptr<AObject> create(ADescription& description)                               = 0;
    virtual void registryTheme(std::string name, const Color_t& primary, const Color_t& secondary,
                               const Color_t& neutral, const Color_t& accent, const Color_t& unical) = 0;
    virtual bool setTheme(std::string name)                                                          = 0;
    virtual bool setFont(std::string path)                                                           = 0;
}; // class IFramework

} // namespace UI

#endif // I_UI_FRAMEWORK_HPP
