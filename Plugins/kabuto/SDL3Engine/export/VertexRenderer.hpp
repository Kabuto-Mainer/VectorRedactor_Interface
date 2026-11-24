#pragma once

#include "Interfaces/IRenderer.hpp"
#include "Utils/Vec2F.hpp"

constexpr static const IID IID_VERTEX_RENDERER = 7;

namespace Kabuto
{

class IVertexRenderer
{
public:
    struct Vertex
    {
        Vec2F x0;
        Vec2F x1;
        Vec2F x2;
    }; // struct Vertex

    IVertexRenderer()          = default;
    virtual ~IVertexRenderer() = default;

    virtual void renderVertex(const IRenderer::Color& color, const Vertex& vertex)     = 0;
    virtual void renderVertexLine(const IRenderer::Color& color, const Vertex& vertex) = 0;
}; // class IVertexRenderer

} // namespace Kabuto
