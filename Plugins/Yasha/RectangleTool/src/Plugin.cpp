#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolBar.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Interfaces/Package.hpp"
#include "RectangleTool.hpp"
#include "ToolBarIID.hpp"

#include <memory>

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

    if (document == nullptr || manager == nullptr || toolbar == nullptr || renderer == nullptr)
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
