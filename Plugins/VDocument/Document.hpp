#ifndef VDOCUMENT_DOCUMENT_HPP
#define VDOCUMENT_DOCUMENT_HPP

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"

#include <algorithm>
#include <memory>
#include <optional>

namespace VDocument
{

class Document final : public IDocument
{
public:
    Document()           = default;
    ~Document() override = default;

    Document(const Document&)            = delete;
    Document(Document&&)                 = delete;
    Document& operator=(const Document&) = delete;
    Document& operator=(Document&&)      = delete;

    // Document owns its shapes: addShape takes ownership, removeShape destroys the shape.
    void addShape(IDocument::TShape* shape) override final;
    void removeShape(IDocument::TShape* shape) override final;

    // Returns the topmost shape (the last one added) under the global position.
    std::optional<TObject::ID> hitTest(const Vec2F& pos) override final;

    // Visits direct shape children in insertion order. Traversal stops when visit() returns false.
    void accept(IDocument::Visitor& visitor) override final;
    void accept(IDocument::Visitor& visitor) const override final;

}; // class Document

/* ====================== Implementation ====================== */

inline void Document::addShape(IDocument::TShape* shape)
{
    if (shape == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Document can't add null shape");
        return;
    }

    const bool already_added =
        std::any_of(children().begin(), children().end(),
                    [shape](const std::unique_ptr<TObject>& child) { return child.get() == shape; });
    if (already_added)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "Shape is already added to document");
        return;
    }

    addChild(std::unique_ptr<TObject>{shape});
}

inline void Document::removeShape(IDocument::TShape* shape)
{
    children().remove_if([shape](const std::unique_ptr<TObject>& child) { return child.get() == shape; });
}

inline std::optional<TObject::ID> Document::hitTest(const Vec2F& pos)
{
    for (auto child = children().rbegin(); child != children().rend(); ++child)
    {
        const auto* shape = dynamic_cast<const IDocument::TShape*>(child->get());
        if (shape != nullptr && shape->hitTest(pos))
        {
            return shape->id();
        }
    }

    return std::nullopt;
}

inline void Document::accept(IDocument::Visitor& visitor)
{
    for (const auto& child : children())
    {
        auto* shape = dynamic_cast<IDocument::TShape*>(child.get());
        if (shape != nullptr && !visitor.visit(*shape))
        {
            return;
        }
    }
}

inline void Document::accept(IDocument::Visitor& visitor) const
{
    for (const auto& child : children())
    {
        const auto* shape = dynamic_cast<const IDocument::TShape*>(child.get());
        if (shape != nullptr && !visitor.visit(*shape))
        {
            return;
        }
    }
}

// Shapes are passive and do not handle input; events are routed to tools instead.
inline bool Document::processEvent(const IInputPump::Event& /*event*/)
{
    return false;
}

inline void Document::render()
{
    renderChildren();
}

inline Vec2F Document::mapLocalToGlobal(const Vec2F& pos) const
{
    if (parent() == nullptr)
    {
        return pos;
    }
    return parent()->mapLocalToGlobal(pos);
}

inline Vec2F Document::mapGlobalToLocal(const Vec2F& pos) const
{
    if (parent() == nullptr)
    {
        return pos;
    }
    return parent()->mapGlobalToLocal(pos);
}

}; // namespace VDocument

#endif // VDOCUMENT_DOCUMENT_HPP
