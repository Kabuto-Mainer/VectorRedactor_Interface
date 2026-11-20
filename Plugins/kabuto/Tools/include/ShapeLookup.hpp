#pragma once

#include "Interfaces/IDocument.hpp"
#include "Utils/TObject.hpp"

#include <vector>

namespace Kabuto
{

namespace Tools
{

/**
 * Stops as soon as the shape with the requested id is visited.
 *
 * IDocument::accept() is const-only, so lookup always goes through the const
 * overload. The mutable result is obtained the same way the other plugins do
 * it: the document owns its shapes and does not hand out const objects.
 */
class ShapeLookupVisitor final : public IDocument::Visitor
{
public:
    explicit ShapeLookupVisitor(TObject::ID target_id);
    ~ShapeLookupVisitor() = default;

    ShapeLookupVisitor(const ShapeLookupVisitor&)            = delete;
    ShapeLookupVisitor(ShapeLookupVisitor&&)                 = delete;
    ShapeLookupVisitor& operator=(const ShapeLookupVisitor&) = delete;
    ShapeLookupVisitor& operator=(ShapeLookupVisitor&&)      = delete;

    bool visit(IDocument::TShape& shape) override;
    bool visit(const IDocument::TShape& shape) override;

    IDocument::TShape*       shape() const;
    const IDocument::TShape* constShape() const;

private:
    TObject::ID              target_id_;
    IDocument::TShape*       shape_       = nullptr;
    const IDocument::TShape* const_shape_ = nullptr;
}; // class ShapeLookupVisitor

class ShapeCollectorVisitor final : public IDocument::Visitor
{
public:
    ShapeCollectorVisitor()  = default;
    ~ShapeCollectorVisitor() = default;

    ShapeCollectorVisitor(const ShapeCollectorVisitor&)            = delete;
    ShapeCollectorVisitor(ShapeCollectorVisitor&&)                 = delete;
    ShapeCollectorVisitor& operator=(const ShapeCollectorVisitor&) = delete;
    ShapeCollectorVisitor& operator=(ShapeCollectorVisitor&&)      = delete;

    bool visit(IDocument::TShape& shape) override;
    bool visit(const IDocument::TShape& shape) override;

    const std::vector<const IDocument::TShape*>& shapes() const;

private:
    std::vector<const IDocument::TShape*> shapes_;
}; // class ShapeCollectorVisitor

/* ====================== Implementation ====================== */

inline ShapeLookupVisitor::ShapeLookupVisitor(TObject::ID target_id) : target_id_(target_id)
{
}

inline bool ShapeLookupVisitor::visit(IDocument::TShape& shape)
{
    if (shape.id() != target_id_)
    {
        return true;
    }

    shape_       = &shape;
    const_shape_ = &shape;
    return false;
}

inline bool ShapeLookupVisitor::visit(const IDocument::TShape& shape)
{
    if (shape.id() != target_id_)
    {
        return true;
    }

    const_shape_ = &shape;
    return false;
}

inline IDocument::TShape* ShapeLookupVisitor::shape() const
{
    return shape_;
}

inline const IDocument::TShape* ShapeLookupVisitor::constShape() const
{
    return const_shape_;
}

inline const IDocument::TShape* findShape(const IDocument& document, TObject::ID shape_id)
{
    ShapeLookupVisitor visitor(shape_id);
    document.accept(visitor);
    return visitor.constShape();
}

inline IDocument::TShape* findShape(IDocument& document, TObject::ID shape_id)
{
    return const_cast<IDocument::TShape*>(findShape(static_cast<const IDocument&>(document), shape_id));
}

inline bool ShapeCollectorVisitor::visit(IDocument::TShape& shape)
{
    shapes_.push_back(&shape);
    return true;
}

inline bool ShapeCollectorVisitor::visit(const IDocument::TShape& shape)
{
    shapes_.push_back(&shape);
    return true;
}

inline const std::vector<const IDocument::TShape*>& ShapeCollectorVisitor::shapes() const
{
    return shapes_;
}

inline std::vector<const IDocument::TShape*> collectShapes(const IDocument& document)
{
    ShapeCollectorVisitor visitor;
    document.accept(visitor);
    return visitor.shapes();
}

}; // namespace Tools

}; // namespace Kabuto
