#ifndef I_TOOL_BAR
#define I_TOOL_BAR

#include "IRenderer.hpp"
#include "IToolManager.hpp"

class IToolBar
{
public:
    IToolBar()          = default;
    virtual ~IToolBar() = default;

    IToolBar(const IToolBar&)            = delete;
    IToolBar(IToolBar&&)                 = delete;
    IToolBar& operator=(const IToolBar&) = delete;
    IToolBar& operator=(IToolBar&&)      = delete;

    virtual void addTool(IRenderer::TextureID texture_id, IToolManager::ATool::ID tool_id)  = 0;
    virtual void removeTool(IToolManager::ATool::ID tool_id) = 0;
}; // IToolBar    
   
#endif // I_TOOL_BAR

