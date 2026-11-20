#pragma once

#include "Interfaces/IDocument.hpp"
#include "dependens.hpp"

namespace Kabuto
{

namespace Serialize
{

// Components
constexpr static const IDocument::AShapeComponent::ID TEXT_COID  = Kabuto::Document::TextComponent::COID;
constexpr static const IDocument::AShapeComponent::ID COLOR_COID = Kabuto::Document::ColorComponent::COID;

// Shapes
constexpr static const IDocument::TShape::TypeID RECTANGLE_SHAPE_ID =
    Kabuto::Document::RectangleShape::RECTANGLE_SHAPE_ID;
constexpr static const IDocument::TShape::TypeID ELLIPSE_SHAPE_ID = Kabuto::Document::EllipseShape::ELLIPSE_SHAPE_ID;

} // namespace Serialize

} // namespace Kabuto
