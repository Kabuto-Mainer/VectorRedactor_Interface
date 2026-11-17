#pragma once

#include "Components.hpp"
#include "IPalette.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <variant>

namespace Kabuto
{

namespace Tools
{

/**
 * Creates a shape by dragging a rectangle on the canvas.
 *
 * Pressing the left mouse button remembers the anchor, motion resizes a live
 * preview shape, releasing the button commits it. A drag smaller than
 * MIN_DRAG_SIZE is treated as a click and the shape is removed again.
 */
class AddShapeTool final : public IToolManager::ATool
{
public:
    using ShapeFactory = std::function<std::unique_ptr<IDocument::TShape>(const Vec2F& size)>;

    AddShapeTool(ID id, IDocument& document, IPalette& palette, const BoxF& ui_box, ShapeFactory factory);
    ~AddShapeTool() override = default;

    AddShapeTool(const AddShapeTool&)            = delete;
    AddShapeTool(AddShapeTool&&)                 = delete;
    AddShapeTool& operator=(const AddShapeTool&) = delete;
    AddShapeTool& operator=(AddShapeTool&&)      = delete;

    ID   id() const override;
    bool processEvent(const IInputPump::Event& event) override;

private:
    static constexpr float MIN_DRAG_SIZE = 2.0F;

    Vec2F toLocal(const Vec2F& global_pos) const;

    bool beginDrag(const Vec2F& global_pos);
    bool updateDrag(const Vec2F& global_pos);
    bool endDrag(const Vec2F& global_pos);
    void cancelDrag();

    void createShape(const Vec2F& center, const Vec2F& size);
    void applyPaletteColor(IDocument::TShape& shape) const;

private:
    ID           id_;
    IDocument&   document_;
    IPalette&    palette_;
    BoxF         ui_box_;
    ShapeFactory factory_;

    IDocument::TShape* shape_    = nullptr;
    TObject::ID        shape_id_ = 0;
    Vec2F              anchor_;
    bool               dragging_ = false;
}; // class AddShapeTool

/* ====================== Implementation ====================== */

inline AddShapeTool::AddShapeTool(ID id, IDocument& document, IPalette& palette, const BoxF& ui_box,
                                  ShapeFactory factory) :
    id_(id), document_(document), palette_(palette), ui_box_(ui_box), factory_(std::move(factory))
{
}

inline IToolManager::ATool::ID AddShapeTool::id() const
{
    return id_;
}

inline Vec2F AddShapeTool::toLocal(const Vec2F& global_pos) const
{
    if (const auto* node = dynamic_cast<const TObject*>(&document_))
    {
        return node->mapGlobalToLocal(global_pos);
    }

    return global_pos;
}

inline bool AddShapeTool::processEvent(const IInputPump::Event& event)
{
    if (const auto* key = std::get_if<IInputPump::EventKeyboard>(&event))
    {
        if (key->type() == IInputPump::EventKeyboard::Type::KEY_DOWN &&
            key->key() == IInputPump::EventKeyboard::Key::ESC &&
            dragging_)
        {
            cancelDrag();
            return true;
        }

        return false;
    }

    const auto* mouse = std::get_if<IInputPump::EventMouse>(&event);
    if (mouse == nullptr)
    {
        return false;
    }

    using enum IInputPump::EventMouse::Type;

    switch (mouse->type())
    {
        case LEFT_BUTTON_PRESSING:
            return beginDrag(mouse->cursorPos());

        case MOTION:
            return updateDrag(mouse->cursorPos());

        case LEFT_BUTTON_RELEASING:
            return endDrag(mouse->cursorPos());

        default:
            return false;
    }
}

inline bool AddShapeTool::beginDrag(const Vec2F& global_pos)
{
    if (ui_box_.contains(global_pos))
    {
        return false;
    }

    anchor_   = toLocal(global_pos);
    shape_    = nullptr;
    shape_id_ = 0;
    dragging_ = true;

    return true;
}

inline bool AddShapeTool::updateDrag(const Vec2F& global_pos)
{
    if (!dragging_)
    {
        return false;
    }

    const Vec2F current = toLocal(global_pos);
    const Vec2F min{std::min(anchor_.x(), current.x()), std::min(anchor_.y(), current.y())};
    const Vec2F max{std::max(anchor_.x(), current.x()), std::max(anchor_.y(), current.y())};
    const Vec2F size = max - min;

    if (shape_ == nullptr)
    {
        if (size.x() < MIN_DRAG_SIZE || size.y() < MIN_DRAG_SIZE)
        {
            return true;
        }

        createShape((min + max) / 2.0F, size);
        if (shape_ == nullptr)
        {
            dragging_ = false;
            return false;
        }

        return true;
    }

    shape_->setScaling(size);
    shape_->setTranslation((min + max) / 2.0F);

    return true;
}

inline bool AddShapeTool::endDrag(const Vec2F& global_pos)
{
    if (!dragging_)
    {
        return false;
    }

    updateDrag(global_pos);

    if (shape_ == nullptr)
    {
        cancelDrag();
        return true;
    }

    const Vec2F size = shape_->scaling();
    if (std::abs(size.x()) < MIN_DRAG_SIZE || std::abs(size.y()) < MIN_DRAG_SIZE)
    {
        cancelDrag();
        return true;
    }

    shape_    = nullptr;
    shape_id_ = 0;
    dragging_ = false;

    return true;
}

inline void AddShapeTool::cancelDrag()
{
    if (shape_ != nullptr)
    {
        document_.removeShape(shape_id_);
    }

    shape_    = nullptr;
    shape_id_ = 0;
    dragging_ = false;
}

inline void AddShapeTool::createShape(const Vec2F& center, const Vec2F& size)
{
    if (!factory_)
    {
        return;
    }

    std::unique_ptr<IDocument::TShape> shape = factory_(size);
    if (!shape)
    {
        return;
    }

    shape->setTranslation(center);
    applyPaletteColor(*shape);

    IDocument::TShape* raw = shape.get();
    shape_id_              = raw->id();
    document_.addShape(std::move(shape));
    shape_ = raw;
}

inline void AddShapeTool::applyPaletteColor(IDocument::TShape& shape) const
{
    IDocument::AShapeComponent* component = nullptr;
    if (shape.queryComponent(Document::ColorComponent::COID, &component) == IDocument::TShape::Error::SUCCESS &&
        component != nullptr)
    {
        static_cast<Document::ColorComponent*>(component)->setColor(palette_.color());
    }
}

}; // namespace Tools

}; // namespace Kabuto
