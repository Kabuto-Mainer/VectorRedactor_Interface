#pragma once

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolManager.hpp"
#include "ToolsConfig.hpp"
#include "Utils/ACommand.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"
#include "export/Components.hpp"
#include "export/ICamera.hpp"
#include "export/IPalette.hpp"
#include "export/IUIFramework.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace Kabuto
{

namespace Tools
{

/**
 * Creates a shape by dragging on the canvas.
 *
 * The shape type is chosen through a modal context menu. The first left click
 * on the canvas opens the menu; a right click opens it again so that the type
 * can be changed at any moment. The menu closes itself on an outside click,
 * and selection closes it through a deferred popModal().
 *
 * Cursor positions are converted with ICamera, so dragging works the same way
 * on a shifted or zoomed view.
 */
class AddShapeTool final : public IToolManager::ATool
{
public:
    static constexpr IToolManager::ATool::ID ID = 3;

    using Factory = std::function<std::unique_ptr<IDocument::TShape>(const Vec2F& size)>;

    struct ShapeOption
    {
        std::string                         label;
        std::optional<IRenderer::TextureID> icon;
        Factory                             factory;
    }; // struct ShapeOption

    AddShapeTool(IDocument& document, IPalette& palette, ICamera& camera, std::vector<ShapeOption> options,
                 AddShapeSettings settings = {});
    ~AddShapeTool() override = default;

    AddShapeTool(const AddShapeTool&)            = delete;
    AddShapeTool(AddShapeTool&&)                 = delete;
    AddShapeTool& operator=(const AddShapeTool&) = delete;
    AddShapeTool& operator=(AddShapeTool&&)      = delete;

    IToolManager::ATool::ID id() const override;
    bool                    processEvent(const IInputPump::Event& event) override;
    /**
     * Removes everything the tool has added on top of the document: the shape
     * menu and an unfinished dragged shape. Called by the plugin when another
     * tool becomes active.
     */
    void deactivate();

private:
    static constexpr float CENTER_FACTOR = 0.5F;

    class PopModalCommand final : public ACommand
    {
    public:
        void execute() const override
        {
            ApplicationCore::instance().popModal();
        }
    }; // class PopModalCommand

    static void popModalAfterEvent();

    bool beginDrag(const Vec2F& global_pos);
    bool updateDrag(const Vec2F& global_pos);
    bool endDrag(const Vec2F& global_pos);
    void cancelDrag();

    bool openShapeMenu(const Vec2F& global_pos);
    void closeShapeMenu();
    void createShape(const Vec2F& center, const Vec2F& size);
    void applyPaletteColor(IDocument::TShape& shape) const;

    IDocument&               document_;
    IPalette&                palette_;
    ICamera&                 camera_;
    std::vector<ShapeOption> options_;
    AddShapeSettings         settings_;
    Factory                  factory_;

    IDocument::TShape* shape_    = nullptr;
    TObject::ID        shape_id_ = 0;
    TObject::ID        menu_id_  = 0;
    Vec2F              anchor_;
    bool               dragging_ = false;
}; // class AddShapeTool

/* ====================== Implementation ====================== */

inline AddShapeTool::AddShapeTool(IDocument& document, IPalette& palette, ICamera& camera,
                                  std::vector<ShapeOption> options, AddShapeSettings settings) :
    document_(document), palette_(palette), camera_(camera), options_(std::move(options)), settings_(settings)
{
}

inline IToolManager::ATool::ID AddShapeTool::id() const
{
    return ID;
}

inline void AddShapeTool::deactivate()
{
    closeShapeMenu();
    cancelDrag();
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
            if (!factory_)
            {
                return openShapeMenu(mouse->cursorPos());
            }
            return beginDrag(mouse->cursorPos());

        case RIGHT_BUTTON_PRESSING:
            if (dragging_)
            {
                cancelDrag();
            }
            return openShapeMenu(mouse->cursorPos());

        case MOTION:
            return updateDrag(mouse->cursorPos());

        case LEFT_BUTTON_RELEASING:
            return endDrag(mouse->cursorPos());

        default:
            return false;
    }
}

inline void AddShapeTool::popModalAfterEvent()
{
    ApplicationCore::instance().pushCommand(std::make_unique<PopModalCommand>());
}

inline bool AddShapeTool::openShapeMenu(const Vec2F& global_pos)
{
    if (options_.empty())
    {
        return false;
    }

    IUIFramework* framework = nullptr;
    if (ApplicationCore::instance().queryInterface(IID_IUIFramework, reinterpret_cast<void**>(&framework)) !=
            ApplicationCore::Error::SUCCESS ||
        framework == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Add shape tool can't find the UI framework");
        return false;
    }

    IUIFramework::ContextMenuDescription description;
    description.min_point = global_pos;
    description.direction = IUIFramework::Direction::VERTICAL;
    description.role      = IUIFramework::Role::DEFAULT;

    std::unique_ptr<IUIFramework::ContextMenu> menu = framework->create(description);
    if (!menu)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Add shape tool can't create the shape menu");
        return false;
    }

    for (std::size_t index = 0; index < options_.size(); ++index)
    {
        IUIFramework::ButtonDescription button;
        button.box = BoxF{
            Vec2F{settings_.menu_item_width, settings_.menu_item_height}
        };
        button.text = options_[index].label;
        if (options_[index].icon.has_value())
        {
            button.icon = IUIFramework::Icon{*options_[index].icon};
        }
        button.role = IUIFramework::Role::DEFAULT;
        button.on_press.callbacks.emplace_back(
            [this, index]
            {
                factory_ = options_[index].factory;
                popModalAfterEvent();
            }
        );

        std::unique_ptr<IUIFramework::Button> item = framework->create(button);
        if (!item)
        {
            ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Add shape tool can't create a shape menu item");
            continue;
        }

        menu->add(std::move(item));
    }

    menu_id_ = menu->id();
    ApplicationCore::instance().pushModal(std::move(menu));
    return true;
}

inline void AddShapeTool::closeShapeMenu()
{
    if (menu_id_ == 0)
    {
        return;
    }

    /* The menu removes itself on an outside click, so the remembered id may
       already be gone by the time the tool is deactivated. */
    if (ApplicationCore::instance().findObject(menu_id_) != nullptr)
    {
        ApplicationCore::instance().removeObject(menu_id_);
    }

    menu_id_ = 0;
}

inline bool AddShapeTool::beginDrag(const Vec2F& global_pos)
{
    anchor_   = global_pos;
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

    const Vec2F min{std::min(anchor_.x(), global_pos.x()), std::min(anchor_.y(), global_pos.y())};
    const Vec2F max{std::max(anchor_.x(), global_pos.x()), std::max(anchor_.y(), global_pos.y())};

    /* The minimal drag size is checked on screen, before the zoom is applied. */
    if (shape_ == nullptr)
    {
        const Vec2F global_size = max - min;
        if (global_size.x() < settings_.min_drag_size || global_size.y() < settings_.min_drag_size)
        {
            return true;
        }
    }

    const Vec2F local_min = camera_.mapGlobalToLocal(min);
    const Vec2F local_max = camera_.mapGlobalToLocal(max);
    const Vec2F size      = local_max - local_min;
    const Vec2F center    = (local_min + local_max) * CENTER_FACTOR;

    if (shape_ == nullptr)
    {
        createShape(center, size);
        if (shape_ == nullptr)
        {
            dragging_ = false;
            return false;
        }

        return true;
    }

    shape_->setScaling(size);
    shape_->setTranslation(center);

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

    const Vec2F global_size{std::abs(global_pos.x() - anchor_.x()), std::abs(global_pos.y() - anchor_.y())};
    if (global_size.x() < settings_.min_drag_size || global_size.y() < settings_.min_drag_size)
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
