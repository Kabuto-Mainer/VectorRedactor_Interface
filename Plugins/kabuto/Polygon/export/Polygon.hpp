#pragma once

#include "Interfaces/IDocument.hpp"
#include "Plugins/kabuto/Document/include/Components.hpp"
#include "Plugins/kabuto/Polygon/export/PolygonComponent.hpp"
#include "Utils/Vec2F.hpp"

namespace Kabuto
{

namespace PolygonHuiny
{

class Polygon final : public IDocument::TShape, public Kabuto::Document::ColorComponent,
                public PolygonComponent
{
public:
    static constexpr IDocument::TShape::TypeID POLYGON_SHAPE_ID = 1'764;

    Polygon(std::list<Vec2F> points);
    ~Polygon() override;

    Error queryComponent(AShapeComponent::ID coid, AShapeComponent const** component) const override;
    Error queryComponent(AShapeComponent::ID coid, AShapeComponent** component) override;
    bool  hitTest(Vec2F const& pos) const override;

    BoxF   axesAlignedBoundingBox() const override;
    TypeID tid() const override;

    bool               movePoint(size_t logic_idx, const Vec2F& new_pos) override;
}; // class Polygon






} // namespace PolygonHuiny

} // namespace Kabuto
