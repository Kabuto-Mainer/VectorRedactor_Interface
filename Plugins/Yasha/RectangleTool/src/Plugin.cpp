#include "RectangleTool.hpp"
#include "RectangleShape.hpp"
#include "Document.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolBar.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Interfaces/Package.hpp"
#include "QueryInterface.hpp"
#include "ToolBarIID.hpp"

#include <memory>

namespace
{

class ToolRegistration final
{
public:
    ~ToolRegistration()
    {
        if (manager_ != nullptr)
        {
            manager_->removeTool(Yasha::Tools::RECTANGLE_TOOL_ID);
        }
    }

    void setManager(IToolManager* manager)
    {
        manager_ = manager;
    }

private:
    IToolManager* manager_ = nullptr;
};

ToolRegistration& registration()
{
    static ToolRegistration guard;
    return guard;
}

} // namespace

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    auto* document = queryInterface<IDocument>(IID_IDOCUMENT);
    auto* manager  = queryInterface<IToolManager>(IID_TOOL_MANAGER);
    auto* toolbar  = queryInterface<IToolBar>(IID_TOOL_BAR);
    auto* renderer = queryInterface<IRenderer>(IID_RENDERER);

    auto* yasha_document = dynamic_cast<Yasha::Document*>(document);
    if (yasha_document == nullptr || manager == nullptr || toolbar == nullptr || renderer == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL,
                             "Rectangle tool requires Document, ToolManager, ToolBar and Render");
        return false;
    }

    const auto texture = renderer->loadTexture("RectangleTool/assets/rectangle.png");

    if (!texture)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Rectangle tool icon could not be loaded");
        return false;
    }

    if (!yasha_document->registerShapeType(
            Yasha::Tools::RectangleShape::TYPE_ID, "rectangle",
            [](IRenderer::Color color) -> std::unique_ptr<IDocument::TShape>
            {
                auto shape = std::make_unique<Yasha::Tools::RectangleShape>(Vec2F{2.0F, 2.0F});
                shape->setColor(color);
                return shape;
            },
            [](const IDocument::TShape& shape)
            { return static_cast<const Yasha::Tools::RectangleShape&>(shape).color(); }))
    {
        return false;
    }

    auto tool = std::make_unique<Yasha::Tools::RectangleTool>(*document);
    if (manager->addTool(std::move(tool)) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Rectangle tool could not be registered");
        return false;
    }
    registration().setManager(manager);

    if (manager->setActiveTool(Yasha::Tools::RECTANGLE_TOOL_ID) != IToolManager::Error::SUCCESS)
    {
        manager->removeTool(Yasha::Tools::RECTANGLE_TOOL_ID);
        registration().setManager(nullptr);
        return false;
    }

    toolbar->addTool(Yasha::Tools::RECTANGLE_TOOL_ID, *texture);
    return true;
}
