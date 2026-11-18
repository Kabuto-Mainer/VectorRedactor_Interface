#pragma once

#include "Document.hpp"
#include "ToolManager.hpp"
#include "Utils/TWidget.hpp"

#include <optional>

namespace Yasha::Tools
{

class SelectionOverlay final : public TWidget
{
public:
    SelectionOverlay(Document& document, ToolManager& manager, IToolManager::ATool::ID tool_id);

    bool processEvent(const IInputPump::Event& event) override;
    void render() override;

    std::optional<Vec2F> rotationHandlePosition() const;
    bool hitTestRotationHandle(const Vec2F& global_pos) const;

private:
    static constexpr float HANDLE_OFFSET = 24.0F;
    static constexpr float HANDLE_RADIUS = 6.0F;
    static constexpr float HIT_RADIUS = 10.0F;

    Document& document_;
    ToolManager& manager_;
    IToolManager::ATool::ID tool_id_;
};

} // namespace Yasha::Tools
