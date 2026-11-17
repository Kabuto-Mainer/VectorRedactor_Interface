#include "SDK/Core/ApplicationCore.hpp"
#include "Interfaces/IToolBar.hpp"
#include "Utils/TObject.hpp"
#include "Utils/TWidget.hpp" 
#include "Interfaces/IToolManager.hpp"
#include "Plugins/UIFW/IUIFramework.hpp"

const BoxF TOOL_BOX {10, 10, 20, 20};
const std::string TOOL_TEXT = "TOOL_TEXT\n";
const std::string TOOL_ICON = "TOOL_ICON\n";

class TToolBar : public TWidget
{
public:
    bool processEvent(const IInputPump::Event& event) ;
    void render()                                     ;

private:

}; // class ToolBar    

class CToolBar :
    public IToolBar
{
private:
    TToolBar* tool_bar_;
public:
    void addTool(IToolManager::ATool::ID tool_id, IRenderer::TextureID texture_id);
    void removeTool(IToolManager::ATool::ID tool_id);
}; // ToolBar

void CToolBar::addTool(IToolManager::ATool::ID tool_id, IRenderer::TextureID texture_id)
{
    IUIFramework* framework = nullptr;
    ApplicationCore::instance().queryInterface(IID_IUIFramework, reinterpret_cast<void**>(&framework));

    IToolManager* tm = nullptr;
    ApplicationCore::instance().queryInterface(IID_TOOL_MANAGER, reinterpret_cast<void**>(&tm));

    IUIFramework::SwitcherDescription sd // где взять описание
    {
        TOOL_BOX,
        TOOL_TEXT, 
        TOOL_ICON,
    };

    framework->create(sd);
    // create();
    // tool_bar_->addChild(child); // где взять ребенка?? сделать самому
}

