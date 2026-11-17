#pragma once

#include "Interfaces/IDocument.hpp"
#include "Utils/TObject.hpp"

#include <memory>
#include <optional>

namespace Kabuto
{

namespace Document
{

class CDocument final : public IDocument, public TObject
{
public:
    CDocument()           = default;
    ~CDocument() override = default;

    CDocument(const CDocument&)            = delete;
    CDocument(CDocument&&)                 = delete;
    CDocument& operator=(const CDocument&) = delete;
    CDocument& operator=(CDocument&&)      = delete;

    // Document owns its shapes: addShape takes ownership, removeShape destroys the shape.
    void addShape(std::unique_ptr<TShape> shape) override;
    void removeShape(TObject::ID shape_id) override;

    // Returns the topmost shape (the last one added) under the global position.
    std::optional<TObject::ID> hitTest(const Vec2F& pos) override;

    // Visits direct shape children in insertion order. Traversal stops when visit() returns false.
    void accept(Visitor& visitor) override;
    void accept(Visitor& visitor) const override;

    // Shapes are passive and do not handle input; events are routed to tools instead.
    bool processEvent(const IInputPump::Event& event) override;
    void render() override;

    Vec2F mapLocalToGlobal(const Vec2F& pos) const override;
    Vec2F mapGlobalToLocal(const Vec2F& pos) const override;
}; // class CDocument

}; // namespace Document

} // namespace Kabuto
