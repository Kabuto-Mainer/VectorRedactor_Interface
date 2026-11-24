#pragma once

#include "Components.hpp"

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

namespace Shapes
{

class EllipseShape final : public IDocument::TShape, public Document::ColorComponent
{
public:
    static constexpr IDocument::TShape::TypeID ELLIPSE_SHAPE_ID = 1;

    EllipseShape();
    ~EllipseShape() override;

    explicit EllipseShape(Vec2F const& size);

    static std::unique_ptr<IDocument::TShape> loadFrom(std::istream& stream);
    void                                      saveTo(std::ostream& stream) const;

    bool processEvent(IInputPump::Event const& event) override;
    void render() override;

    bool                      hitTest(Vec2F const& pos) const override;
    BoxF                      axesAlignedBoundingBox() const override;
    IDocument::TShape::TypeID tid() const override;

    IDocument::TShape::Error queryComponent(IDocument::AShapeComponent::ID coid,
                                            IDocument::AShapeComponent**   component) override;
    IDocument::TShape::Error queryComponent(IDocument::AShapeComponent::ID     coid,
                                            IDocument::AShapeComponent const** component) const override;
};

/* ====================== Implementation ====================== */

inline EllipseShape::EllipseShape()  = default;
inline EllipseShape::~EllipseShape() = default;

inline EllipseShape::EllipseShape(Vec2F const& size)
{
    setScaling(size);
}

inline std::unique_ptr<IDocument::TShape> EllipseShape::loadFrom(std::istream& stream)
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

    auto shape = std::make_unique<EllipseShape>();
    shape->setScaling(Vec2F{scaling_x, scaling_y});
    shape->setRotation(rotation);
    shape->setTranslation(Vec2F{translation_x, translation_y});
    shape->setColor(IRenderer::Color{static_cast<uint8_t>(std::clamp(red, 0, 255)),
                                     static_cast<uint8_t>(std::clamp(green, 0, 255)),
                                     static_cast<uint8_t>(std::clamp(blue, 0, 255)),
                                     static_cast<uint8_t>(std::clamp(alpha, 0, 255))});

    return shape;
}

inline void EllipseShape::saveTo(std::ostream& stream) const
{
    stream << std::setprecision(std::numeric_limits<float>::max_digits10) << scaling().x() << ' ' << scaling().y()
           << ' ' << rotation() << ' ' << translation().x() << ' ' << translation().y() << ' '
           << static_cast<unsigned int>(color().r) << ' ' << static_cast<unsigned int>(color().g) << ' '
           << static_cast<unsigned int>(color().b) << ' ' << static_cast<unsigned int>(color().a);
}

inline bool EllipseShape::processEvent(IInputPump::Event const& /*event*/)
{
    ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "EllipseShape can't process events\n");
    return false;
}

inline void EllipseShape::render()
{
    IRenderer* renderer{};
    const ApplicationCore::Error error =
        ApplicationCore::instance().queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer));

    if (error != ApplicationCore::Error::SUCCESS || renderer == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "EllipseShape can't find the renderer\n");
        return;
    }

    const Vec2F center = mapLocalToGlobal(Vec2F{});
    const Vec2F x_axis = mapLocalToGlobal(Vec2F{1.0F, 0.0F}) - center;
    const Vec2F y_axis = mapLocalToGlobal(Vec2F{0.0F, 1.0F}) - center;

    BoxF to_render{
        Vec2F{x_axis.len(), y_axis.len()}
    };
    to_render.setCenter(center);
    renderer->renderEllipse(color(), to_render, std::atan2(x_axis.y(), x_axis.x()));

    assert(children().empty());
}

inline bool EllipseShape::hitTest(Vec2F const& pos) const
{
    return (mapGlobalToLocal(pos) * 2.0F).len2() <= 1.0F;
}

inline BoxF EllipseShape::axesAlignedBoundingBox() const
{
    Vec2F const extent{
        0.5F * std::sqrt((transform()[0][0] * transform()[0][0]) + (transform()[0][1] * transform()[0][1])),
        0.5F * std::sqrt((transform()[1][0] * transform()[1][0]) + (transform()[1][1] * transform()[1][1]))
    };

    return BoxF{translation().x() - extent.x(), translation().y() - extent.y(), translation().x() + extent.x(),
                translation().y() + extent.y()};
}

inline IDocument::TShape::TypeID EllipseShape::tid() const
{
    return ELLIPSE_SHAPE_ID;
}

inline IDocument::TShape::Error EllipseShape::queryComponent(IDocument::AShapeComponent::ID coid,
                                                             IDocument::AShapeComponent**   component)
{
    if (component == nullptr)
    {
        return IDocument::TShape::Error::INVALID_ARGS;
    }

    switch (coid)
    {
        case Document::ColorComponent::COID:
        {
            (*component) = static_cast<Document::ColorComponent*>(this);
            return IDocument::TShape::Error::SUCCESS;
        }
        default:
            break;
    }
    (*component) = nullptr;
    return IDocument::TShape::Error::COMPONENT_NOT_FOUND;
}

inline IDocument::TShape::Error EllipseShape::queryComponent(IDocument::AShapeComponent::ID     coid,
                                                             IDocument::AShapeComponent const** component) const
{
    if (component == nullptr)
    {
        return IDocument::TShape::Error::INVALID_ARGS;
    }

    switch (coid)
    {
        case Document::ColorComponent::COID:
        {
            (*component) = static_cast<Document::ColorComponent const*>(this);
            return IDocument::TShape::Error::SUCCESS;
        }
        default:
            break;
    }
    (*component) = nullptr;
    return IDocument::TShape::Error::COMPONENT_NOT_FOUND;
}

} // namespace Shapes

} // namespace Kabuto
