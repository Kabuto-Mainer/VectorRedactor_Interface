#pragma once

#include "Interfaces/IRenderer.hpp"

#include <memory>
#include <optional>
#include <string>

class RaylibRender final : public IRenderer
{
public:
    static RaylibRender& instance();

    RaylibRender(const RaylibRender&)            = delete;
    RaylibRender(RaylibRender&&)                 = delete;
    RaylibRender& operator=(const RaylibRender&) = delete;
    RaylibRender& operator=(RaylibRender&&)      = delete;

    // clang-format off
    bool ready() const { return ready_; }
    // clang-format on

    std::optional<TextureID> loadTexture(const std::string& filename) override;
    std::optional<FontID>    loadFont(const std::string& filename) override;

    void renderRect(const IRenderer::Color& color, const BoxF& box, float angle = 0) override;
    void renderRectLines(const IRenderer::Color& color, const BoxF& box, float angle = 0) override;
    void renderTexture(const TextureID& texture, const BoxF& box, float angle = 0) override;
    void renderEllipse(const IRenderer::Color& color, const BoxF& box, float angle = 0) override;
    void renderEllipseLines(const IRenderer::Color& color, const BoxF& box, float angle = 0) override;
    void renderLine(const IRenderer::Color& color, const Vec2F& origin, const Vec2F& end) override;
    void renderVector(const IRenderer::Color& color, const Vec2F& origin, const Vec2F& vector) override;
    void renderPixel(const IRenderer::Color& color, const Vec2F& pos) override;
    void renderText(const IRenderer::Color& color, const BoxF& box, const std::string& text, const TextFormat& format,
                    float angle = 0) override;

    void update() override;
    void clear() override;

private:
    struct State;

    RaylibRender();
    ~RaylibRender() override;

    bool                   ready_ = false;
    std::unique_ptr<State> state_;
};
