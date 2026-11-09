#ifndef ELLIPSE_SHAPE
#define ELLIPSE_SHAPE

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Utils/Document/AShape.hpp"
#include "Utils/Math/BoxF.hpp"
#include "Utils/Math/Vec2F.hpp"

#include <cassert>
#include <cmath>

class EllipseShape : public AShape
{
public:
    EllipseShape();
    ~EllipseShape() override final;

    EllipseShape(Vec2F const& size);

    bool processEvent(Event const& event) override final;
    void render() const override final;

    bool         hitTest(Vec2F const& pos) const override final;
    virtual BoxF axesAlignedBoundingBox() const override final;

private:
    Color color_{};
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

inline void EllipseShape::render() const
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
