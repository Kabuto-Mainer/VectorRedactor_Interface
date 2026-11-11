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

    void switchRenderColor(const IRenderer::Color& color);
    void openWindow(const Vec2F& size);
    void closeWindow();

public:
    GUIEngine(const GUIEngine&)            = delete;
    GUIEngine(GUIEngine&&)                 = delete;
    GUIEngine& operator=(const GUIEngine&) = delete;
    GUIEngine& operator=(GUIEngine&&)      = delete;

    static GUIEngine& instance();

    /* from IRenderer */

    std::optional<TextureID> loadTexture(const std::string& filename) override;
    std::optional<FontID>    loadFont(const std::string& filename) override;
    void                     renderRect(const IRenderer::Color& color, const BoxF& box, float angle = 0) override;
    void                     renderRectLines(const IRenderer::Color& color, const BoxF& box, float angle = 0) override;
    void renderTexture(const TextureID& texture, const BoxF& box, float angle = 0) override;
    void renderEllipse(const IRenderer::Color& color, const BoxF& box, float angle = 0) override;
    void renderEllipseLines(const IRenderer::Color& color, const BoxF& box, float angle = 0) override;
    void renderLine(const Vec2F& origin, const Vec2F& end, const IRenderer::Color& color) override;
    void renderVector(const Vec2F& origin, const Vec2F& vector, const IRenderer::Color& color) override;
    void renderPixel(const Vec2F& pos, const IRenderer::Color& color) override;

    void update() override;
    void clear() override;
    void renderText(const std::string& text, const IRenderer::Color& color, const BoxF& box, const TextFormat& format,
                               float angle = 0) override;

    /* from IInputPump */

    std::optional<Event> pollEvent() override;
};
