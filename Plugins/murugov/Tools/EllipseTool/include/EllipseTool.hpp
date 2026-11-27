#pragma once

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolManager.hpp"
#include "../../Palette/IMurugovPalette.hpp"
#include "export/EllipseShape.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <variant>

namespace Murugov
{

namespace Tools
{

constexpr IToolManager::ATool::ID ELLIPSE_TOOL_ID = 10;

/* ====================== EllipseTool ====================== */

class EllipseTool final : public IToolManager::ATool
{
public:
    EllipseTool(ID tool_id, IDocument& document, IRenderer& renderer);
    ~EllipseTool() override = default;

    EllipseTool(const EllipseTool&)            = delete;
    EllipseTool(EllipseTool&&)                 = delete;
    EllipseTool& operator=(const EllipseTool&) = delete;
    EllipseTool& operator=(EllipseTool&&)      = delete;

    ID   id() const override { return id_; }
    bool processEvent(const IInputPump::Event& event) override;
    void render();

private:
    static constexpr float MIN_ELLIPSE_SIZE = 4.0F;

    Vec2F            toDocLocal(const Vec2F& global_pos) const;
    Vec2F            toDocGlobal(const Vec2F& local_pos) const;
    IRenderer::Color currentColor() const;
    
    ID         id_;
    IDocument& document_;
    IRenderer& renderer_;

    bool  is_drawing_ = false;
    Vec2F start_local_;
    Vec2F current_local_;
};

/* ====================== Implementation ====================== */

inline EllipseTool::EllipseTool(ID tool_id, IDocument& document, IRenderer& renderer) :
    id_(tool_id), document_(document), renderer_(renderer)
{
}

inline Vec2F EllipseTool::toDocLocal(const Vec2F& global_pos) const
{
    if (const auto* node = dynamic_cast<const TObject*>(&document_))
    {
        return node->mapGlobalToLocal(global_pos);
    }
    return global_pos;
}

inline Vec2F EllipseTool::toDocGlobal(const Vec2F& local_pos) const
{
    if (const auto* node = dynamic_cast<const TObject*>(&document_))
    {
        return node->mapLocalToGlobal(local_pos);
    }
    return local_pos;
}

inline IRenderer::Color EllipseTool::currentColor() const
{
    IPalette* palette = nullptr;
    if (ApplicationCore::instance().queryInterface(Murugov::IID_PALETTE, reinterpret_cast<void**>(&palette)) ==
            ApplicationCore::Error::SUCCESS &&
        palette != nullptr)
    {
        return palette->selectedColor();
    }
    return IRenderer::Color::WHITE;
}

inline bool EllipseTool::processEvent(const IInputPump::Event& event)
{
    if (const auto* key = std::get_if<IInputPump::EventKeyboard>(&event))
    {
        if (key->type() == IInputPump::EventKeyboard::Type::KEY_DOWN &&
            key->key() == IInputPump::EventKeyboard::Key::ESC && is_drawing_)
        {
            is_drawing_ = false;
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
        {
            is_drawing_    = true;
            start_local_   = toDocLocal(mouse->cursorPos());
            current_local_ = start_local_;
            return true;
        }

        case MOTION:
        {
            if (!is_drawing_)
            {
                return false;
            }
            current_local_ = toDocLocal(mouse->cursorPos());
            return true;
        }

        case LEFT_BUTTON_RELEASING:
        {
            if (!is_drawing_)
            {
                return false;
            }
            is_drawing_ = false;

            const float min_x = std::min(start_local_.x(), current_local_.x());
            const float max_x = std::max(start_local_.x(), current_local_.x());
            const float min_y = std::min(start_local_.y(), current_local_.y());
            const float max_y = std::max(start_local_.y(), current_local_.y());

            const float w = max_x - min_x;
            const float h = max_y - min_y;

            if (w >= MIN_ELLIPSE_SIZE && h >= MIN_ELLIPSE_SIZE)
            {
                auto shape = std::make_unique<Kabuto::Document::EllipseShape>(Vec2F{w, h});
                shape->setTranslation({(min_x + max_x) * 0.5F, (min_y + max_y) * 0.5F});
                shape->setColor(currentColor());
                document_.addShape(std::move(shape));
            }

            return true;
        }

        default:
            return false;
    }
}

inline void EllipseTool::render()
{
    if (!is_drawing_)
    {
        return;
    }

    const Vec2F g1 = toDocGlobal(start_local_);
    const Vec2F g2 = toDocGlobal(current_local_);

    const BoxF preview(std::min(g1.x(), g2.x()), std::min(g1.y(), g2.y()),
                       std::max(g1.x(), g2.x()), std::max(g1.y(), g2.y()));

    renderer_.renderEllipse(currentColor(), preview);
    renderer_.renderEllipseLines(currentColor(), preview);
}

} // namespace Tools

} // namespace Murugov