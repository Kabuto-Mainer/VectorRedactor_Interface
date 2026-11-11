#pragma once

#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/Package.hpp"
#include "Utils/Event.hpp"
#include "Utils/Vec2F.hpp"

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <optional>
#include <string>

extern "C" InitPackageFunc INIT_PACKAGE_FUNC_NAME;

/* NOTE:
 * This singleton is hidden from other, it uses only
 * to provide access to IRenderer and IInputPump
 */
class GUIEngine final : public IRenderer, public IInputPump
{
private:
    GUIEngine();
    ~GUIEngine();

    SDL_Window*   window_   = nullptr;
    SDL_Renderer* renderer_ = nullptr;

    void switchRenderColor(const Color& color);
    void openWindow(const Vec2F& size);
    void closeWindow();

public:
    GUIEngine(const GUIEngine&)            = delete;
    GUIEngine(GUIEngine&&)                 = delete;
    GUIEngine& operator=(const GUIEngine&) = delete;
    GUIEngine& operator=(GUIEngine&&)      = delete;

    static GUIEngine& instance();

    /* from IRenderer */

    std::optional<TextureID> loadTexture(const char* path) override;
    std::optional<FontID>    loadFont(const char* filename) override;
    void                     renderRect(const Color& color, const BoxF& box, float angle = 0) override;
    void                     renderRectLines(const Color& color, const BoxF& box, float angle = 0) override;
    void renderTexture(const TextureID& texture, const BoxF& box, float angle = 0, bool flip_by_oy = false) override;
    void renderEllipse(const Color& color, const BoxF& box, float angle = 0) override;
    void renderEllipseLines(const Color& color, const BoxF& box, float angle = 0) override;
    void renderLine(const Vec2F& origin, const Vec2F& end, const Color& color) override;
    void renderVector(const Vec2F& origin, const Vec2F& vector, const Color& color) override;
    void renderPixel(const Vec2F& pos, const Color& color) override;

    void update() override;
    void clear() override;
    void renderText(const std::string& text, const Color& color, const BoxF& box, float angle, const FontID& font_id,
                    int font_size, bool flip_by_oy = false) override;
    void renderTextPro(const std::string& text, const Color& color, const BoxF& box, const TextFormat& format,
                       float angle = 0, bool flip_on_oy = false) override;

    /* from IInputPump */

    std::optional<Event> pollEvent() override;
};
