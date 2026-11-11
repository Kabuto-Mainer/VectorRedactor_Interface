#ifndef I_TOOL_MANAGER_HPP
#define I_TOOL_MANAGER_HPP

#include "Interfaces/IID.hpp"
#include "Interfaces/ITool.hpp"

class IToolManager
{

    constexpr static const IID IID_TOOL_MANAGER = 3;

public:
    IToolManager()          = default;
    virtual ~IToolManager() = default;

    IToolManager(const IToolManager&)            = delete;
    IToolManager(IToolManager&&)                 = delete;
    IToolManager& operator=(const IToolManager&) = delete;
    IToolManager& operator=(IToolManager&&)      = delete;

    enum class Error;

    virtual Error  addTool(ITool* tool)          = 0;
    virtual Error  removeTool(ToolID* tool_id)   = 0;
    virtual ITool* activeTool() const            = 0;
    virtual Error  setActiveTool(ToolID tool_id) = 0;
};

enum class IToolManager::Error
{
    SUCCESS,
    INVALID_ARGS,

    TOOL_NOT_FOUND,
};

#endif
