#ifndef A_DOCUMENT_OBJECT_HPP
#define A_DOCUMENT_OBJECT_HPP

#include "Core/ApplicationCore.hpp"
#include "Utils/ADocumentObjectComponent.hpp"
#include "Utils/AObject.hpp"
#include "Utils/Matrix3F.hpp"
#include "Utils/Transformable.hpp"
#include "Utils/Vec2F.hpp"

#include <cassert>
#include <cstdint>
#include <memory>
#include <unordered_map>

// forward declaration
class BoxF;

// DocumentObjectType
using DOT_ID = uint64_t;

class ADocumentObject : public AObject, public Transformable
{
public:
    virtual DOT_ID id() const = 0;

    ADocumentObject();
    ~ADocumentObject() override = 0;

    Vec2F mapLocalToGlobal(Vec2F const& pos) const override final;
    Vec2F mapGlobalToLocal(Vec2F const& pos) const override final;

    virtual ApplicationCore::Error queryComponent(CoID coid, ADocumentObjectComponent** component)             = 0;
    virtual ApplicationCore::Error queryComponent(CoID coid, ADocumentObjectComponent const** component) const = 0;

    virtual bool hitTest(Vec2F const& pos) const = 0;
    virtual BoxF axesAlignedBoundingBox() const  = 0;
}; // class ADocumentObject

/* ====================== Implementation ====================== */

inline ADocumentObject::~ADocumentObject() = default;

inline Vec2F ADocumentObject::mapLocalToGlobal(Vec2F const& pos) const
{
    return parent()->mapLocalToGlobal(transform().transformPoint(pos));
}

inline Vec2F ADocumentObject::mapGlobalToLocal(Vec2F const& pos) const
{
    return transform().inverted().transformPoint(parent()->mapGlobalToLocal(pos));
}


class ADocumentObjectVisitor
{
public:
    virtual ~ADocumentObjectVisitor()                 = 0;
    virtual bool visit(ADocumentObject& object)       = 0;
    virtual bool visit(const ADocumentObject& object) = 0;

}; // class ADocumentObjectVisitor

#endif // A_DOCUMENT_OBJECT_HPP
