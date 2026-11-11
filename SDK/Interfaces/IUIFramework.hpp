#ifndef I_UI_FRAMEWORK_HPP
#define I_UI_FRAMEWORK_HPP

#include "Interfaces/IID.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Event.hpp"
#include "Utils/Vec2F.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class ACommand;
class AObject;

constexpr IID IID_IUIFramework = 0x00'06;

namespace UI
{

class ADescription;

class ADescriptionVisitor
{
public:
    ~ADescriptionVisitor() = default;

    virtual bool visitButton(ADescription& button)      = 0;
    virtual bool visitSwitcher(ADescription& button)    = 0;
    virtual bool visitContextMenu(ADescription& button) = 0;
    virtual bool visitPoolMenu(ADescription& button)    = 0;
    virtual bool visitWindow(ADescription& button)      = 0;
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

enum class Direction
{
    VERTICAL,
    HORIZONTAL,
    MAX,
};

enum class Role
{
    DEFAULT,
    PRIMARY,
    DANGER,
    MAX,
};

struct ButtonDescription : public ADescription
{
    BoxF                       box;
    std::optional<std::string> text;
    std::optional<std::string> icon;
    Action                     on_press;
    Role                       role = Role::DEFAULT;

    bool accept(ADescriptionVisitor& visitor) override
    {
        return visitor.visitButton(*this);
    }
}; // struct ButtonDescription

struct SwitcherDescription : public ADescription
{
    BoxF                       box;
    std::optional<std::string> text;
    std::optional<std::string> icon;
    Action                     on_active;
    Action                     on_deactive;
    Role                       role = Role::DEFAULT;

    bool accept(ADescriptionVisitor& visitor) override
    {
        return visitor.visitSwitcher(*this);
    }
}; // struct SwitcherDescription

struct ContextMenuDescription : public ADescription
{
    Vec2F                                      min_point;
    Direction                                  direction = Direction::VERTICAL;
    std::vector<std::unique_ptr<ADescription>> objects;
    std::optional<EventHandler>                on_event;
    Role                                       role = Role::DEFAULT;

    bool accept(ADescriptionVisitor& visitor) override
    {
        return visitor.visitContextMenu(*this);
    }
}; // struct ContextMenu

struct PoolMenuDescription : public ADescription
{
    Vec2F                                      min_point;
    Vec2F                                      size;
    Direction                                  direction = Direction::VERTICAL;
    std::vector<std::unique_ptr<ADescription>> objects;
    Role                                       role = Role::DEFAULT;

    bool accept(ADescriptionVisitor& visitor) override
    {
        return visitor.visitPoolMenu(*this);
    }
}; // struct PoolMenu

struct WindowDescription : public ADescription
{
    BoxF                                       box;
    std::optional<EventHandler>                on_event;
    std::optional<Action>                      on_close;
    std::optional<std::string>                 label;
    std::vector<std::unique_ptr<ADescription>> objects;
    Role                                       role = Role::DEFAULT;

    bool accept(ADescriptionVisitor& visitor) override
    {
        return visitor.visitWindow(*this);
    }
}; // struct Window

class IFramework
{
public:
    virtual ~IFramework() = default;

    virtual std::unique_ptr<AObject> create(ADescription& description)                                             = 0;
    virtual void                     registryTheme(const std::string& name, const IRenderer::Color& primary,
                                                   const IRenderer::Color& secondary, const IRenderer::Color& neutral,
                                                   const IRenderer::Color& accent, const IRenderer::Color& unical) = 0;
    virtual bool                     setTheme(const std::string& name)                                             = 0;
    virtual bool                     setFont(const std::string& path)                                              = 0;
}; // class IFramework

} // namespace UI

#endif // I_UI_FRAMEWORK_HPP
