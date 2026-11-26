#pragma once

#include "include/Components.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <istream>
#include <limits>
#include <memory>
#include <ostream>

namespace Kabuto
{

namespace Document
{

class RectangleShape final : public IDocument::TShape, public ColorComponent
{
public:
    static constexpr IDocument::TShape::TypeID RECTANGLE_SHAPE_ID = 0;

    RectangleShape();
    ~RectangleShape();

    RectangleShape(Vec2F const& size);

    static std::unique_ptr<IDocument::TShape> loadFrom(std::istream& stream);
    void                                      saveTo(std::ostream& stream) const;

    bool processEvent(IInputPump::Event const& event) override;
    void render() override;

    bool                              hitTest(Vec2F const& pos) const override;
    virtual BoxF                      axesAlignedBoundingBox() const override;
    virtual IDocument::TShape::TypeID tid() const override;

    virtual IDocument::TShape::Error queryComponent(IDocument::AShapeComponent::ID coid,
                                                    IDocument::AShapeComponent**   component) override;
    virtual IDocument::TShape::Error queryComponent(IDocument::AShapeComponent::ID     coid,
                                                    IDocument::AShapeComponent const** component) const override;
};

/* ====================== Implementation ====================== */

inline RectangleShape::RectangleShape()  = default;
inline RectangleShape::~RectangleShape() = default;

inline RectangleShape::RectangleShape(Vec2F const& size)
{
    setScaling(size);
}

inline std::unique_ptr<IDocument::TShape> RectangleShape::loadFrom(std::istream& stream)
{
    float scaling_x{};
    float scaling_y{};
    float rotation{};
    float translation_x{};
    float translation_y{};
    int   red{};
    int   green{};
    int   blue{};
    int   alpha{};

    if (!(stream >>
          scaling_x >>
          scaling_y >>
          rotation >>
          translation_x >>
          translation_y >>
          red >>
          green >>
          blue >>
          alpha))
    {
        return nullptr;
    }

    auto shape = std::make_unique<RectangleShape>();
    shape->setScaling(Vec2F{scaling_x, scaling_y});
    shape->setRotation(rotation);
    shape->setTranslation(Vec2F{translation_x, translation_y});
    shape->setColor(IRenderer::Color{static_cast<uint8_t>(std::clamp(red, 0, 255)),
                                     static_cast<uint8_t>(std::clamp(green, 0, 255)),
                                     static_cast<uint8_t>(std::clamp(blue, 0, 255)),
                                     static_cast<uint8_t>(std::clamp(alpha, 0, 255))});

    return shape;
}

inline void RectangleShape::saveTo(std::ostream& stream) const
{
    stream << std::setprecision(std::numeric_limits<float>::max_digits10) << scaling().x() << ' ' << scaling().y()
           << ' ' << rotation() << ' ' << translation().x() << ' ' << translation().y() << ' '
           << static_cast<unsigned int>(color().r) << ' ' << static_cast<unsigned int>(color().g) << ' '
           << static_cast<unsigned int>(color().b) << ' ' << static_cast<unsigned int>(color().a);
}

inline bool RectangleShape::processEvent(IInputPump::Event const& /*event*/)
{
    ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "RectangleShape can't process events\n");
    return false;
}

inline void RectangleShape::render()
{
    IRenderer* renderer{};
    ApplicationCore::instance().queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer));
    assert(renderer);

    BoxF to_render{
        Vec2F{std::abs(scaling().x()), std::abs(scaling().y())}
    };
    to_render.setCenter(translation());
    renderer->renderRect(color(), to_render, rotation());

    assert(children().empty());
}

inline bool RectangleShape::hitTest(Vec2F const& pos) const
{
    return BoxF{-0.5F, -0.5F, 0.5F, 0.5F}.contains(mapGlobalToLocal(pos));
}

inline BoxF RectangleShape::axesAlignedBoundingBox() const
{
    Vec2F const vertex1{transform().transformPoint(Vec2F{-0.5F, -0.5F})};
    Vec2F const vertex2{transform().transformPoint(Vec2F{-0.5F, +0.5F})};
    Vec2F const vertex3{transform().transformPoint(Vec2F{+0.5F, -0.5F})};
    Vec2F const vertex4{transform().transformPoint(Vec2F{+0.5F, +0.5F})};

    return BoxF{std::min({vertex1.x(), vertex2.x(), vertex3.x(), vertex4.x()}),
                std::min({vertex1.y(), vertex2.y(), vertex3.y(), vertex4.y()}),
                std::max({vertex1.x(), vertex2.x(), vertex3.x(), vertex4.x()}),
                std::max({vertex1.y(), vertex2.y(), vertex3.y(), vertex4.y()})};
}

inline IDocument::TShape::TypeID RectangleShape::tid() const
{
    return RECTANGLE_SHAPE_ID;
}

inline IDocument::TShape::Error RectangleShape::queryComponent(IDocument::AShapeComponent::ID coid,
                                                               IDocument::AShapeComponent**   component)
{
    assert(component);

    switch (coid)
    {
        case ColorComponent::COID:
        {
            (*component) = static_cast<ColorComponent*>(this);
            return IDocument::TShape::Error::SUCCESS;
        }
        default:
            break;
    }
    (*component) = nullptr;
    return IDocument::TShape::Error::COMPONENT_NOT_FOUND;
}

inline IDocument::TShape::Error RectangleShape::queryComponent(IDocument::AShapeComponent::ID     coid,
                                                               IDocument::AShapeComponent const** component) const
{
    assert(component);

    switch (coid)
    {
        case ColorComponent::COID:
        {
            (*component) = static_cast<ColorComponent const*>(this);
            return IDocument::TShape::Error::SUCCESS;
        }
        default:
            break;
    }
    (*component) = nullptr;
    return IDocument::TShape::Error::COMPONENT_NOT_FOUND;
}

}; // namespace Document

}; // namespace Kabuto
