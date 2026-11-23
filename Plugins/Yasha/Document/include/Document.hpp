#pragma once

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Utils/TWidget.hpp"

#include <map>
#include <memory>
#include <string>

namespace Yasha {
class Document final : public TWidget, public IDocument
{
public:
    using ShapeFactory = std::unique_ptr<TShape> (*)(IRenderer::Color);
    using ShapeColor = IRenderer::Color (*)(const TShape&);

    bool registerShapeType(TShape::TypeID id, std::string name, ShapeFactory factory, ShapeColor color);
    bool save() const;
    bool load();

    // addShape transfers ownership to the document; removeShape destroys the shape.
    void addShape(std::unique_ptr<TShape> shape) override;
    void removeShape(TObject::ID shape_id) override;

    std::optional<TObject::ID> hitTest(const Vec2F& pos) override;

    // Shapes are visited in drawing order; returning false stops traversal.
    void accept(Visitor& visitor) const override;

    bool processEvent(const IInputPump::Event& event) override;
    void render() override;

private:
    struct ShapeType
    {
        std::string name;
        ShapeFactory factory;
        ShapeColor color;
    };
    std::map<TShape::TypeID, ShapeType> shape_types_;
};

ApplicationCore::Error initializeDocument();
} // namespace Yasha
