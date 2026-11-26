#pragma once

#include "Interfaces/IDocument.hpp"
#include "Interfaces/IRenderer.hpp"

namespace Yasha::Tools
{

class ColorComponent : public IDocument::AShapeComponent
{
public:
    static constexpr ID COID = 4;

    // clang-format off
    IRenderer::Color color() const { return color_; }
    void setColor(IRenderer::Color color) { color_ = color; }
    // clang-format on

private:
    IRenderer::Color color_{66, 181, 226, 220};
};

class RectangleShape final : public IDocument::TShape, public ColorComponent
{
public:
    static constexpr TypeID TYPE_ID = 0;

    explicit RectangleShape(const Vec2F& size);

    bool processEvent(const IInputPump::Event& event) override;
    void render() override;

    bool hitTest(const Vec2F& pos) const override;
    BoxF axesAlignedBoundingBox() const override;
    TypeID tid() const override;

    Error queryComponent(IDocument::AShapeComponent::ID coid, IDocument::AShapeComponent** component) override;
    Error queryComponent(IDocument::AShapeComponent::ID coid, const IDocument::AShapeComponent** component) const override;
};

} // namespace Yasha::Tools
