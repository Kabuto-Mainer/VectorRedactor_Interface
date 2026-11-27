#pragma once

#include "Core/ApplicationCore.hpp"
#include "IMurugovStatusBar.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/Package.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/TWidget.hpp"
#include "Utils/Vec2F.hpp"

#include <string>

extern "C" InitPackageFunc INIT_PACKAGE_FUNC_NAME;

namespace Murugov
{

class TStatusBarWidget final : public TWidget
{
public:
    TStatusBarWidget(float window_width, float window_height, float bar_height,
                     IRenderer& renderer, IRenderer::FontID font_id);

    bool processEvent(const IInputPump::Event& event) override;
    void render() override;

    void setStatusText(const std::string& text) { status_text_ = text; }
    void setActiveToolName(const std::string& tool_name) { tool_name_ = tool_name; }

private:
    IRenderer&        renderer_;
    IRenderer::FontID font_id_     = 0;
    Vec2F             cursor_pos_  = {0.0F, 0.0F};
    std::string       status_text_ = "Ready";
    std::string       tool_name_   = "None";
};

class StatusBar final : public IStatusBar
{
private:
    struct Config
    {
        float window_width  = 1600.0F;
        float window_height = 900.0F;
        float bar_height    = 26.0F;
    };

    bool              initialized_ = false;
    TStatusBarWidget* widget_      = nullptr;
    TObject::ID       widget_id_   = 0;

    StatusBar();
    ~StatusBar() override;

    static Config loadConfig(const std::string& filepath);

public:
    StatusBar(const StatusBar&)            = delete;
    StatusBar(StatusBar&&)                 = delete;
    StatusBar& operator=(const StatusBar&) = delete;
    StatusBar& operator=(StatusBar&&)      = delete;

    static StatusBar& instance();

    bool isValid() const { return initialized_; }

    /* from IStatusBar */

    void setStatusText(const std::string& text) override;
    void setActiveToolName(const std::string& tool_name) override;
};

} // namespace Murugov
