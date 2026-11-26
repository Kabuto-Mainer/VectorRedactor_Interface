#pragma once

#include "Interfaces/IDocument.hpp"
#include "Utils/TObject.hpp"

namespace Kabuto
{

namespace Tools
{

/**
 * Finds a shape by its identifier. IDocument::accept() is const-only, while
 * tools naturally need to modify the found shape; the document owns its
 * shapes, so casting away constness does not violate the ownership model.
 */
class FindShapeVisitor final : public IDocument::Visitor
{
public:
    explicit FindShapeVisitor(TObject::ID shape_id);
    ~FindShapeVisitor() = default;

    FindShapeVisitor(const FindShapeVisitor&)            = delete;
    FindShapeVisitor(FindShapeVisitor&&)                 = delete;
    FindShapeVisitor& operator=(const FindShapeVisitor&) = delete;
    FindShapeVisitor& operator=(FindShapeVisitor&&)      = delete;

    bool visit(IDocument::TShape& shape) override;
    bool visit(const IDocument::TShape& shape) override;

    IDocument::TShape* found() const;

private:
    TObject::ID        shape_id_;
    IDocument::TShape* shape_ = nullptr;
}; // class FindShapeVisitor

/* ====================== Implementation ====================== */

inline FindShapeVisitor::FindShapeVisitor(TObject::ID shape_id) : shape_id_(shape_id)
{
}

inline bool FindShapeVisitor::visit(IDocument::TShape& shape)
{
    if (shape.id() != shape_id_)
    {
        return true;
    }

    shape_ = &shape;
    return false;
}

inline bool FindShapeVisitor::visit(const IDocument::TShape& shape)
{
    return visit(const_cast<IDocument::TShape&>(shape));
}

inline IDocument::TShape* FindShapeVisitor::found() const
{
    return shape_;
}

}; // namespace Tools

}; // namespace Kabuto
