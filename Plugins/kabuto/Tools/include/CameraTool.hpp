#pragma once

#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Utils/Vec2F.hpp"
#include "export/ICamera.hpp"

#include <optional>
#include <variant>

namespace Kabuto
{

namespace Tools
{

/**
 * Pans and zooms the document view.
 *
 * Mouse: the wheel zooms around the cursor, the left or middle button drags
 * the view.
 * Keyboard: '='/'+' and '-' zoom, the arrows pan and '0' resets the view, so
 * the tool is usable without a wheel at all.
 *
 * A keyboard event carries no cursor position, therefore keyboard zoom is
 * anchored at the last position reported by a mouse event. Before the first
 * mouse event the anchor is the top-left corner of the screen.
 */
class CameraTool final : public IToolManager::ATool
{
public:
    static constexpr IToolManager::ATool::ID ID = 1'323;

    CameraTool(ICamera& camera);
    ~CameraTool() override = default;

    CameraTool(const CameraTool&)            = delete;
    CameraTool(CameraTool&&)                 = delete;
    CameraTool& operator=(const CameraTool&) = delete;
    CameraTool& operator=(CameraTool&&)      = delete;

    IToolManager::ATool::ID id() const override;
    bool                    processEvent(const IInputPump::Event& event) override;
    /**
     * Drops a pan that was in progress, so a release lost while another tool
     * was active cannot leave the camera attached to the cursor.
     */
    void deactivate();

private:
    static constexpr float ZOOM_STEP       = 0.1F;
    static constexpr float PAN_STEP        = 50.0F;
    static constexpr float FAST_PAN_FACTOR = 4.0F;

    bool processMouse(const IInputPump::EventMouse& mouse);
    bool processKeyboard(IInputPump::EventKeyboard keyboard);

    bool beginPan(const Vec2F& pos, IInputPump::EventMouse::Type button);
    bool updatePan(const Vec2F& pos);
    /**
     * Ends the pan. pressed_button is the type of the button that started it,
     * so a release event can be matched with the corresponding press event.
     */
    bool endPan(std::optional<IInputPump::EventMouse::Type> pressed_button = std::nullopt);

    Vec2F zoomAnchor() const;

    ICamera&                                    camera_;
    std::optional<Vec2F>                        last_cursor_;
    std::optional<IInputPump::EventMouse::Type> pan_button_;
    Vec2F                                       pan_anchor_;
}; // class CameraTool

/* ====================== Implementation ====================== */

inline CameraTool::CameraTool(ICamera& camera) : camera_(camera)
{
}

inline IToolManager::ATool::ID CameraTool::id() const
{
    return ID;
}

inline void CameraTool::deactivate()
{
    pan_button_.reset();
}

inline bool CameraTool::processEvent(const IInputPump::Event& event)
{
    if (const auto* mouse = std::get_if<IInputPump::EventMouse>(&event))
    {
        return processMouse(*mouse);
    }

    if (const auto* keyboard = std::get_if<IInputPump::EventKeyboard>(&event))
    {
        return processKeyboard(*keyboard);
    }

    return false;
}

inline bool CameraTool::processMouse(const IInputPump::EventMouse& mouse)
{
    last_cursor_ = mouse.cursorPos();

    using enum IInputPump::EventMouse::Type;

    switch (mouse.type())
    {
        case WHEEL_FORWARD:
            camera_.addScaleAt(ZOOM_STEP, mouse.cursorPos());
            return true;

        case WHEEL_BACKWARD:
            camera_.addScaleAt(-ZOOM_STEP, mouse.cursorPos());
            return true;

        case MIDDLE_BUTTON_PRESSING:
            return beginPan(mouse.cursorPos(), mouse.type());

        case MIDDLE_BUTTON_RELEASING:
            return endPan(IInputPump::EventMouse::Type::MIDDLE_BUTTON_PRESSING);

        case LEFT_BUTTON_PRESSING:
            return beginPan(mouse.cursorPos(), mouse.type());

        case LEFT_BUTTON_RELEASING:
            return endPan(IInputPump::EventMouse::Type::LEFT_BUTTON_PRESSING);

        case MOTION:
            return updatePan(mouse.cursorPos());

        default:
            return false;
    }
}

inline bool CameraTool::processKeyboard(IInputPump::EventKeyboard keyboard)
{
    if (keyboard.type() != IInputPump::EventKeyboard::Type::KEY_DOWN)
    {
        return false;
    }

    using enum IInputPump::EventKeyboard::Key;
    using enum IInputPump::EventKeyboard::Modifier;

    const bool  fast_pan = keyboard.hasModifiers(LEFT_SHIFT) || keyboard.hasModifiers(RIGHT_SHIFT);
    const float pan_step = fast_pan ? PAN_STEP * FAST_PAN_FACTOR : PAN_STEP;

    switch (keyboard.key())
    {
        case EQUALS:
            camera_.addScaleAt(ZOOM_STEP, zoomAnchor());
            return true;

        case MINUS:
            camera_.addScaleAt(-ZOOM_STEP, zoomAnchor());
            return true;

        case NUM_0:
            camera_.reset();
            return true;

        case LEFT:
            camera_.addShift(Vec2F{-pan_step, 0.0F});
            return true;

        case RIGHT:
            camera_.addShift(Vec2F{pan_step, 0.0F});
            return true;

        case UP:
            camera_.addShift(Vec2F{0.0F, -pan_step});
            return true;

        case DOWN:
            camera_.addShift(Vec2F{0.0F, pan_step});
            return true;

        default:
            return false;
    }
}

inline bool CameraTool::beginPan(const Vec2F& pos, IInputPump::EventMouse::Type button)
{
    /* Always re-anchor: a lost release event must not wedge the pan forever. */
    pan_anchor_ = pos;
    pan_button_ = button;

    return true;
}

inline bool CameraTool::updatePan(const Vec2F& pos)
{
    if (!pan_button_.has_value())
    {
        return false;
    }

    camera_.addShift(pos - pan_anchor_);
    pan_anchor_ = pos;

    return true;
}

inline bool CameraTool::endPan(std::optional<IInputPump::EventMouse::Type> pressed_button)
{
    if (!pan_button_.has_value())
    {
        return false;
    }

    if (pressed_button.has_value() && pan_button_ != pressed_button)
    {
        return false;
    }

    pan_button_.reset();
    return true;
}

inline Vec2F CameraTool::zoomAnchor() const
{
    return last_cursor_.value_or(Vec2F{});
}

}; // namespace Tools

}; // namespace Kabuto
