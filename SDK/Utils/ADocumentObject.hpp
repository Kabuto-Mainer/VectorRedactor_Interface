#ifndef A_DOCUMENT_OBJECT_HPP
#define A_DOCUMENT_OBJECT_HPP

#include "Core/ApplicationCore.hpp"
#include "Utils/ADocumentObjectComponent.hpp"
#include "Utils/AObject.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Matrix3F.hpp"
#include "Utils/Transformable.hpp"
#include "Utils/Vec2F.hpp"

#include <cassert>
#include <memory>
#include <unordered_map>

class ADocumentObject : public AObject, public Transformable
{
public:
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

#endif // A_DOCUMENT_OBJECT_HPP
