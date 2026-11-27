#pragma once

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IInputPump.hpp"
#include "../../Palette/IMurugovPalette.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <variant>

namespace Murugov
{

namespace Tools
{

constexpr IToolManager::ATool::ID TEXT_TOOL_ID = 12;

class TextColorComponent : public IDocument::AShapeComponent
{
public:
    constexpr static const ID COID = 4;

    IRenderer::Color color() const { return color_; }
    void             setColor(const IRenderer::Color& color) { color_ = color; }

protected:
    IRenderer::Color color_ = IRenderer::Color::WHITE;
};

class TextFormatComponent : public IDocument::AShapeComponent
{
public:
    constexpr static const ID COID = 6;

    const IRenderer::TextFormat& getFormat() const { return format_; }
    void                         setFormat(const IRenderer::TextFormat& format) { format_ = format; }

protected:
    IRenderer::TextFormat format_{
        .font            = 0,
        .font_size       = 28,
        .overflow_action = IRenderer::TextOverflowAction::FONTSIZELESS,
        .h_align         = IRenderer::TextHAlign::LEFT,
        .w_align         = IRenderer::TextWAlign::TOP,
        .padding         = 4.0F,
        .line_spacing    = 1.2F,
        .wrap            = false,
    };
};

/* ====================== TextShape ====================== */

class TextShape final : public IDocument::TShape, public TextColorComponent, public TextFormatComponent
{
public:
    static constexpr IDocument::TShape::TypeID TEXT_SHAPE_ID = 105;
    static constexpr float                     BASE_HEIGHT   = 36.0F;
    static constexpr float                     MIN_WIDTH     = 32.0F;

    TextShape(const std::string& text, IRenderer::FontID font_id, IRenderer::Color color = IRenderer::Color::WHITE);
    ~TextShape() override = default;

    const std::string& text() const { return text_; }
    void               setText(const std::string& text);

    bool processEvent(const IInputPump::Event& event) override;
    void render() override;

    bool   hitTest(Vec2F const& pos) const override;
    BoxF   axesAlignedBoundingBox() const override;
    TypeID tid() const override { return TEXT_SHAPE_ID; }

    Error queryComponent(IDocument::AShapeComponent::ID coid, IDocument::AShapeComponent** component) override;
    Error queryComponent(IDocument::AShapeComponent::ID     coid,
                         IDocument::AShapeComponent const** component) const override;

private:
    void recalculateBounds();

    std::string text_;
};

/* ====================== TextTool ====================== */

class TextTool final : public IToolManager::ATool
{
public:
    TextTool(ID tool_id, IDocument& document, IRenderer& renderer, IRenderer::FontID font_id);
    ~TextTool() override = default;

    TextTool(const TextTool&)            = delete;
    TextTool(TextTool&&)                 = delete;
    TextTool& operator=(const TextTool&) = delete;
    TextTool& operator=(TextTool&&)      = delete;

    ID   id() const override { return id_; }
    bool processEvent(const IInputPump::Event& event) override;
    void render();

    void finishEditing();

private:
    Vec2F            toDocLocal(const Vec2F& global_pos) const;
    Vec2F            toDocGlobal(const Vec2F& local_pos) const;
    IRenderer::Color currentColor() const;

    TextShape* activeShape() const;
    void       insertChar(char ch);
    void       handleBackspace();
    void       handleDelete();

    static std::optional<char> keyToChar(IInputPump::EventKeyboard key);

    ID                id_;
    IDocument&        document_;
    IRenderer&        renderer_;
    IRenderer::FontID font_id_ = 0;

    TObject::ID editing_shape_id_ = 0;
    std::size_t caret_pos_        = 0;
};

/* ====================== Implementation ====================== */

inline TextShape::TextShape(const std::string& text, IRenderer::FontID font_id, IRenderer::Color color)
    : text_(text)
{
    setColor(color);
    format_.font = font_id;
    setScaling({MIN_WIDTH, BASE_HEIGHT});
    recalculateBounds();
}

inline void TextShape::setText(const std::string& text)
{
    text_ = text;
    recalculateBounds();
}

inline void TextShape::recalculateBounds()
{
    int         line_count = 1;
    std::size_t max_chars  = 0;
    std::size_t curr_chars = 0;

    for (char c : text_)
    {
        if (c == '\n')
        {
            ++line_count;
            max_chars  = std::max(max_chars, curr_chars);
            curr_chars = 0;
        }
        else
        {
            ++curr_chars;
        }
    }
    max_chars = std::max(max_chars, curr_chars);

    const float cur_h       = std::max(std::abs(scaling().y()), 16.0F);
    const float line_h      = cur_h / static_cast<float>(std::max(line_count, 1));
    const float char_w      = line_h * 0.38F;
    const float desired_w   = std::max(MIN_WIDTH, static_cast<float>(max_chars) * char_w + 16.0F);
    const float desired_h   = static_cast<float>(line_count) * std::max(line_h, 24.0F);

    const Vec2F old_tl = transform().transformPoint({-0.5F, -0.5F});
    setScaling({desired_w, desired_h});
    const Vec2F new_tl = transform().transformPoint({-0.5F, -0.5F});

    setTranslation(translation() + (old_tl - new_tl));
}

inline bool TextShape::processEvent(const IInputPump::Event& /*event*/)
{
    return false;
}

inline void TextShape::render()
{
    if (text_.empty())
    {
        return;
    }

    IRenderer* renderer = nullptr;
    if (ApplicationCore::instance().queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer)) !=
            ApplicationCore::Error::SUCCESS ||
        renderer == nullptr)
    {
        return;
    }

    int line_count = 1;
    for (char c : text_)
    {
        if (c == '\n')
        {
            ++line_count;
        }
    }

    const float w = std::abs(scaling().x());
    const float h = std::abs(scaling().y());

    IRenderer::TextFormat fmt = format_;
    fmt.font_size             = std::max(8, static_cast<int>((h / static_cast<float>(line_count)) * 0.78F));

    BoxF box(Vec2F{w, h});
    box.setCenter(translation());

    renderer->renderText(color(), box, text_, fmt, rotation());
}

inline bool TextShape::hitTest(Vec2F const& pos) const
{
    return BoxF{-0.5F, -0.5F, 0.5F, 0.5F}.contains(mapGlobalToLocal(pos));
}

inline BoxF TextShape::axesAlignedBoundingBox() const
{
    const Vec2F v1 = transform().transformPoint({-0.5F, -0.5F});
    const Vec2F v2 = transform().transformPoint({-0.5F, +0.5F});
    const Vec2F v3 = transform().transformPoint({+0.5F, -0.5F});
    const Vec2F v4 = transform().transformPoint({+0.5F, +0.5F});

    return BoxF{std::min({v1.x(), v2.x(), v3.x(), v4.x()}), std::min({v1.y(), v2.y(), v3.y(), v4.y()}),
                std::max({v1.x(), v2.x(), v3.x(), v4.x()}), std::max({v1.y(), v2.y(), v3.y(), v4.y()})};
}

inline IDocument::TShape::Error TextShape::queryComponent(IDocument::AShapeComponent::ID coid,
                                                          IDocument::AShapeComponent**   component)
{
    if (component == nullptr)
    {
        return Error::INVALID_ARGS;
    }

    switch (coid)
    {
        case TextColorComponent::COID:
            *component = static_cast<TextColorComponent*>(this);
            return Error::SUCCESS;

        case TextFormatComponent::COID:
            *component = static_cast<TextFormatComponent*>(this);
            return Error::SUCCESS;

        default:
            *component = nullptr;
            return Error::COMPONENT_NOT_FOUND;
    }
}

inline IDocument::TShape::Error TextShape::queryComponent(IDocument::AShapeComponent::ID     coid,
                                                          IDocument::AShapeComponent const** component) const
{
    if (component == nullptr)
    {
        return Error::INVALID_ARGS;
    }

    switch (coid)
    {
        case TextColorComponent::COID:
            *component = static_cast<const TextColorComponent*>(this);
            return Error::SUCCESS;

        case TextFormatComponent::COID:
            *component = static_cast<const TextFormatComponent*>(this);
            return Error::SUCCESS;

        default:
            *component = nullptr;
            return Error::COMPONENT_NOT_FOUND;
    }
}

/* ====================== TextTool Implementation ====================== */

inline TextTool::TextTool(ID tool_id, IDocument& document, IRenderer& renderer, IRenderer::FontID font_id)
    : id_(tool_id), document_(document), renderer_(renderer), font_id_(font_id)
{
}

inline Vec2F TextTool::toDocLocal(const Vec2F& global_pos) const
{
    if (const auto* node = dynamic_cast<const TObject*>(&document_))
    {
        return node->mapGlobalToLocal(global_pos);
    }
    return global_pos;
}

inline Vec2F TextTool::toDocGlobal(const Vec2F& local_pos) const
{
    if (const auto* node = dynamic_cast<const TObject*>(&document_))
    {
        return node->mapLocalToGlobal(local_pos);
    }
    return local_pos;
}

inline IRenderer::Color TextTool::currentColor() const
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

inline TextShape* TextTool::activeShape() const
{
    if (editing_shape_id_ == 0)
    {
        return nullptr;
    }
    return dynamic_cast<TextShape*>(ApplicationCore::instance().findObject(editing_shape_id_));
}

inline void TextTool::finishEditing()
{
    if (TextShape* shape = activeShape(); shape != nullptr && shape->text().empty())
    {
        document_.removeShape(editing_shape_id_);
    }
    editing_shape_id_ = 0;
    caret_pos_        = 0;
}

inline void TextTool::insertChar(char ch)
{
    TextShape* shape = activeShape();
    if (shape == nullptr)
    {
        return;
    }

    std::string txt = shape->text();
    caret_pos_      = std::min(caret_pos_, txt.size());
    txt.insert(caret_pos_, 1, ch);
    ++caret_pos_;
    shape->setText(txt);
}

inline void TextTool::handleBackspace()
{
    TextShape* shape = activeShape();
    if (shape == nullptr || caret_pos_ == 0)
    {
        return;
    }

    std::string txt = shape->text();
    if (caret_pos_ <= txt.size())
    {
        txt.erase(caret_pos_ - 1, 1);
        --caret_pos_;
        shape->setText(txt);
    }
}

inline void TextTool::handleDelete()
{
    TextShape* shape = activeShape();
    if (shape == nullptr)
    {
        return;
    }

    std::string txt = shape->text();
    if (caret_pos_ < txt.size())
    {
        txt.erase(caret_pos_, 1);
        shape->setText(txt);
    }
}

inline std::optional<char> TextTool::keyToChar(IInputPump::EventKeyboard key)
{
    using Key = IInputPump::EventKeyboard::Key;
    using Mod = IInputPump::EventKeyboard::Modifier;

    const bool shift = key.hasModifiers(Mod::LEFT_SHIFT) || key.hasModifiers(Mod::RIGHT_SHIFT);
    const Key  k     = key.key();

    if (k >= Key::A && k <= Key::Z)
    {
        const int offset = static_cast<int>(k) - static_cast<int>(Key::A);
        return static_cast<char>((shift ? 'A' : 'a') + offset);
    }

    switch (k)
    {
        case Key::NUM_1:        return shift ? '!' : '1';
        case Key::NUM_2:        return shift ? '@' : '2';
        case Key::NUM_3:        return shift ? '#' : '3';
        case Key::NUM_4:        return shift ? '$' : '4';
        case Key::NUM_5:        return shift ? '%' : '5';
        case Key::NUM_6:        return shift ? '^' : '6';
        case Key::NUM_7:        return shift ? '&' : '7';
        case Key::NUM_8:        return shift ? '*' : '8';
        case Key::NUM_9:        return shift ? '(' : '9';
        case Key::NUM_0:        return shift ? ')' : '0';

        case Key::SPACE:        return ' ';
        case Key::TAB:          return '\t';
        case Key::ENTER:        return '\n';

        case Key::MINUS:        return shift ? '_' : '-';
        case Key::EQUALS:       return shift ? '+' : '=';
        case Key::LEFTBRACKET:  return shift ? '{' : '[';
        case Key::RIGHTBRACKET: return shift ? '}' : ']';
        case Key::BACKSLASH:    return shift ? '|' : '\\';
        case Key::NONUSHASH:    return shift ? '~' : '#';
        case Key::SEMICOLON:    return shift ? ':' : ';';
        case Key::APOSTROPHE:   return shift ? '"' : '\'';
        case Key::COMMA:        return shift ? '<' : ',';
        case Key::PERIOD:       return shift ? '>' : '.';
        case Key::SLASH:        return shift ? '?' : '/';

        default:
            return std::nullopt;
    }
}

inline bool TextTool::processEvent(const IInputPump::Event& event)
{
    if (const auto* key = std::get_if<IInputPump::EventKeyboard>(&event))
    {
        if (key->type() != IInputPump::EventKeyboard::Type::KEY_DOWN)
        {
            return false;
        }

        TextShape* shape = activeShape();
        if (shape == nullptr)
        {
            return false;
        }

        using Key = IInputPump::EventKeyboard::Key;
        switch (key->key())
        {
            case Key::ESC:
                finishEditing();
                return true;

            case Key::BACKSPACE:
                handleBackspace();
                return true;

            case Key::DEL:
                handleDelete();
                return true;

            case Key::LEFT:
                if (caret_pos_ > 0)
                {
                    --caret_pos_;
                }
                return true;

            case Key::RIGHT:
                if (caret_pos_ < shape->text().size())
                {
                    ++caret_pos_;
                }
                return true;

            case Key::HOME:
                caret_pos_ = 0;
                return true;

            case Key::END:
                caret_pos_ = shape->text().size();
                return true;

            default:
                if (const auto ch = keyToChar(*key); ch.has_value())
                {
                    insertChar(*ch);
                    return true;
                }
                return false;
        }
    }

    const auto* mouse = std::get_if<IInputPump::EventMouse>(&event);
    if (mouse == nullptr || mouse->type() != IInputPump::EventMouse::Type::LEFT_BUTTON_PRESSING)
    {
        return false;
    }

    const Vec2F global_pos = mouse->cursorPos();
    const Vec2F local_pos  = toDocLocal(global_pos);

    if (const auto hit = document_.hitTest(global_pos); hit.has_value())
    {
        if (auto* clicked_text = dynamic_cast<TextShape*>(ApplicationCore::instance().findObject(*hit)))
        {
            if (editing_shape_id_ != clicked_text->id())
            {
                finishEditing();
                editing_shape_id_ = clicked_text->id();
            }
            caret_pos_ = clicked_text->text().size();
            return true;
        }
    }

    if (activeShape() != nullptr)
    {
        finishEditing();
        return true;
    }

    auto shape = std::make_unique<TextShape>("", font_id_, currentColor());
    shape->setTranslation(local_pos + Vec2F{shape->scaling().x() * 0.5F, shape->scaling().y() * 0.5F});
    editing_shape_id_ = shape->id();
    caret_pos_        = 0;

    document_.addShape(std::move(shape));
    return true;
}

inline void TextTool::render()
{
    TextShape* shape = activeShape();
    if (shape == nullptr)
    {
        return;
    }

    shape->setColor(currentColor());

    const Matrix3F& m  = shape->transform();
    const Vec2F     c1 = toDocGlobal(m.transformPoint({-0.5F, -0.5F}));
    const Vec2F     c2 = toDocGlobal(m.transformPoint({+0.5F, -0.5F}));
    const Vec2F     c3 = toDocGlobal(m.transformPoint({+0.5F, +0.5F}));
    const Vec2F     c4 = toDocGlobal(m.transformPoint({-0.5F, +0.5F}));

    renderer_.renderLine(IRenderer::Color::BLUE, c1, c2);
    renderer_.renderLine(IRenderer::Color::BLUE, c2, c3);
    renderer_.renderLine(IRenderer::Color::BLUE, c3, c4);
    renderer_.renderLine(IRenderer::Color::BLUE, c4, c1);

    const std::string& txt      = shape->text();
    const std::size_t  safe_pos = std::min(caret_pos_, txt.size());

    int         total_lines = 1;
    int         caret_line  = 0;
    std::size_t col_chars   = 0;

    for (std::size_t i = 0; i < txt.size(); ++i)
    {
        if (i == safe_pos)
        {
            caret_line = total_lines - 1;
        }
        if (txt[i] == '\n')
        {
            ++total_lines;
            if (i < safe_pos)
            {
                col_chars = 0;
            }
        }
        else if (i < safe_pos)
        {
            ++col_chars;
        }
    }
    if (safe_pos == txt.size())
    {
        caret_line = total_lines - 1;
    }

    const float w      = std::abs(shape->scaling().x());
    const float h      = std::abs(shape->scaling().y());
    const float line_h = h / static_cast<float>(std::max(total_lines, 1));
    const float char_w = line_h * 0.38F;

    const float norm_x   = std::clamp(-0.5F + (4.0F + static_cast<float>(col_chars) * char_w) / w, -0.48F, 0.48F);
    const float norm_y1  = -0.5F + (static_cast<float>(caret_line) * line_h + 4.0F) / h;
    const float norm_y2  = -0.5F + (static_cast<float>(caret_line + 1) * line_h - 4.0F) / h;

    const Vec2F caret_top = toDocGlobal(m.transformPoint({norm_x, norm_y1}));
    const Vec2F caret_bot = toDocGlobal(m.transformPoint({norm_x, norm_y2}));
    renderer_.renderLine(IRenderer::Color::WHITE, caret_top, caret_bot);
}

} // namespace Tools

} // namespace Murugov