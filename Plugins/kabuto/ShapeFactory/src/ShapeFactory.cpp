
#include "include/ShapeFactory.hpp"
#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/Package.hpp"
#include "export/IShapeFactory.hpp"

#include <fstream>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace Kabuto
{

namespace ShapeFactory
{

namespace
{

class CSaveVisitor final : public IDocument::Visitor
{
public:
    using Writer = std::function<bool(IDocument::TShape&, std::ostream&)>;

    CSaveVisitor(Writer writer, std::ostream& stream) : writer_(std::move(writer)), stream_(stream)
    {
    }

    bool visit(IDocument::TShape& shape) override
    {
        return writer_(shape, stream_);
    }

    bool visit(const IDocument::TShape& shape) override
    {
        return writer_(const_cast<IDocument::TShape&>(shape), stream_);
    }

private:
    Writer        writer_;
    std::ostream& stream_;
}; // class CSaveVisitor

} // namespace

bool CShapeFactory::loadFrom(const std::string& path) const
{
    std::ifstream file{path};
    if (!file.is_open())
    {
        ApplicationCore::instance().log(ApplicationCore::LogLevel::ERROR, "File %s not founded", path.c_str());
        return false;
    }

    auto shapes = std::vector<std::unique_ptr<IDocument::TShape>>();

    file >> std::ws;
    while (file.peek() != std::char_traits<char>::eof())
    {
        IDocument::TShape::TypeID type_id = 0;
        if (!(file >> type_id))
        {
            ApplicationCore::instance().log(ApplicationCore::LogLevel::ERROR, "Invalid shape IID in file '%s'",
                                            path.c_str());
            break;
        }

        const auto found = map_.find(type_id);
        if (found == map_.end())
        {
            ApplicationCore::instance().log(ApplicationCore::LogLevel::ERROR, "Shape IID %llu is not registered",
                                            static_cast<unsigned long long>(type_id));
            break;
        }

        std::unique_ptr<IDocument::TShape> shape = found->second.loader(file);
        if (!shape)
        {
            ApplicationCore::instance().log(ApplicationCore::LogLevel::ERROR,
                                            "Failed to load shape with IID %llu from '%s'",
                                            static_cast<unsigned long long>(type_id), path.c_str());
            break;
        }

        shapes.push_back(std::move(shape));
        file >> std::ws;
    }

    if (file.bad())
    {
        ApplicationCore::instance().log(ApplicationCore::LogLevel::ERROR, "Failed to read file '%s'", path.c_str());
        return false;
    }

    IDocument * document = nullptr;
    auto result = ApplicationCore::instance().queryInterface(IID_IDOCUMENT, reinterpret_cast<void**>(&document));
    if (result != ApplicationCore::Error::SUCCESS)
    {
        ApplicationCore::instance().log(ApplicationCore::LogLevel::ERROR, "ShapeFactory can't find Document to load shapes");
        return false;
    }

    for (auto & it : shapes)
    {
        document->addShape(std::move(it));
    }
    return true;
}

bool CShapeFactory::saveTo(const std::string& path) const
{
    IDocument*                   document = nullptr;
    const ApplicationCore::Error error =
        ApplicationCore::instance().queryInterface(IID_IDOCUMENT, reinterpret_cast<void**>(&document));
    if (error != ApplicationCore::Error::SUCCESS || document == nullptr)
    {
        ApplicationCore::instance().log(ApplicationCore::LogLevel::ERROR, "Shape factory can't find the document: %s",
                                        ApplicationCore::strError(error));
        return false;
    }

    std::ofstream file{path};
    if (!file.is_open())
    {
        ApplicationCore::instance().log(ApplicationCore::LogLevel::ERROR, "File '%s' can't be opened for writing",
                                        path.c_str());
        return false;
    }

    CSaveVisitor visitor(
        [this](IDocument::TShape& shape, std::ostream& stream) -> bool
        {
            const auto found = map_.find(shape.tid());
            if (found == map_.end())
            {
                ApplicationCore::instance().log(ApplicationCore::LogLevel::WARNING,
                                                "Shape IID %llu is not registered and will be skipped",
                                                static_cast<unsigned long long>(shape.tid()));
                return true;
            }

            stream << shape.tid() << ' ';
            found->second.saver(shape, stream);
            stream << '\n';

            if (!stream)
            {
                ApplicationCore::instance().log(ApplicationCore::LogLevel::ERROR, "Failed to save shape with IID %llu",
                                                static_cast<unsigned long long>(shape.tid()));
                return false;
            }

            return true;
        },
        file
    );

    document->accept(visitor);

    if (!file)
    {
        ApplicationCore::instance().log(ApplicationCore::LogLevel::ERROR, "Failed to write file '%s'", path.c_str());
        return false;
    }
    return true;
}

void CShapeFactory::registry(IDocument::TShape::TypeID type_id, Loader loader, Saver saver)
{
    if (!loader || !saver)
    {
        ApplicationCore::instance().log(ApplicationCore::LogLevel::ERROR,
                                        "Can't register shape IID %llu: loader or saver is empty",
                                        static_cast<unsigned long long>(type_id));
        return;
    }

    const auto result = map_.insert_or_assign(type_id, Item{std::move(loader), std::move(saver)});
    if (!result.second)
    {
        ApplicationCore::instance().log(ApplicationCore::LogLevel::WARNING,
                                        "Shape IID %llu is already registered; loader and saver are replaced",
                                        static_cast<unsigned long long>(type_id));
    }
}

CShapeFactory& CShapeFactory::instance()
{
    static CShapeFactory factory;
    return factory;
}

} // namespace ShapeFactory

} // namespace Kabuto


extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    ApplicationCore& core = ApplicationCore::instance();

    IDocument* document = nullptr;
    if (core.queryInterface(IID_IDOCUMENT, reinterpret_cast<void**>(&document)) != ApplicationCore::Error::SUCCESS ||
        document == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "ShapeFactory can't find the document");
        return false;
    }

    const ApplicationCore::Error error =
        core.registerInterface(Kabuto::IID_ISHAPE_FACTORY, static_cast<void*>(static_cast<Kabuto::IShapeFactory*>(&Kabuto::ShapeFactory::CShapeFactory::instance())));
    if (error != ApplicationCore::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Failed to register ShapeFactory: %s",
                             ApplicationCore::strError(error));
        return false;
    }
    return true;
}
