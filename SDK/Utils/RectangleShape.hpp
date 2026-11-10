#ifndef RECTANGLE_SHAPE
#define RECTANGLE_SHAPE

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Utils/AShape.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Vec2F.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

class RectangleShape final : public AShape
{
public:
    RectangleShape();
    ~RectangleShape() override final;

    RectangleShape(Vec2F const& size);

    bool processEvent(Event const&) override final;
    void render() override final;

    bool         hitTest(Vec2F const& pos) const override final;
    virtual BoxF axesAlignedBoundingBox() const override final;
};

/* ====================== Implementation ====================== */

inline RectangleShape::RectangleShape()  = default;
inline RectangleShape::~RectangleShape() = default;

inline RectangleShape::RectangleShape(Vec2F const& size)
{
    setScaling(size);
}

inline bool RectangleShape::processEvent(Event const&)
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
    return BoxF{-1, -1, 1, 1}.contains(mapGlobalToLocal(pos));
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
