#pragma once

#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/Package.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Vec2F.hpp"

#include <optional>
#include <queue>
#include <raylib.h>
#include <string>
#include <unordered_map>

extern "C" InitPackageFunc INIT_PACKAGE_FUNC_NAME;

namespace Murugov
{

namespace Engine
{

class Raylib_GUIEngine final : public IRenderer, public IInputPump
{
private:
    bool      initialized_ = false;
    bool      is_drawing_  = false;
    bool      quit_sent_   = false;
    Vector2   last_mouse_  = {-1.0F, -1.0F};
    TextureID next_tex_    = 1;
    FontID    next_font_   = 1;

    std::queue<Event>                        events_;
    std::unordered_map<TextureID, Texture2D> textures_;
    std::unordered_map<FontID, ::Font>       fonts_;

    Raylib_GUIEngine();
    ~Raylib_GUIEngine() override;

    void pollSystemEvents();

public:
    Raylib_GUIEngine(const Raylib_GUIEngine&)            = delete;
    Raylib_GUIEngine(Raylib_GUIEngine&&)                 = delete;
    Raylib_GUIEngine& operator=(const Raylib_GUIEngine&) = delete;
    Raylib_GUIEngine& operator=(Raylib_GUIEngine&&)      = delete;

    static Raylib_GUIEngine& instance();

    bool isValid() const;

    /* from IRenderer */

    std::optional<TextureID> loadTexture(const std::string& filename) override;
    std::optional<FontID>    loadFont(const std::string& filename) override;

    void renderRect(const Color& color, const BoxF& box, float angle = 0) override;
    void renderRectLines(const Color& color, const BoxF& box, float angle = 0) override;
    void renderTexture(const TextureID& texture, const BoxF& box, float angle = 0) override;
    void renderEllipse(const Color& color, const BoxF& box, float angle = 0) override;
    void renderEllipseLines(const Color& color, const BoxF& box, float angle = 0) override;
    void renderLine(const Color& color, const Vec2F& origin, const Vec2F& end) override;
    void renderVector(const Color& color, const Vec2F& origin, const Vec2F& vector) override;
    void renderPixel(const Color& color, const Vec2F& pos) override;

    void update() override;
    void clear() override;
    void renderText(const Color& color, const BoxF& box, const std::string& text, const TextFormat& format,
                    float angle = 0) override;

    /* from IInputPump */

    std::optional<Event> pollEvent() override;
}; // class Raylib_GUIEngine

} // namespace Engine

} // namespace Murugov
