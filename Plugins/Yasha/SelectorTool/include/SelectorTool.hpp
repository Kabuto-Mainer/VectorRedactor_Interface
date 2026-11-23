#pragma once

#include "Interfaces/IToolManager.hpp"
#include "SelectionOverlay.hpp"
#include "ToolManager.hpp"

#include <optional>

namespace Yasha::Tools
{

inline constexpr IToolManager::ATool::ID SELECTOR_TOOL_ID = 2;

class SelectorTool final : public IToolManager::ATool
{
public:
    SelectorTool(IDocument& document, ToolManager& manager, SelectionOverlay& overlay,
                 SelectionState& state);

    // clang-format off
    ID id() const override { return SELECTOR_TOOL_ID; }
    // clang-format on
    bool processEvent(const IInputPump::Event& event) override;

private:
    enum class Gesture
    {
        NONE,
        MOVE,
        ROTATE,
    };

    Vec2F toShapeParentLocal(const IDocument::TShape& shape, const Vec2F& global_pos) const;
    bool  press(const Vec2F& global_pos);
    bool  move(const Vec2F& global_pos);
    bool  release(const Vec2F& global_pos);
    bool  cancel();

    IDocument&                      document_;
    ToolManager&                    manager_;
    SelectionOverlay&               overlay_;
    SelectionState&                 state_;
    std::uint64_t                   last_activation_revision_;
    std::optional<TObject::ID>      dragged_id_;
    Gesture                         gesture_ = Gesture::NONE;
    Vec2F                           start_cursor_;
    Vec2F                           start_translation_;
    float                           start_rotation_        = 0.0F;
    float                           previous_cursor_angle_ = 0.0F;
};

} // namespace Yasha::Tools
