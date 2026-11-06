#ifndef RECTANGLE_SHAPE
#define RECTANGLE_SHAPE

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Utils/AShape.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Vec2F.hpp"

#include <algorithm>
#include <cassert>

class RectangleShape : public AShape
{
public:
    RectangleShape();
    ~RectangleShape() override final;

    RectangleShape(Vec2F const& size);

    bool processEvent(Event const& event) override final;
    void render() const override final;

    virtual Color&       color() override final;
    virtual Color const& color() const override final;

    virtual BoxF axesAlignedBoundingBox() const override final;

private:
    Color color_{};
};

/* ====================== Implementation ====================== */

inline RectangleShape::RectangleShape()  = default;
inline RectangleShape::~RectangleShape() = default;

inline RectangleShape::RectangleShape(Vec2F const& size)
{
    setScaling(size);
}

inline Color& RectangleShape::color()
{
    return color_;
}

inline Color const& RectangleShape::color() const
{
    return color_;
}

inline bool RectangleShape::processEvent(Event const& event)
{
    return false;
}

inline void RectangleShape::render() const
{
    IRenderer* renderer{};
    ApplicationCore::instance().queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer));
    assert(renderer);

    BoxF to_render{scaling()};
    to_render.setCenter(translation());
    renderer->renderRect(color(), to_render, rotation());
}

inline BoxF RectangleShape::axesAlignedBoundingBox() const
{
    Vec2F const vertex1{transform().transformPoint(Vec2F{-1, -1})};
    Vec2F const vertex2{transform().transformPoint(Vec2F{-1, +1})};
    Vec2F const vertex3{transform().transformPoint(Vec2F{+1, -1})};
    Vec2F const vertex4{transform().transformPoint(Vec2F{+1, +1})};

    return BoxF{std::min({vertex1.x(), vertex2.x(), vertex3.x(), vertex4.x()}),
                std::min({vertex1.y(), vertex2.y(), vertex3.y(), vertex4.y()}),
                std::max({vertex1.x(), vertex2.x(), vertex3.x(), vertex4.x()}),
                std::max({vertex1.y(), vertex2.y(), vertex3.y(), vertex4.y()})};
}

#endif
