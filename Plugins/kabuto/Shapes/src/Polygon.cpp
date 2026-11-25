#include "Core/ApplicationCore.hpp"
#include "Interfaces/Package.hpp"
#include "Plugins/kabuto/ShapeFactory/export/IShapeFactory.hpp"
#include "export/EllipseShape.hpp"
#include "export/Polygon.hpp"
#include "export/RectangleShape.hpp"

#include <memory>
#include <ostream>

namespace
{

template<typename T>
T* queryInterface(IID iid)
{
    void* raw = nullptr;
    if (ApplicationCore::instance().queryInterface(iid, &raw) != ApplicationCore::Error::SUCCESS)
    {
        return nullptr;
    }

    return static_cast<T*>(raw);
}

template<typename Shape>
void registerShape(Kabuto::IShapeFactory& factory, IDocument::TShape::TypeID type_id)
{
    factory.registry(
        type_id, &Shape::loadFrom,
        [](IDocument::TShape& shape, std::ostream& stream) { static_cast<Shape&>(shape).saveTo(stream); }
    );
}

} // namespace

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    Kabuto::IShapeFactory* factory = queryInterface<Kabuto::IShapeFactory>(Kabuto::IID_ISHAPE_FACTORY);
    if (factory == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Shapes can't find the shape factory");
        return false;
    }

    registerShape<Kabuto::Shapes::RectangleShape>(*factory, Kabuto::Shapes::RectangleShape::RECTANGLE_SHAPE_ID);
    registerShape<Kabuto::Shapes::EllipseShape>(*factory, Kabuto::Shapes::EllipseShape::ELLIPSE_SHAPE_ID);
    registerShape<Kabuto::Shapes::PolygonShape>(*factory, Kabuto::Shapes::PolygonShape::POLYGON_SHAPE_ID);

    return true;
}
