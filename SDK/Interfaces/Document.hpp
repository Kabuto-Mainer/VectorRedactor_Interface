#ifndef _DOCUMENT_HPP_
#define _DOCUMENT_HPP_

#include "Utils/ADocumentObject.hpp"
#include "Utils/Vec2F.hpp"

class IDocument
{
public:
    IDocument()          = default;
    virtual ~IDocument() = default;

    virtual void addShape(ADocumentObject* shape) = 0;
    virtual void removeShape()                    = 0;

    virtual ADocumentObject* findShape(ObjectID id) const = 0;
    virtual ADocumentObject* hitTest(const Vec2F& pos)    = 0;
}; // class IDocument

class Document : public AObject, public IDocument
{
public:
    void addShape(ADocumentObject* shape) override;
    void removeShape() override;

    ADocumentObject* findShape(ObjectID id) const override;
    ADocumentObject* hitTest(const Vec2F& pos) override;

    bool processEvent(const Event& event) override;
    void render() override;

    Vec2F mapLocalToGlobal(const Vec2F& pos) const override;
    Vec2F mapGlobalToLocal(const Vec2F& pos) const override;
}; // class Document

#endif
