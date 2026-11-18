#pragma once

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDocument.hpp"
#include "Utils/TWidget.hpp"

class Document final : public TWidget, public IDocument
{
public:
    // addShape transfers ownership to the document; removeShape destroys the shape.
    void addShape(TShape* shape) override;
    void removeShape(TShape* shape) override;

    std::optional<TObject::ID> hitTest(const Vec2F& pos) override;

    // Shapes are visited in drawing order; returning false stops traversal.
    void accept(Visitor& visitor) override;
    void accept(Visitor& visitor) const override;

    bool processEvent(const IInputPump::Event& event) override;
    void render() override;
};

ApplicationCore::Error initializeDocument();
