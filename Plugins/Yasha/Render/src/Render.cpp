#include "Render.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/Package.hpp"

#include <algorithm>
#include <cmath>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{

::Color rayColor(const IRenderer::Color& color)
{
    return {color.r, color.g, color.b, color.a};
}

Rectangle rayRectangle(const BoxF& box)
{
    return {box.minX(), box.minY(), box.size().x(), box.size().y()};
}

Vector2 rayVector(const Vec2F& vector)
{
    return {vector.x(), vector.y()};
}

std::vector<std::string> textLines(const std::string& text, Font font, float size, float width, bool wrap)
{
    std::vector<std::string> lines{""};
    for (std::size_t index = 0; index < text.size();)
    {
        if (text[index] == '\n')
        {
            lines.emplace_back();
            ++index;
            continue;
        }
        int bytes = 0;
        GetCodepointNext(text.c_str() + index, &bytes);
        const std::string character = text.substr(index, static_cast<std::size_t>(std::max(1, bytes)));
        index += character.size();
        std::string candidate = lines.back() + character;
        if (wrap && !lines.back().empty() && MeasureTextEx(font, candidate.c_str(), size, 0).x > width)
        {
            lines.push_back(character);
        }
        else
        {
            lines.back() = std::move(candidate);
        }
    }
    return lines;
}

} // namespace

struct RaylibRender::State
{
    TextureID                                next_texture_id = 1;
    FontID                                   next_font_id    = 1;
    std::unordered_map<TextureID, Texture2D> textures;
    std::unordered_map<FontID, Font>         fonts;
};

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    void* existing = nullptr;
    auto& core     = ApplicationCore::instance();
    if (core.queryInterface(IID_RENDERER, &existing) == ApplicationCore::Error::SUCCESS)
    {
        return false;
    }

    RaylibRender& backend = RaylibRender::instance();
    if (!backend.ready())
    {
        return false;
    }

    return core.registerInterface(IID_RENDERER, static_cast<IRenderer*>(&backend)) == ApplicationCore::Error::SUCCESS;
}

RaylibRender& RaylibRender::instance()
{
    static RaylibRender backend;
    return backend;
}

RaylibRender::RaylibRender() : state_(std::make_unique<State>())
{
    InitWindow(1'000, 1'000, "Vector Redactor");
    ready_ = IsWindowReady();
    if (ready_)
    {
        SetTargetFPS(60);
        BeginDrawing();
        clear();
    }
}

RaylibRender::~RaylibRender()
{
    if (!ready_)
    {
        return;
    }
    for (const auto& [id, texture] : state_->textures)
    {
        (void)id;
        UnloadTexture(texture);
    }
    for (const auto& [id, font] : state_->fonts)
    {
        (void)id;
        UnloadFont(font);
    }
    EndDrawing();
    CloseWindow();
}

std::optional<IRenderer::TextureID> RaylibRender::loadTexture(const std::string& filename)
{
    if (!ready_ || !FileExists(filename.c_str()))
    {
        return std::nullopt;
    }
    Texture2D texture = LoadTexture(filename.c_str());
    if (!IsTextureValid(texture))
    {
        return std::nullopt;
    }
    const TextureID id = state_->next_texture_id++;
    state_->textures.emplace(id, texture);
    return id;
}

std::optional<IRenderer::FontID> RaylibRender::loadFont(const std::string& filename)
{
    if (!ready_ || !FileExists(filename.c_str()))
    {
        return std::nullopt;
    }
    Font font = LoadFont(filename.c_str());
    if (!IsFontValid(font))
    {
        return std::nullopt;
    }
    const FontID id = state_->next_font_id++;
    state_->fonts.emplace(id, font);
    return id;
}

void RaylibRender::renderRect(const IRenderer::Color& color, const BoxF& box, float angle)
{
    DrawRectanglePro(rayRectangle(box), {0, 0}, angle, rayColor(color));
}

void RaylibRender::renderRectLines(const IRenderer::Color& color, const BoxF& box, float angle)
{
    const Rectangle rect      = rayRectangle(box);
    Vector2         corners[] = {
        {             rect.x,               rect.y},
        {rect.x + rect.width,               rect.y},
        {rect.x + rect.width, rect.y + rect.height},
        {             rect.x, rect.y + rect.height}
    };
    if (angle != 0.0F)
    {
        const Vector2 pivot{rect.x, rect.y};
        for (auto& corner : corners)
        {
            corner = Vector2Add(pivot, Vector2Rotate(Vector2Subtract(corner, pivot), angle * DEG2RAD));
        }
    }
    for (int index = 0; index < 4; ++index)
    {
        DrawLineV(corners[index], corners[(index + 1) % 4], rayColor(color));
    }
}

void RaylibRender::renderTexture(const TextureID& texture_id, const BoxF& box, float angle)
{
    const auto it = state_->textures.find(texture_id);
    if (it == state_->textures.end())
    {
        return;
    }
    const Texture2D texture = it->second;
    DrawTexturePro(texture, {0, 0, static_cast<float>(texture.width), static_cast<float>(texture.height)},
                   rayRectangle(box), {0, 0}, angle, ::Color{255, 255, 255, 255});
}

void RaylibRender::renderEllipse(const IRenderer::Color& color, const BoxF& box, float angle)
{
    const Vec2F center = box.center();
    const Vec2F radius = box.size() / 2.0F;
    rlPushMatrix();
    rlTranslatef(center.x(), center.y(), 0.0F);
    rlRotatef(angle, 0.0F, 0.0F, 1.0F);
    DrawEllipse(0, 0, radius.x(), radius.y(), rayColor(color));
    rlPopMatrix();
}

void RaylibRender::renderEllipseLines(const IRenderer::Color& color, const BoxF& box, float angle)
{
    const Vec2F center = box.center();
    const Vec2F radius = box.size() / 2.0F;
    rlPushMatrix();
    rlTranslatef(center.x(), center.y(), 0.0F);
    rlRotatef(angle, 0.0F, 0.0F, 1.0F);
    DrawEllipseLines(0, 0, radius.x(), radius.y(), rayColor(color));
    rlPopMatrix();
}

void RaylibRender::renderLine(const IRenderer::Color& color, const Vec2F& origin, const Vec2F& end)
{
    DrawLineV(rayVector(origin), rayVector(end), rayColor(color));
}

void RaylibRender::renderVector(const IRenderer::Color& color, const Vec2F& origin, const Vec2F& vector)
{
    renderLine(color, origin, origin + vector);
}

void RaylibRender::renderPixel(const IRenderer::Color& color, const Vec2F& pos)
{
    DrawPixelV(rayVector(pos), rayColor(color));
}

void RaylibRender::renderText(const IRenderer::Color& color, const BoxF& box, const std::string& text,
                              const TextFormat& format, float angle)
{
    if (box.size().x() <= 0 || box.size().y() <= 0)
    {
        return;
    }

    Font font = GetFontDefault();
    if (const auto it = state_->fonts.find(format.font); it != state_->fonts.end())
    {
        font = it->second;
    }

    const float padding   = std::max(0.0F, format.padding);
    const float width     = std::max(1.0F, box.size().x() - 2.0F * padding);
    const float height    = std::max(1.0F, box.size().y() - 2.0F * padding);
    float       font_size = static_cast<float>(std::max(1, format.font_size));
    auto        lines     = textLines(text, font, font_size, width, format.wrap);

    if (format.overflow_action == TextOverflowAction::FONTSIZELESS)
    {
        while (font_size > 1.0F)
        {
            float widest = 0;
            for (const auto& line : lines)
            {
                widest = std::max(widest, MeasureTextEx(font, line.c_str(), font_size, 0).x);
            }
            const float line_height = font_size + std::max(0.0F, format.line_spacing);
            if (widest <= width && lines.size() * line_height <= height)
            {
                break;
            }
            font_size -= 1.0F;
            lines = textLines(text, font, font_size, width, format.wrap);
        }
    }

    if (format.overflow_action == TextOverflowAction::TAILREPLACE)
    {
        for (auto& line : lines)
        {
            if (MeasureTextEx(font, line.c_str(), font_size, 0).x <= width)
            {
                continue;
            }
            while (!line.empty() && MeasureTextEx(font, (line + "...").c_str(), font_size, 0).x > width)
            {
                line.pop_back();
                while (!line.empty() && (static_cast<unsigned char>(line.back()) & 0xC0U) == 0x80U)
                {
                    line.pop_back();
                }
            }
            line += "...";
        }
    }

    const float line_height  = font_size + std::max(0.0F, format.line_spacing);
    const float total_height = line_height * static_cast<float>(lines.size());
    float       y            = box.minY() + padding;
    if (format.w_align == TextWAlign::CENTER)
    {
        y += (height - total_height) / 2.0F;
    }
    else if (format.w_align == TextWAlign::BOTTOM)
    {
        y += height - total_height;
    }

    BeginScissorMode(static_cast<int>(box.minX()), static_cast<int>(box.minY()), static_cast<int>(box.size().x()),
                     static_cast<int>(box.size().y()));
    for (const auto& line : lines)
    {
        const float line_width = MeasureTextEx(font, line.c_str(), font_size, 0).x;
        float       x          = box.minX() + padding;
        if (format.h_align == TextHAlign::CENTER)
        {
            x += (width - line_width) / 2.0F;
        }
        else if (format.h_align == TextHAlign::RIGHT)
        {
            x += width - line_width;
        }
        DrawTextPro(font, line.c_str(), {x, y}, {0, 0}, angle, font_size, 0, rayColor(color));
        y += line_height;
    }
    EndScissorMode();
}

void RaylibRender::update()
{
    if (!ready_)
    {
        return;
    }
    EndDrawing();
    BeginDrawing();
    clear();
}

void RaylibRender::clear()
{
    ClearBackground(::Color{30, 30, 30, 255});
}
