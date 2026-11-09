#ifndef A_DOCUMENT_OBJECT_HPP
#define A_DOCUMENT_OBJECT_HPP

#include "Utils/Base/AObject.hpp"
#include "Utils/Document/Transformable.hpp"

// forward declaration
class BoxF;
class Color;

class ADocumentObject : public AObject, public Transformable
{
public:
    ADocumentObject();
    ~ADocumentObject() override = 0;

    Vec2F mapLocalToGlobal(Vec2F const& pos) const override final;
    Vec2F mapGlobalToLocal(Vec2F const& pos) const override final;

    virtual void setColor(Color color) = 0;

    virtual bool hitTest(Vec2F const& pos) const = 0;
    virtual BoxF axesAlignedBoundingBox() const  = 0;
};

/* ====================== Implementation ====================== */

inline ADocumentObject::ADocumentObject()  = default;
inline ADocumentObject::~ADocumentObject() = default;

inline Vec2F ADocumentObject::mapLocalToGlobal(Vec2F const& pos) const
{
    return parent()->mapLocalToGlobal(transform().transformPoint(pos));
}

inline Vec2F ADocumentObject::mapGlobalToLocal(Vec2F const& pos) const
{
    return transform().inverted().transformPoint(parent()->mapGlobalToLocal(pos));
}

#endif
