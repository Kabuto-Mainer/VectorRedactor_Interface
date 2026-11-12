#ifndef DOCUMENT_HPP
#define DOCUMENT_HPP

#include "Core/ApplicationCore.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Transformable.hpp"
#include <cstdint>
#include <optional>

constexpr static const IID IID_IDOCUMENT = 0x00120;

class IDocument
{
public:
    class TShape;
    class AShapeComponent;

    IDocument()          = default;
    virtual ~IDocument() = default;

    virtual void addShape(TShape* shape)    = 0;
    virtual void removeShape(TShape* shape) = 0;

    virtual std::optional<TObject::ID> hitTest(const Vec2F& pos) = 0;

}; // class IDocument

class IDocument::AShapeComponent
{
public:
    using ID = uint64_t;

    AShapeComponent()          = default;
    virtual ~AShapeComponent() = default;

    AShapeComponent(const AShapeComponent&)            = delete;
    AShapeComponent(AShapeComponent&&)                 = delete;
    AShapeComponent& operator=(const AShapeComponent&) = delete;
    AShapeComponent& operator=(AShapeComponent&&)      = delete;

    // Every component reports the CoID it is registered under.
    virtual ID coid() const = 0;
}; // class AShapeComponent

class IDocument::TShape : public TObject, public Transformable
{
public:
    TShape();
    ~TShape() override = 0;

    Vec2F mapLocalToGlobal(Vec2F const& pos) const override final;
    Vec2F mapGlobalToLocal(Vec2F const& pos) const override final;

    virtual ApplicationCore::Error queryComponent(AShapeComponent::ID coid, AShapeComponent** component)             = 0;
    virtual ApplicationCore::Error queryComponent(AShapeComponent::ID coid, AShapeComponent const** component) const = 0;

    virtual bool hitTest(Vec2F const& pos) const = 0;
    virtual BoxF axesAlignedBoundingBox() const  = 0;
}; // class AShape

/* ====================== Implementation ====================== */

inline IDocument::TShape::TShape()  = default;
inline IDocument::TShape::~TShape() = default;

inline Vec2F IDocument::TShape::mapLocalToGlobal(Vec2F const& pos) const
{
    return parent()->mapLocalToGlobal(transform().transformPoint(pos));
}

inline Vec2F IDocument::TShape::mapGlobalToLocal(Vec2F const& pos) const
{
    return transform().inverted().transformPoint(parent()->mapGlobalToLocal(pos));
}

#endif // DOCUMENT_HPP
