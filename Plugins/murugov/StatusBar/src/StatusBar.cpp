#include "StatusBar.hpp"

#include "../Palette/IMurugovPalette.hpp"
#include "Interfaces/IDesktop.hpp"

#include <dlfcn.h>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <regex>
#include <sstream>
#include <utility>
#include <variant>

namespace Murugov
{

/* ====================== TStatusBarWidget ====================== */

TStatusBarWidget::TStatusBarWidget(float window_width, float window_height, float bar_height,
                                   IRenderer& renderer, IRenderer::FontID font_id)
    : renderer_(renderer), font_id_(font_id)
{
    const Vec2F top_left{0.0F, window_height - bar_height};
    const Vec2F bottom_right{window_width, window_height};
    setBox(BoxF(top_left, bottom_right));
}

bool TStatusBarWidget::processEvent(const IInputPump::Event& event)
{
    const auto* mouse = std::get_if<IInputPump::EventMouse>(&event);
    if (mouse == nullptr)
    {
        return false;
    }

    if (mouse->type() == IInputPump::EventMouse::Type::MOTION)
    {
        cursor_pos_ = mouse->cursorPos();
        return false;
    }

    if (box().contains(mouse->cursorPos()))
    {
        return true;
    }

    return false;
}

void TStatusBarWidget::render()
{
    const BoxF bar = box();

    renderer_.renderRect(IRenderer::Color::DARK_GRAY, bar);
    renderer_.renderLine(IRenderer::Color::GRAY, bar.topLeft(), bar.topRight());

    IRenderer::TextFormat fmt{
        .font            = font_id_,
        .font_size       = 14,
        .overflow_action = IRenderer::TextOverflowAction::CUTBYBOX,
        .h_align         = IRenderer::TextHAlign::LEFT,
        .w_align         = IRenderer::TextWAlign::CENTER,
        .padding         = 8.0F,
        .line_spacing    = 1.0F,
        .wrap            = false,
    };

    const float total_w = bar.size().x();

    // 1. Left section (40%): current status/hint
    const float status_w = total_w * 0.40F;
    const BoxF  status_box(bar.topLeft(), Vec2F{bar.left() + status_w, bar.bottom()});
    renderer_.renderText(IRenderer::Color::WHITE, status_box, status_text_, fmt);

    // 2. Color section (20%): label and a colored square from IPalette
    const float color_w = total_w * 0.20F;
    const BoxF  color_box(Vec2F{status_box.right(), bar.top()}, Vec2F{status_box.right() + color_w, bar.bottom()});
    renderer_.renderLine(IRenderer::Color::GRAY, color_box.topLeft(), color_box.bottomLeft());
    renderer_.renderText(IRenderer::Color::LIGHT_GRAY, color_box, "Color:", fmt);

    IPalette* palette = nullptr;
    if (ApplicationCore::instance().queryInterface(IID_PALETTE, reinterpret_cast<void**>(&palette)) ==
            ApplicationCore::Error::SUCCESS &&
        palette != nullptr)
    {
        const float swatch_size = bar.size().y() - 8.0F;
        const float swatch_x    = color_box.left() + 58.0F;
        const float swatch_y    = bar.top() + 4.0F;
        const BoxF  swatch_box(Vec2F{swatch_x, swatch_y}, Vec2F{swatch_x + swatch_size, swatch_y + swatch_size});

        const IRenderer::Color col = palette->selectedColor();
        if (col.a == 0)
        {
            renderer_.renderRect(IRenderer::Color::LIGHT_GRAY, swatch_box);
            renderer_.renderLine(IRenderer::Color::RED, swatch_box.topLeft(), swatch_box.bottomRight());
            renderer_.renderLine(IRenderer::Color::RED, swatch_box.topRight(), swatch_box.bottomLeft());
        }
        else
        {
            renderer_.renderRect(col, swatch_box);
        }
        renderer_.renderRectLines(IRenderer::Color::BLACK, swatch_box);
    }

    // 3. Instrument section (20%): active instrument
    const float tool_w = total_w * 0.20F;
    const BoxF  tool_box(Vec2F{color_box.right(), bar.top()}, Vec2F{color_box.right() + tool_w, bar.bottom()});
    renderer_.renderLine(IRenderer::Color::GRAY, tool_box.topLeft(), tool_box.bottomLeft());

    const std::string tool_label = "Tool: " + tool_name_;
    renderer_.renderText(IRenderer::Color::LIGHT_GRAY, tool_box, tool_label, fmt);

    // 4. Right section (20%): mouse cursor coordinates
    const BoxF coords_box(Vec2F{tool_box.right(), bar.top()}, bar.bottomRight());
    renderer_.renderLine(IRenderer::Color::GRAY, coords_box.topLeft(), coords_box.bottomLeft());

    const std::string coords_text = std::format("X: {:.0f}   Y: {:.0f}", cursor_pos_.x(), cursor_pos_.y());

    fmt.h_align = IRenderer::TextHAlign::RIGHT;
    renderer_.renderText(IRenderer::Color::WHITE, coords_box, coords_text, fmt);
}

/* ====================== StatusBar Singleton ====================== */

StatusBar::StatusBar()
{
    auto& core = ApplicationCore::instance();

    IRenderer* renderer = nullptr;
    if (core.queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer)) != ApplicationCore::Error::SUCCESS ||
        renderer == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "StatusBar requires Renderer");
        return;
    }

    IDesktop* desktop = nullptr;
    if (core.queryInterface(IID_DESKTOP, reinterpret_cast<void**>(&desktop)) != ApplicationCore::Error::SUCCESS ||
        desktop == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "StatusBar requires Desktop");
        return;
    }

    IRenderer::FontID font_id = 0;
    if (auto font = renderer->loadFont("/System/Library/Fonts/Helvetica.ttc"); font.has_value())
    {
        font_id = *font;
    }

    std::filesystem::path config_path = "status_bar.json";

    Dl_info info;
    if (dladdr(reinterpret_cast<void*>(&StatusBar::instance), &info) != 0 && info.dli_fname != nullptr)
    {
        config_path = std::filesystem::path(info.dli_fname).parent_path() / "status_bar.json";
    }

    const Config cfg = loadConfig(config_path.string());

    auto widget = std::make_unique<TStatusBarWidget>(cfg.window_width, cfg.window_height, cfg.bar_height,
                                                     *renderer, font_id);
    widget_     = widget.get();
    widget_id_  = widget->id();

    if (desktop->add(std::move(widget)) != IDesktop::Error::SUCCESS)
    {
        widget_    = nullptr;
        widget_id_ = 0;
        return;
    }

    initialized_ = true;
}

StatusBar::~StatusBar()
{
    if (!initialized_ || widget_id_ == 0)
    {
        return;
    }

    IDesktop* desktop = nullptr;
    if (ApplicationCore::instance().queryInterface(IID_DESKTOP, reinterpret_cast<void**>(&desktop)) ==
            ApplicationCore::Error::SUCCESS &&
        desktop != nullptr)
    {
        desktop->remove(widget_id_);
    }
}

StatusBar::Config StatusBar::loadConfig(const std::string& filepath)
{
    Config cfg;
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "status_bar.json not found, using default layout");
        return cfg;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string content = buffer.str();

    auto extractFloat = [&content](const std::string& key, float default_val) -> float
    {
        const std::regex pattern("\"" + key + "\"\\s*:\\s*([-+]?[0-9]*\\.?[0-9]+)");
        std::smatch match;
        if (std::regex_search(content, match, pattern) && match.size() > 1)
        {
            try
            {
                return std::stof(match[1].str());
            }
            catch (...)
            {
                ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "Invalid number format in status_bar.json");
            }
        }
        return default_val;
    };

    cfg.window_width  = extractFloat("window_width", cfg.window_width);
    cfg.window_height = extractFloat("window_height", cfg.window_height);
    cfg.bar_height    = extractFloat("bar_height", cfg.bar_height);

    return cfg;
}

StatusBar& StatusBar::instance()
{
    static StatusBar inst;
    return inst;
}

void StatusBar::setStatusText(const std::string& text)
{
    if (widget_ != nullptr)
    {
        widget_->setStatusText(text);
    }
}

void StatusBar::setActiveToolName(const std::string& tool_name)
{
    if (widget_ != nullptr)
    {
        widget_->setActiveToolName(tool_name);
    }
}

} // namespace Murugov

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    auto& bar = Murugov::StatusBar::instance();
    if (!bar.isValid())
    {
        return false;
    }

    return ApplicationCore::instance().registerInterface(Murugov::IID_STATUS_BAR,
                                                         static_cast<Murugov::IStatusBar*>(&bar)) ==
           ApplicationCore::Error::SUCCESS;
}