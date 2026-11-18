#pragma once

#include "Interfaces/IDocument.hpp"
#include "Interfaces/IToolManager.hpp"

namespace Yasha::Tools
{

inline constexpr IToolManager::ATool::ID ELLIPSE_TOOL_ID = 3;

class EllipseTool final : public IToolManager::ATool
{
public:
    explicit EllipseTool(IDocument& document);

    // clang-format off
    ID id() const override { return ELLIPSE_TOOL_ID; }
    // clang-format on
    bool processEvent(const IInputPump::Event& event) override;

private:
    static constexpr float MIN_DRAG_SIZE = 2.0F;

    Vec2F toLocal(const Vec2F& global_pos) const;
    bool  beginDrag(const Vec2F& global_pos);
    bool  updateDrag(const Vec2F& global_pos);
    bool  endDrag(const Vec2F& global_pos);
    void  cancelDrag();
    void  createShape(const Vec2F& center, const Vec2F& size);

    IDocument&         document_;
    IDocument::TShape* preview_ = nullptr;
    Vec2F              anchor_;
    bool               dragging_ = false;
};

} // namespace Yasha::Tools
