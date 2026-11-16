#pragma once

#include "Interfaces/IDocument.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"

#include <cstdint>
#include <memory>
#include <variant>

namespace MiniEditor
{

// Shape must be an IDocument::TShape and a ColorComponent, so it can be placed and coloured directly.
template<typename Shape>
class ShapeTool final : public IToolManager::ATool
{

public:
    ShapeTool(uint64_t tool_id, IDocument& document, const Vec2F& size, const BoxF& excluded,
              const IRenderer::Color& color);

    ID   id() const override;
    bool processEvent(const IInputPump::Event& event) override;

private:
    uint64_t         id_;
    IDocument&       document_;
    Vec2F            size_;
    BoxF             excluded_;
    IRenderer::Color color_;
};

/* ====================== Implementation ====================== */

template<typename Shape>
ShapeTool<Shape>::ShapeTool(uint64_t tool_id, IDocument& document, const Vec2F& size, const BoxF& excluded,
                            const IRenderer::Color& color) :
    id_(tool_id), document_(document), size_(size), excluded_(excluded), color_(color)
{
}

template<typename Shape>
uint64_t ShapeTool<Shape>::id() const
{
    return id_;
}

template<typename Shape>
bool ShapeTool<Shape>::processEvent(const IInputPump::Event& event)
{
    const auto* mouse = std::get_if<IInputPump::EventMouse>(&event);
    if (mouse == nullptr || mouse->type() != IInputPump::EventMouse::Type::LEFT_BUTTON_PRESSING)
    {
        return false;
    }

    // Clicks on the control panel must not create shapes underneath it.
    const Vec2F cursor = mouse->cursorPos();
    if (excluded_.contains(cursor))
    {
        return false;
    }

    auto shape = std::make_unique<Shape>(size_);
    shape->setTranslation(cursor);
    shape->setColor(color_);
    document_.addShape(shape.release());

    return true;
}

} // namespace MiniEditor
