#ifndef A_DOCUMENT_HPP
#define A_DOCUMENT_HPP

#include "SDK/Utils/AObject.hpp"
#include "SDK/Utils/AShape.hpp"

#include <memory>

// Forward declaration
class ITool;

class IDocument : private AObject
{
public:
    using ShapeID                      = size_t;
    static constexpr ShapeID NULLSHAPE = SIZE_MAX;

public:
    virtual ~IDocument() = default;



    virtual ShapeID addShape(std::unique_ptr<AShape> shape)
    {
        addChild(std::move(shape));
        return getChildren().size() - 1;
    }
    virtual AShape* queryShape(ShapeID const& sid)
    {
        return static_cast<AShape*>(getChildren()[sid].get());
    }
    virtual AShape const* queryShape(ShapeID const& sid) const
    {
        return static_cast<AShape const*>(getChildren()[sid].get());
    }
    // I can't remove children for now
    virtual void removeShape(ShapeID const& sid) = 0;



    ShapeID hitTest(Vec2F const& point) const
    {
        for (auto child = getChildren().rbegin(); child != getChildren().rend(); ++child)
        {
            AShape const* const shape = static_cast<AShape const*>(child->get());
            if (shape->hitTest(point))
            {
                return getChildren().size() - 1 - (child - getChildren().rbegin());
            }
        }

        return NULLSHAPE;
    }

private:
    explicit IDocument() = default;
};

#endif
