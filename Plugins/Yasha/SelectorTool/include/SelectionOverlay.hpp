#pragma once

#include "Interfaces/IDocument.hpp"
#include "ToolManager.hpp"
#include "Utils/TWidget.hpp"

#include <optional>

namespace Yasha::Tools
{

struct SelectionState
{
    std::optional<TObject::ID> selected_shape_id;
};

class SelectionOverlay final : public TWidget
{
public:
    SelectionOverlay(IDocument& document, ToolManager& manager, IToolManager::ATool::ID tool_id,
                     SelectionState& state);

    bool processEvent(const IInputPump::Event& event) override;
    void render() override;

    std::optional<Vec2F> rotationHandlePosition() const;
    bool                 hitTestRotationHandle(const Vec2F& global_pos) const;

private:
    static constexpr float HANDLE_OFFSET = 24.0F;
    static constexpr float HANDLE_RADIUS = 6.0F;
    static constexpr float HIT_RADIUS    = 10.0F;

    IDocument&                      document_;
    ToolManager&                    manager_;
    IToolManager::ATool::ID         tool_id_;
    SelectionState&                 state_;
};

} // namespace Yasha::Tools
