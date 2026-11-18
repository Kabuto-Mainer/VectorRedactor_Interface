#pragma once

#include "IDocumentView.hpp"
#include "Interfaces/IDocument.hpp"
#include "Utils/TObject.hpp"

#include <memory>
#include <optional>

namespace Kabuto
{

namespace Document
{

/**
 * Scene-tree node that owns and displays the document shapes.
 *
 * The IDocument implementation is a service (CDocument) and is deliberately
 * not a tree object. This node is the only part of the document that lives in
 * the tree; the service delegates all shape operations to it.
 */
class DocumentNode final : public TObject
{
public:
    DocumentNode()           = default;
    ~DocumentNode() override = default;

    DocumentNode(const DocumentNode&)            = delete;
    DocumentNode(DocumentNode&&)                 = delete;
    DocumentNode& operator=(const DocumentNode&) = delete;
    DocumentNode& operator=(DocumentNode&&)      = delete;

    // Node owns its shapes: addShape takes ownership, removeShape destroys the shape.
    void addShape(std::unique_ptr<IDocument::TShape> shape);
    void removeShape(TObject::ID shape_id);

    // Returns the topmost shape (the last one added) under the global position.
    std::optional<TObject::ID> hitTest(const Vec2F& pos);

    // Visits direct shape children in insertion order. Traversal stops when visit() returns false.
    void accept(IDocument::Visitor& visitor);
    void accept(IDocument::Visitor& visitor) const;

    // Shapes are passive and do not handle input; events are routed to tools instead.
    bool  processEvent(const IInputPump::Event& event) override;
    void  render() override;
    Vec2F mapLocalToGlobal(const Vec2F& pos) const override;
    Vec2F mapGlobalToLocal(const Vec2F& pos) const override;
}; // class DocumentNode

/**
 * IDocument singleton service.
 *
 * The service stores no scene objects by itself: all document content is
 * owned by DocumentNode, which is bound to the service during initialization.
 */
class CDocument final : public IDocument, public IDocumentView
{
public:
    CDocument(const CDocument&)            = delete;
    CDocument(CDocument&&)                 = delete;
    CDocument& operator=(const CDocument&) = delete;
    CDocument& operator=(CDocument&&)      = delete;

    static CDocument& instance();

    void bindNode(DocumentNode& node);
    void unbindNode();

    /**
     * Scene-tree node owned by the document.
     *
     * The service itself is not a TObject; plugins that need document
     * coordinates or document-layer placement use this node.
     */
    DocumentNode* view() const override;

    void addShape(std::unique_ptr<TShape> shape) override;
    void removeShape(TObject::ID shape_id) override;

    std::optional<TObject::ID> hitTest(const Vec2F& pos) override;

    void accept(Visitor& visitor) override;
    void accept(Visitor& visitor) const override;

private:
    CDocument()           = default;
    ~CDocument() override = default;

    DocumentNode* node() const;

    DocumentNode* node_ = nullptr;
}; // class CDocument

}; // namespace Document

} // namespace Kabuto
