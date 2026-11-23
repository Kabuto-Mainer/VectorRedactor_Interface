#pragma once

#include "Interfaces/IDocument.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Matrix3F.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace Murugov
{

namespace Tools
{

constexpr IToolManager::ATool::ID SELECT_TOOL_ID = 5;

class SelectTool final : public IToolManager::ATool
{
public:
    SelectTool(ID id, IDocument& document, IRenderer& renderer, const BoxF& ui_box);
    ~SelectTool() override = default;

    SelectTool(const SelectTool&)            = delete;
    SelectTool(SelectTool&&)                 = delete;
    SelectTool& operator=(const SelectTool&) = delete;
    SelectTool& operator=(SelectTool&&)      = delete;

    ID   id() const override;
    bool processEvent(const IInputPump::Event& event) override;
    void render();

private:
    enum class Mode
    {
        NONE,
        MOVE,
        RESIZE,
        ROTATE,
        BOX_SELECT,
    };

    struct InitialShapeState
    {
        Vec2F translation;
        Vec2F scaling;
        float rotation = 0.0F;
    };

    static constexpr float HANDLE_HALF_SIZE   = 4.5F;
    static constexpr float ROTATE_HANDLE_DIST = 26.0F;
    static constexpr float MIN_SHAPE_SIZE     = 4.0F;

    Vec2F toLocal(const Vec2F& global_pos) const;
    Vec2F toGlobal(const Vec2F& local_pos) const;

    std::vector<const IDocument::TShape*> allShapes() const;
    std::vector<IDocument::TShape*>       selectedShapes() const;
    bool                                  isSelected(TObject::ID id) const;
    void                                  clearSelection();
    void                                  captureInitialStates(const std::vector<IDocument::TShape*>& shapes);

    std::optional<BoxF>  groupBounds(const std::vector<IDocument::TShape*>& shapes) const;
    std::array<Vec2F, 8> selectionHandles(const std::vector<IDocument::TShape*>& shapes) const;
    Vec2F                rotateHandlePos(const std::vector<IDocument::TShape*>& shapes) const;

    bool hitTestPoint(const Vec2F& p, const Vec2F& target, float radius) const;
    int  hitTestResizeHandle(const std::vector<IDocument::TShape*>& shapes, const Vec2F& local_pos) const;

    bool onMouseDown(const Vec2F& global_pos);
    bool onMouseMove(const Vec2F& global_pos);
    bool onMouseUp();
    bool onKeyDown(const IInputPump::EventKeyboard& key);

private:
    ID         id_;
    IDocument& document_;
    IRenderer& renderer_;
    BoxF       ui_box_;

    std::unordered_set<TObject::ID> selected_ids_;
    Mode                            mode_ = Mode::NONE;

    // For the selection marquee
    Vec2F box_select_start_;
    Vec2F box_select_current_;

    // For transforming a single figure or a group
    Vec2F                                              prev_mouse_local_;
    Vec2F                                              fixed_anchor_;
    Vec2F                                              group_center_;
    BoxF                                               initial_group_box_;
    float                                              group_visual_angle_ = 0.0F;
    int                                                active_handle_idx_  = -1;
    std::unordered_map<TObject::ID, InitialShapeState> initial_states_;
}; // class SelectTool

} // namespace Tools
} // namespace Murugov
