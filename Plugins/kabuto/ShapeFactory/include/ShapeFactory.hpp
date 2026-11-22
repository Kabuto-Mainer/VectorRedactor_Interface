#pragma once

#include "Interfaces/IDocument.hpp"
#include "export/IShapeFactory.hpp"
#include <functional>
#include <istream>
#include <memory>
#include <map>

namespace Kabuto
{

namespace ShapeFactory
{

class CShapeFactory final : public IShapeFactory
{
public:
    std::unique_ptr<std::vector<std::unique_ptr<IDocument::TShape>>> loadFrom(const std::string & path) const override;
    void saveTo(const std::string & path) const override;
    void registry(IDocument::TShape::TypeID type_id, Loader loader, Saver saver) override;

    static CShapeFactory& instance();

private:
    struct Item
    {
        Loader loader_;
        Saver saver_;
    }; // struct Item

    std::map<IDocument::TShape::TypeID, Item> map_;

}; // class CShapeFactory

} // namespace ShapeFactory

} // namespace Kabuto
