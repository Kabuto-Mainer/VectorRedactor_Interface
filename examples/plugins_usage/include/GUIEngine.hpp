#pragma once

#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/Package.hpp"
#include "Utils/Event.hpp"
#include "Utils/Vec2F.hpp"

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <optional>

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

    ResourceID loadTexture(const char* path) override;
    FontID     loadFont(const char* filename) override;

    void renderRect(const Color& color, const BoxF& box, float angle) override;
    void renderRectLines(const Color& color, const BoxF& box, float angle) override;
    void renderTexture(const ResourceID& texture, const BoxF& box, float angle) override;
    void renderEllipse(const Color& color, const BoxF& box) override;
    void renderCircle(const Color& color, const BoxF& box) override;
    void renderEllipseLines(const Color& color, const BoxF& box) override;
    void renderLine(const Vec2F& origin, const Vec2F& end, const Color& color) override;
    void renderVector(const Vec2F& origin, const Vec2F& vector, const Color& color) override;
    void renderPixel(const Vec2F& pos, const Color& color) override;

    void update() override;
    void clear() override;
    void renderText(const char* text, const Color& color, const BoxF& box, const FontID& font_id,
                    int font_size) override;

    /* from IInputPump */

    std::optional<Event> pollEvent() override;
};
