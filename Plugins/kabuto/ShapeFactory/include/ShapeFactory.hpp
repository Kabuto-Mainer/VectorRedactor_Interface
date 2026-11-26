#pragma once

#include "Interfaces/IDocument.hpp"
#include "export/IShapeFactory.hpp"

#include <functional>
#include <istream>
#include <map>
#include <memory>

namespace Kabuto
{

namespace ShapeFactory
{

class CShapeFactory final : public IShapeFactory
{
public:
    bool loadFrom(const std::string& path) const override;
    bool saveTo(const std::string& path) const override;
    void registry(IDocument::TShape::TypeID type_id, Loader loader, Saver saver) override;

    static CShapeFactory& instance();

private:
    struct Item
    {
        Loader loader;
        Saver  saver;
    }; // struct Item

    std::map<IDocument::TShape::TypeID, Item> map_;

}; // class CShapeFactory

} // namespace ShapeFactory

} // namespace Kabuto
