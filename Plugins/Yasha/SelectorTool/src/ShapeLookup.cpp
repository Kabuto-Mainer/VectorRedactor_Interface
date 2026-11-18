#include "ShapeLookup.hpp"

namespace Yasha::Tools
{

namespace
{

class ShapeLookupVisitor final : public IDocument::Visitor
{
public:
    explicit ShapeLookupVisitor(TObject::ID id) : id_(id)
    {
    }

    bool visit(IDocument::TShape& shape) override
    {
        if (shape.id() != id_)
        {
            return true;
        }
        mutable_shape_ = &shape;
        shape_         = &shape;
        return false;
    }

    bool visit(const IDocument::TShape& shape) override
    {
        if (shape.id() != id_)
        {
            return true;
        }
        shape_ = &shape;
        return false;
    }

    IDocument::TShape* mutableShape() const
    {
        return mutable_shape_;
    }
    const IDocument::TShape* shape() const
    {
        return shape_;
    }

private:
    TObject::ID              id_;
    IDocument::TShape*       mutable_shape_ = nullptr;
    const IDocument::TShape* shape_         = nullptr;
};

} // namespace

IDocument::TShape* findShape(IDocument& document, TObject::ID id)
{
    ShapeLookupVisitor visitor{id};
    document.accept(visitor);
    return visitor.mutableShape();
}

const IDocument::TShape* findShape(const IDocument& document, TObject::ID id)
{
    ShapeLookupVisitor visitor{id};
    document.accept(visitor);
    return visitor.shape();
}

} // namespace Yasha::Tools
