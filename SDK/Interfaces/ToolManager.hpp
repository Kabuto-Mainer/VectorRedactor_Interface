#ifndef TOOLMANAGER_HPP
#define TOOLMANAGER_HPP

#include "Interfaces/IID.hpp"
#include "Utils/Base/AObject.hpp"

class IToolManager
{
public:
    IToolManager()          = default;
    virtual ~IToolManager() = default;

    enum class Error;

    /* FIXME:
     * addTool should take AButton instead AObject,
     * but UIFramework isn't implemented for now
     */
    virtual Error addTool(AObject* tool) = 0;
    virtual void* activeTool() const     = 0;
    virtual Error setActiveTool(IID iid) = 0;
};

enum class IToolManager::Error
{
    SUCCESS,
    INVALID_ARGS,

    TOOL_NOT_FOUND,
};

#endif
