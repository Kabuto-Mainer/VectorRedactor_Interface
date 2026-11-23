#pragma once

#include "Interfaces/IRenderer.hpp"
#include "Utils/ACommand.hpp"
#include "Utils/TWidget.hpp"

#include <list>
#include <memory>
#include <string>
#include <utility>

namespace UI
{

inline constexpr float BUTTON_SIZE = 40.0F;

enum class ButtonState
{
    NO_PRESSED,
    PRESSED,
};

enum class ButtonPoolDirection
{
    HORIZONTAL,
    VERTICAL,
};

// Buttons keep a non-owning reference to their command, as in Photoshop1.
class Button : public TWidget
{
public:
    Button(const Vec2F& pos, const Vec2F& size, ACommand& command, IRenderer::Color color,
           ButtonState state = ButtonState::NO_PRESSED);
    Button(const BoxF& box, ACommand& command, IRenderer::Color color,
           ButtonState state = ButtonState::NO_PRESSED);

    bool processEvent(const IInputPump::Event& event) override;
    void render() override;

    // clang-format off
    IRenderer::Color color() const { return color_; }
    void setColor(IRenderer::Color color) { color_ = color; }
    ButtonState state() const { return state_; }
    void setState(ButtonState state) { state_ = state; }
    ACommand& command() const { return command_; }
    // clang-format on

protected:
    BoxF       globalBox() const;
    IRenderer* renderer() const;

private:
    ACommand&        command_;
    IRenderer::Color color_;
    ButtonState      state_;
};

class ButtonText : public Button
{
public:
    ButtonText(const Vec2F& pos, const Vec2F& size, std::string name, ACommand& command, IRenderer::FontID font,
               IRenderer::Color color);
    ButtonText(const BoxF& box, std::string name, ACommand& command, IRenderer::FontID font,
               IRenderer::Color color);

    void render() override;

    // clang-format off
    const std::string& name() const { return name_; }
    void setName(std::string name) { name_ = std::move(name); }
    // clang-format on

private:
    std::string       name_;
    IRenderer::FontID font_;
};

class ColorButton : public Button
{
public:
    ColorButton(const Vec2F& pos, const Vec2F& size, ACommand& command, IRenderer::Color color);
    ColorButton(const BoxF& box, ACommand& command, IRenderer::Color color);

    void render() override;
};

class IconButton : public Button
{
public:
    IconButton(const Vec2F& pos, const Vec2F& size, ACommand& command, IRenderer::TextureID texture,
               IRenderer::Color color);
    IconButton(const BoxF& box, ACommand& command, IRenderer::TextureID texture, IRenderer::Color color);

    void render() override;

private:
    IRenderer::TextureID texture_;
};

class ButtonPool : public TWidget
{
public:
    explicit ButtonPool(const Vec2F& pos, ButtonPoolDirection direction = ButtonPoolDirection::HORIZONTAL);

    void add(std::unique_ptr<Button> button);
    bool remove(TObject::ID button_id);
    bool processEvent(const IInputPump::Event& event) override;
    void render() override;

    const std::list<std::unique_ptr<TObject>>& buttons() const { return children(); }

    // clang-format off
    ButtonPoolDirection direction() const { return direction_; }
    void setDirection(ButtonPoolDirection direction) { direction_ = direction; layout(); }
    // clang-format on

private:
    void layout();

    ButtonPoolDirection direction_;
};

} // namespace UI
