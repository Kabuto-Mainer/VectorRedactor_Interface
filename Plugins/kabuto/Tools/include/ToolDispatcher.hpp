#pragma once

#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"

namespace Kabuto
{

namespace Tools
{

/**
 * Scene-tree adapter for IToolManager.
 *
 * IToolManager is a service, not a TObject, so events never reach it through
 * the desktop by themselves. The dispatcher is placed near the bottom of the
 * desktop (above the document, below UI panels) and forwards everything that
 * the UI did not consume to the currently active tool.
 */
class ToolDispatcher final : public TObject
{
public:
    explicit ToolDispatcher(IToolManager& tools);
    ~ToolDispatcher() override = default;

    ToolDispatcher(const ToolDispatcher&)            = delete;
    ToolDispatcher(ToolDispatcher&&)                 = delete;
    ToolDispatcher& operator=(const ToolDispatcher&) = delete;
    ToolDispatcher& operator=(ToolDispatcher&&)      = delete;

    bool  processEvent(const IInputPump::Event& event) override;
    void  render() override;
    Vec2F mapLocalToGlobal(const Vec2F& pos) const override;
    Vec2F mapGlobalToLocal(const Vec2F& pos) const override;

private:
    IToolManager& tools_;
}; // class ToolDispatcher

inline ToolDispatcher::ToolDispatcher(IToolManager& tools) : tools_(tools)
{
}

inline bool ToolDispatcher::processEvent(const IInputPump::Event& event)
{
    return tools_.processEvent(event);
}

inline void ToolDispatcher::render()
{
}

inline Vec2F ToolDispatcher::mapLocalToGlobal(const Vec2F& pos) const
{
    return parent() != nullptr ? parent()->mapLocalToGlobal(pos) : pos;
}

inline Vec2F ToolDispatcher::mapGlobalToLocal(const Vec2F& pos) const
{
    return parent() != nullptr ? parent()->mapGlobalToLocal(pos) : pos;
}

}; // namespace Tools

}; // namespace Kabuto
