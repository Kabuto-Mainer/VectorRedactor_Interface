#pragma once

#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/Package.hpp"
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
class SDL_GUIEngine final : public IRenderer, public IInputPump
{
private:
    SDL_GUIEngine();
    ~SDL_GUIEngine();

    SDL_Window*   window_   = nullptr;
    SDL_Renderer* renderer_ = nullptr;

    void switchRenderColor(const IRenderer::Color& color);
    void openWindow(const Vec2F& size);
    void closeWindow();

public:
    SDL_GUIEngine(const SDL_GUIEngine&)            = delete;
    SDL_GUIEngine(SDL_GUIEngine&&)                 = delete;
    SDL_GUIEngine& operator=(const SDL_GUIEngine&) = delete;
    SDL_GUIEngine& operator=(SDL_GUIEngine&&)      = delete;

    static SDL_GUIEngine& instance();

    /* from IRenderer */

    std::optional<TextureID> loadTexture(const std::string& filename) override;
    std::optional<FontID>    loadFont(const std::string& filename) override;
    void                     renderRect(const IRenderer::Color& color, const BoxF& box, float angle = 0) override;
    void                     renderRectLines(const IRenderer::Color& color, const BoxF& box, float angle = 0) override;
    void                     renderTexture(const TextureID& texture, const BoxF& box, float angle = 0) override;
    void                     renderEllipse(const IRenderer::Color& color, const BoxF& box, float angle = 0) override;
    void renderEllipseLines(const IRenderer::Color& color, const BoxF& box, float angle = 0) override;
    void renderLine(const IRenderer::Color& color, const Vec2F& origin, const Vec2F& end) override;
    void renderVector(const IRenderer::Color& color, const Vec2F& origin, const Vec2F& vector) override;
    void renderPixel(const IRenderer::Color& color, const Vec2F& pos) override;

    void update() override;
    void clear() override;
    void renderText(const IRenderer::Color& color, const BoxF& box, const std::string& text, const TextFormat& format,
                    float angle = 0) override;

    /* from IInputPump */

    std::optional<Event> pollEvent() override;
};
