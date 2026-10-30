#ifndef TOOLMANAGER_HPP
#define TOOLMANAGER_HPP

#include "Core/ApplicationCore.hpp"
#include "Interfaces/ITool.hpp"
#include "Interfaces/IUnknown.hpp"

#include <memory>
#include <string>
#include <unordered_map>
// forward-declaration
class Event;

// for now ToolManager doesn't own a tool
// so it doesn't track its lifetime
// that's why there could be some issues when the plugin is unloaded
// and the tool is still being used (nullptr reference)
// TODO: find a proper way how to track this
// or maybe DGAF

class ToolManager
{
public: // constructors && destructors
    ToolManager(ApplicationCore& core) : core_(core)
    {
    }
    // delete copy && move semantics
    // cuz application core owns it
    ToolManager(ToolManager&&)                 = delete;
    ToolManager& operator=(const ToolManager&) = delete;
    ToolManager& operator=(ToolManager&&)      = delete;

public: // getters && setters
    bool setActiveTool(const IID& id)
    {
        void* pv = nullptr;
        if (core.getInterface(id, &pv) != S_OK || !pv)
        {
            return false;
        }
        auto tool = static_cast<ITool*>(pv);
        if (tool == active_tool_)
        {
            tool->release();
            return true;
        }

        if (active_tool_) active_tool_->deactivate();

        active_tool_ = tool;
        active_tool_->activate();
        return true;
    }

    ITool* getActiveTool() const
    {
        return active_tool_;
    }
    bool processEvent(const Event& event)
    {
        if (active_tool_)
        {
            return active_tool_->processEvent(event);
        }
        return false;
    }
    void clear() noexcept
    {
        if (active_tool_)
        {
            active_tool_->deactivate();
            active_tool_->release();
            active_tool_ = nullptr;
        }
    }

private:
    ApplicationCore& core_;
    ITool*           active_tool_ = nullptr;
};

#endif
