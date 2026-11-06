#ifndef ELLIPSE_SHAPE
#define ELLIPSE_SHAPE

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Utils/AShape.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Vec2F.hpp"

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

    virtual Color&       color() override final;
    virtual Color const& color() const override final;

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

inline Color& EllipseShape::color()
{
    return color_;
}

inline Color const& EllipseShape::color() const
{
    return color_;
}

inline bool EllipseShape::processEvent(Event const& event)
{
    return false;
}

inline void EllipseShape::render() const
{
    IRenderer* renderer{};
    ApplicationCore::instance().queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer));
    assert(renderer);

    BoxF to_render{scaling()};
    to_render.setCenter(translation());
    renderer->renderEllipse(color(), to_render, rotation());
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
