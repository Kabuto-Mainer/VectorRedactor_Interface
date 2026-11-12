#ifndef DOCUMENT_HPP
#define DOCUMENT_HPP

#include "Utils/ADocumentObject.hpp"
#include "Utils/Vec2F.hpp"
#include <optional>

class IDocument
{
public:
    IDocument()          = default;
    virtual ~IDocument() = default;

    virtual void addShape(ADocumentObject* shape)    = 0;
    virtual void removeShape(ADocumentObject* shape) = 0;

    virtual std::optional<ObjectID> hitTest(const Vec2F& pos)  = 0;
}; // class IDocument

class Document : public AObject 
{
public:
    bool processEvent(const Event& event) override;
    void render() override;

    Vec2F mapLocalToGlobal(const Vec2F& pos) const override;
    Vec2F mapGlobalToLocal(const Vec2F& pos) const override;
}; // class Document

#endif // DOCUMENT_HPP
   
