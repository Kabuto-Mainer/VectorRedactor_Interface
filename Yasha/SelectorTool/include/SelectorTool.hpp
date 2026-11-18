#pragma once

#include "Interfaces/IToolManager.hpp"
#include "SelectionOverlay.hpp"
#include "Document.hpp"
#include "ToolManager.hpp"

#include <optional>

namespace Yasha::Tools
{

inline constexpr IToolManager::ATool::ID SELECTOR_TOOL_ID = 2;

class SelectorTool final : public IToolManager::ATool
{
public:
    SelectorTool(Document& document, ToolManager& manager, SelectionOverlay& overlay);

    // clang-format off
    ID id() const override { return SELECTOR_TOOL_ID; }
    // clang-format on
    bool processEvent(const IInputPump::Event& event) override;

private:
    enum class Gesture { NONE, MOVE, ROTATE };

    Vec2F toDocumentLocal(const Vec2F& global_pos) const;
    bool press(const Vec2F& global_pos);
    bool move(const Vec2F& global_pos);
    bool release(const Vec2F& global_pos);
    bool cancel();

    Document& document_;
    ToolManager& manager_;
    SelectionOverlay& overlay_;
    std::uint64_t last_activation_revision_;
    std::optional<TObject::ID> dragged_id_;
    Gesture gesture_ = Gesture::NONE;
    Vec2F start_cursor_;
    Vec2F start_translation_;
    float start_rotation_ = 0.0F;
    float previous_cursor_angle_ = 0.0F;
};

} // namespace Yasha::Tools
