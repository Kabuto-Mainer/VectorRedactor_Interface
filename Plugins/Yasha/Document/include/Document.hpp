#pragma once

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDocument.hpp"
#include "Utils/TWidget.hpp"

#include <memory>
namespace Yasha {
class Document final : public TWidget, public IDocument
{
public:
    // addShape transfers ownership to the document; removeShape destroys the shape.
    void addShape(std::unique_ptr<TShape> shape) override;
    void removeShape(TObject::ID shape_id) override;

    std::optional<TObject::ID> hitTest(const Vec2F& pos) override;

    // Shapes are visited in drawing order; returning false stops traversal.
    void accept(Visitor& visitor) const override;

    bool processEvent(const IInputPump::Event& event) override;
    void render() override;
};

ApplicationCore::Error initializeDocument();
} // namespace Yasha
