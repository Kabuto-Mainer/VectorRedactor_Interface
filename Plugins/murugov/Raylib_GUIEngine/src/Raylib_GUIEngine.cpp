#include "Raylib_GUIEngine.hpp"

#include "Core/ApplicationCore.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace Murugov
{

namespace Engine
{
    
namespace
{
constexpr float RAD_TO_DEG = 180.0F / std::numbers::pi_v<float>;

::Color toRL(const IRenderer::Color& c)
{
    return ::Color{c.r, c.g, c.b, c.a};
}

Vec2F rotateVec(Vec2F v, float angle)
{
    float c = std::cos(angle);
    float s = std::sin(angle);
    return {v.x() * c - v.y() * s, v.x() * s + v.y() * c};
}
} // namespace

Raylib_GUIEngine::Raylib_GUIEngine()
{
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1600, 900, "Vectoria");
    if (!IsWindowReady())
    {
        return;
    }

    SetExitKey(KEY_NULL);
    initialized_ = true;

    BeginDrawing();
    ClearBackground(toRL(Color::Black));
    is_drawing_ = true;
}

Raylib_GUIEngine::~Raylib_GUIEngine()
{
    if (is_drawing_)
    {
        EndDrawing();
    }
    for (auto& [id, tex] : textures_)
    {
        UnloadTexture(tex);
    }
    for (auto& [id, font] : fonts_)
    {
        UnloadFont(font);
    }
    if (IsWindowReady())
    {
        CloseWindow();
    }
}

Raylib_GUIEngine& Raylib_GUIEngine::instance()
{
    static Raylib_GUIEngine inst;
    return inst;
}

bool Raylib_GUIEngine::isValid() const
{
    return initialized_;
}

std::optional<IRenderer::TextureID> Raylib_GUIEngine::loadTexture(const std::string& filename)
{
    Texture2D tex = LoadTexture(filename.c_str());
    if (tex.id == 0)
    {
        return std::nullopt;
    }
    TextureID id  = next_tex_++;
    textures_[id] = tex;
    return id;
}

std::optional<IRenderer::FontID> Raylib_GUIEngine::loadFont(const std::string& filename)
{
    ::Font font = LoadFontEx(filename.c_str(), 64, nullptr, 0);
    if (font.texture.id == 0)
    {
        return std::nullopt;
    }
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    FontID id  = next_font_++;
    fonts_[id] = font;
    return id;
}

void Raylib_GUIEngine::renderRect(const Color& color, const BoxF& box, float angle)
{
    Vec2F     ctr  = box.center();
    Vec2F     sz   = box.size();
    Rectangle rect = {ctr.x(), ctr.y(), sz.x(), sz.y()};
    DrawRectanglePro(rect, {sz.x() * 0.5F, sz.y() * 0.5F}, angle * RAD_TO_DEG, toRL(color));
}

void Raylib_GUIEngine::renderRectLines(const Color& color, const BoxF& box, float angle)
{
    if (angle == 0.0F)
    {
        DrawRectangleLinesEx({box.left(), box.top(), box.size().x(), box.size().y()}, 1.0F, toRL(color));
        return;
    }
    Vec2F c      = box.center();
    Vec2F pts[4] = {
        c + rotateVec(box.topLeft() - c, angle),
        c + rotateVec(box.topRight() - c, angle),
        c + rotateVec(box.bottomRight() - c, angle),
        c + rotateVec(box.bottomLeft() - c, angle),
    };
    for (int i = 0; i < 4; ++i)
    {
        DrawLineV({pts[i].x(), pts[i].y()}, {pts[(i + 1) % 4].x(), pts[(i + 1) % 4].y()}, toRL(color));
    }
}

void Raylib_GUIEngine::renderTexture(const TextureID& texture, const BoxF& box, float angle)
{
    auto it = textures_.find(texture);
    if (it == textures_.end())
    {
        return;
    }
    const Texture2D& tex = it->second;
    Vec2F            ctr = box.center();
    Vec2F            sz  = box.size();
    DrawTexturePro(tex, {0.0F, 0.0F, static_cast<float>(tex.width), static_cast<float>(tex.height)},
                   {ctr.x(), ctr.y(), sz.x(), sz.y()}, {sz.x() * 0.5F, sz.y() * 0.5F}, angle * RAD_TO_DEG,
                   toRL(Color::White));
}

void Raylib_GUIEngine::renderEllipse(const Color& color, const BoxF& box, float angle)
{
    Vec2F c = box.center();
    Vec2F r = box.size() * 0.5F;
    if (angle == 0.0F)
    {
        DrawEllipse(static_cast<int>(c.x()), static_cast<int>(c.y()), r.x(), r.y(), toRL(color));
        return;
    }

    constexpr int SEGMENTS = 72;
    ::Color       rl_col   = toRL(color);
    Vec2F         prev     = c + rotateVec({r.x(), 0.0F}, angle);

    for (int i = 1; i <= SEGMENTS; ++i)
    {
        float theta = 2.0F * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(SEGMENTS);
        Vec2F curr  = c + rotateVec({std::cos(theta) * r.x(), std::sin(theta) * r.y()}, angle);
        DrawTriangle({c.x(), c.y()}, {curr.x(), curr.y()}, {prev.x(), prev.y()}, rl_col);
        prev = curr;
    }
}

void Raylib_GUIEngine::renderEllipseLines(const Color& color, const BoxF& box, float angle)
{
    Vec2F c = box.center();
    Vec2F r = box.size() * 0.5F;
    if (angle == 0.0F)
    {
        DrawEllipseLines(static_cast<int>(c.x()), static_cast<int>(c.y()), r.x(), r.y(), toRL(color));
        return;
    }

    constexpr int SEGMENTS = 72;
    ::Color       rl_col   = toRL(color);
    Vec2F         prev     = c + rotateVec({r.x(), 0.0F}, angle);

    for (int i = 1; i <= SEGMENTS; ++i)
    {
        float theta = 2.0F * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(SEGMENTS);
        Vec2F curr  = c + rotateVec({std::cos(theta) * r.x(), std::sin(theta) * r.y()}, angle);
        DrawLineV({prev.x(), prev.y()}, {curr.x(), curr.y()}, rl_col);
        prev = curr;
    }
}

void Raylib_GUIEngine::renderLine(const Color& color, const Vec2F& origin, const Vec2F& end)
{
    DrawLineV({origin.x(), origin.y()}, {end.x(), end.y()}, toRL(color));
}

void Raylib_GUIEngine::renderVector(const Color& color, const Vec2F& origin, const Vec2F& vector)
{
    Vec2F end = origin + vector;
    renderLine(color, origin, end);
    float len = vector.len();
    if (len > 0.0F)
    {
        Vec2F dir  = vector / len;
        float head = std::min(10.0F, len);
        renderLine(color, end, end - rotateVec(dir, 0.436F) * head);
        renderLine(color, end, end - rotateVec(dir, -0.436F) * head);
    }
}

void Raylib_GUIEngine::renderPixel(const Color& color, const Vec2F& pos)
{
    DrawPixelV({pos.x(), pos.y()}, toRL(color));
}

void Raylib_GUIEngine::renderText(const Color& color, const BoxF& box, const std::string& text, const TextFormat& fmt,
                                  float angle)
{
    if (text.empty() || fmt.font_size <= 0)
    {
        return;
    }

    ::Font  font = fonts_.count(fmt.font) ? fonts_[fmt.font] : GetFontDefault();
    float   size = static_cast<float>(fmt.font_size);
    Vector2 dim  = MeasureTextEx(font, text.c_str(), size, 1.0F);

    float x = box.left() + fmt.padding;
    float y = box.top() + fmt.padding;
    float w = box.size().x() - 2.0F * fmt.padding;
    float h = box.size().y() - 2.0F * fmt.padding;

    if (fmt.h_align == TextHAlign::CENTER)
    {
        x += (w - dim.x) * 0.5F;
    }
    else if (fmt.h_align == TextHAlign::RIGHT)
    {
        x += w - dim.x;
    }

    if (fmt.w_align == TextWAlign::CENTER)
    {
        y += (h - dim.y) * 0.5F;
    }
    else if (fmt.w_align == TextWAlign::BOTTOM)
    {
        y += h - dim.y;
    }

    BeginScissorMode(static_cast<int>(box.left()), static_cast<int>(box.top()), static_cast<int>(box.size().x()),
                     static_cast<int>(box.size().y()));
    DrawTextPro(font, text.c_str(), {x, y}, {0.0F, 0.0F}, angle * RAD_TO_DEG, size, 1.0F, toRL(color));
    EndScissorMode();
}

void Raylib_GUIEngine::update()
{
    EndDrawing();
    pollSystemEvents();
    BeginDrawing();
    ClearBackground(toRL(Color::Black));
}

void Raylib_GUIEngine::clear()
{
    ClearBackground(toRL(Color::Black));
}

void Raylib_GUIEngine::pollSystemEvents()
{
    if (WindowShouldClose() && !quit_sent_)
    {
        quit_sent_ = true;
        events_.emplace(EventQuit());
    }

    Vector2 m = GetMousePosition();
    if (m.x != last_mouse_.x || m.y != last_mouse_.y)
    {
        last_mouse_ = m;
        events_.emplace(EventMouse(EventMouse::Type::MOTION, {m.x, m.y}));
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        events_.emplace(EventMouse(EventMouse::Type::LEFT_BUTTON_PRESSING, {m.x, m.y}));
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
    {
        events_.emplace(EventMouse(EventMouse::Type::LEFT_BUTTON_RELEASING, {m.x, m.y}));
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
    {
        events_.emplace(EventMouse(EventMouse::Type::RIGHT_BUTTON_PRESSING, {m.x, m.y}));
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT))
    {
        events_.emplace(EventMouse(EventMouse::Type::RIGHT_BUTTON_RELEASING, {m.x, m.y}));
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0.0F)
    {
        events_.emplace(
            EventMouse(wheel > 0.0F ? EventMouse::Type::WHEEL_FORWARD : EventMouse::Type::WHEEL_BACKWARD, {m.x, m.y}));
    }

    if (IsKeyPressed(KEY_BACKSPACE))
    {
        events_.emplace(EventKeyboard(EventKeyboard::Type::KEY_DOWN, EventKeyboard::Key::BACKSPACE));
    }
    if (IsKeyPressed(KEY_DELETE))
    {
        events_.emplace(EventKeyboard(EventKeyboard::Type::KEY_DOWN, EventKeyboard::Key::DEL));
    }
    if (IsKeyPressed(KEY_ESCAPE))
    {
        events_.emplace(EventKeyboard(EventKeyboard::Type::KEY_DOWN, EventKeyboard::Key::ESC));
    }
}

std::optional<IInputPump::Event> Raylib_GUIEngine::pollEvent()
{
    if (events_.empty())
    {
        return std::nullopt;
    }

    Event ev = std::move(events_.front());
    events_.pop();
    return ev;
}

} // namespace Engine

} // namespace Murugov

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    auto& engine = Murugov::Engine::Raylib_GUIEngine::instance();
    if (!engine.isValid())
    {
        return false;
    }

    return ApplicationCore::instance().registerInterface(IID_RENDERER, static_cast<IRenderer*>(&engine)) ==
               ApplicationCore::Error::SUCCESS &&
           ApplicationCore::instance().registerInterface(IID_INPUT_PUMP, static_cast<IInputPump*>(&engine)) ==
               ApplicationCore::Error::SUCCESS;
}