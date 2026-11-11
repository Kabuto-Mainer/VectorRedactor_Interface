#ifndef ELLIPSE_SHAPE
#define ELLIPSE_SHAPE

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Utils/AShape.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/ColorComponent.hpp"
#include "Utils/Vec2F.hpp"

#include <cassert>
#include <cmath>

class EllipseShape final : public AShape
{
public:
    EllipseShape();
    ~EllipseShape() override final;

    EllipseShape(Vec2F const& size);

    bool processEvent(Event const& event) override final;
    void render() override final;

    bool         hitTest(Vec2F const& pos) const override final;
    virtual BoxF axesAlignedBoundingBox() const override final;
};

/* ====================== Implementation ====================== */

inline EllipseShape::EllipseShape()  = default;
inline EllipseShape::~EllipseShape() = default;

inline EllipseShape::EllipseShape(Vec2F const& size)
{
    setScaling(size);
}

inline bool EllipseShape::processEvent(Event const& event)
{
    ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "EllipseShape can't process events\n");
    return false;
}

inline void EllipseShape::render()
{
    IRenderer* renderer{};
    ApplicationCore::instance().queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer));
    assert(renderer);

    BoxF to_render{
        Vec2F{std::abs(scaling().x()), std::abs(scaling().y())}
    };
    to_render.setCenter(translation());
    renderer->renderEllipse(color(), to_render, rotation());
}

inline bool EllipseShape::hitTest(Vec2F const& pos) const
{
    return mapGlobalToLocal(pos).len2() <= 1.0F;
}

// I can't prove that
inline BoxF EllipseShape::axesAlignedBoundingBox() const
{
    Vec2F const extent{std::sqrt((transform()[0][0] * transform()[0][0]) + (transform()[0][1] * transform()[0][1])),
                       std::sqrt((transform()[1][0] * transform()[1][0]) + (transform()[1][1] * transform()[1][1]))};

    return BoxF{translation().x() - extent.x(), translation().y() - extent.y(), translation().x() + extent.x(),
                translation().y() + extent.y()};
}

#endif
