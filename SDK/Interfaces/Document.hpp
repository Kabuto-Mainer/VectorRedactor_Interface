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

    virtual std::optional<ObjectID> hitTest(const Vec2F& pos) = 0;
}; // class IDocument

#endif // DOCUMENT_HPP
