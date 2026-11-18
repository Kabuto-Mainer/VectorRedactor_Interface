#pragma once

#include "Core/ApplicationCore.hpp"
#include "Geometry.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IRenderer.hpp"
#include "SelectionState.hpp"
#include "ShapeLookup.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"

#include <memory>

namespace Kabuto
{

namespace Tools
{

/**
 * Draws the selection frame and the marquee rectangle.
 *
 * The overlay is a plain tree object: it renders above the document but never
 * consumes input, so tools continue to receive events through ToolDispatcher.
 */
class SelectionOverlay final : public TObject
{
public:
    SelectionOverlay(IDocument& document, TObject& document_view, std::shared_ptr<SelectionState> state);
    ~SelectionOverlay() override = default;

    SelectionOverlay(const SelectionOverlay&)            = delete;
    SelectionOverlay(SelectionOverlay&&)                 = delete;
    SelectionOverlay& operator=(const SelectionOverlay&) = delete;
    SelectionOverlay& operator=(SelectionOverlay&&)      = delete;

    bool  processEvent(const IInputPump::Event& event) override;
    void  render() override;
    Vec2F mapLocalToGlobal(const Vec2F& pos) const override;
    Vec2F mapGlobalToLocal(const Vec2F& pos) const override;

private:
    void renderSelectionFrame(IRenderer& renderer, const BoxF& box) const;

private:
    IDocument&                      document_;
    TObject&                        document_view_;
    std::shared_ptr<SelectionState> state_;
}; // class SelectionOverlay

/* ====================== Implementation ====================== */

inline SelectionOverlay::SelectionOverlay(IDocument& document, TObject& document_view,
                                          std::shared_ptr<SelectionState> state) :
    document_(document), document_view_(document_view), state_(std::move(state))
{
}

inline bool SelectionOverlay::processEvent(const IInputPump::Event& /*event*/)
{
    return false;
}

inline void SelectionOverlay::render()
{
    constexpr IRenderer::Color SELECTION_OUTER{255, 196, 0, 255};
    constexpr IRenderer::Color SELECTION_INNER{0, 0, 0, 255};
    constexpr IRenderer::Color MARQUEE_FILL{53, 132, 228, 60};
    constexpr IRenderer::Color MARQUEE_LINE{53, 132, 228, 255};
    constexpr float            BORDER_INSET     = 1.0F;
    constexpr float            MIN_VISIBLE_SIZE = 2.0F;

    IRenderer* renderer = nullptr;
    if (ApplicationCore::instance().queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer)) !=
            ApplicationCore::Error::SUCCESS ||
        renderer == nullptr)
    {
        return;
    }

    for (const TObject::ID shape_id : state_->selected())
    {
        IDocument::TShape* shape = findShape(document_, shape_id);
        if (shape == nullptr)
        {
            continue;
        }

        const BoxF box = documentToGlobal(document_view_, shape->axesAlignedBoundingBox());
        renderer->renderRectLines(SELECTION_OUTER, box);

        if (box.size().x() > MIN_VISIBLE_SIZE && box.size().y() > MIN_VISIBLE_SIZE)
        {
            const BoxF inner{box.topLeft() + Vec2F(BORDER_INSET), box.bottomRight() - Vec2F(BORDER_INSET)};
            renderer->renderRectLines(SELECTION_INNER, inner);
        }
    }

    if (state_->marquee().has_value())
    {
        const BoxF box = documentToGlobal(document_view_, *state_->marquee());
        renderer->renderRect(MARQUEE_FILL, box);
        renderer->renderRectLines(MARQUEE_LINE, box);
    }
}

inline Vec2F SelectionOverlay::mapLocalToGlobal(const Vec2F& pos) const
{
    return parent() != nullptr ? parent()->mapLocalToGlobal(pos) : pos;
}

inline Vec2F SelectionOverlay::mapGlobalToLocal(const Vec2F& pos) const
{
    return parent() != nullptr ? parent()->mapGlobalToLocal(pos) : pos;
}

}; // namespace Tools

}; // namespace Kabuto
