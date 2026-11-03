#ifndef A_DOCUMENT_HPP
#define A_DOCUMENT_HPP

#include "Utils/AObject.hpp"
#include "Utils/DocumentObject.hpp"

#include <memory>

// Forward declaration
class ITool;

class IDocument
{
public:
    using ShapeID                      = size_t;
    static constexpr ShapeID NULLSHAPE = SIZE_MAX;

public:
    virtual ~IDocument() = default;

    virtual ShapeID addShape(std::unique_ptr<DocumentObject> shape) = 0;
    virtual DocumentObject* queryShape(ShapeID const& sid) = 0;
    virtual DocumentObject const* queryShape(ShapeID const& sid) const = 0;
    virtual void removeShape(ShapeID const& sid) = 0;
    std::optional<ShapeID> hitTest(Vec2F const& point) const = 0;

private:
    explicit IDocument() = default;
};

#endif
