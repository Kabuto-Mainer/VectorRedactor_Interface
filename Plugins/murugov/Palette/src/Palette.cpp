#include "Palette.hpp"

#include "Interfaces/IDesktop.hpp"

#include <memory>
#include <variant>

namespace Murugov
{

TPaletteWidget::TPaletteWidget(const Vec2F& top_right_anchor, IRenderer& renderer) : renderer_(renderer)
{
    constexpr int rows = static_cast<int>((COLORS.size() + COLUMNS - 1) / COLUMNS);

    const float total_w = PADDING * 2.0F + COLUMNS * SWATCH_SIZE + (COLUMNS - 1) * GAP;
    const float total_h = PADDING * 2.0F + rows * SWATCH_SIZE + (rows - 1) * GAP;

    const Vec2F top_left{top_right_anchor.x() - total_w, top_right_anchor.y()};
    setBox(BoxF(top_left, top_left + Vec2F{total_w, total_h}));
}

BoxF TPaletteWidget::swatchBox(std::size_t index) const
{
    const int row = static_cast<int>(index) / COLUMNS;
    const int col = static_cast<int>(index) % COLUMNS;

    const float x = PADDING + static_cast<float>(col) * (SWATCH_SIZE + GAP);
    const float y = PADDING + static_cast<float>(row) * (SWATCH_SIZE + GAP);

    return BoxF(Vec2F{x, y}, Vec2F{x + SWATCH_SIZE, y + SWATCH_SIZE});
}

bool TPaletteWidget::processEvent(const IInputPump::Event& event)
{
    const auto* mouse = std::get_if<IInputPump::EventMouse>(&event);
    if (mouse == nullptr || mouse->type() != IInputPump::EventMouse::Type::LEFT_BUTTON_PRESSING)
    {
        return false;
    }

    const Vec2F global_pos = mouse->cursorPos();
    const Vec2F top_left   = mapLocalToGlobal(Vec2F{0.0F, 0.0F});
    const BoxF  panel(top_left, top_left + box().size());

    if (!panel.contains(global_pos))
    {
        return false;
    }

    const Vec2F local_pos = global_pos - top_left;
    for (std::size_t i = 0; i < COLORS.size(); ++i)
    {
        if (swatchBox(i).contains(local_pos))
        {
            selected_index_ = i;
            return true;
        }
    }

    return true;
}

void TPaletteWidget::render()
{
    const Vec2F top_left = mapLocalToGlobal(Vec2F{0.0F, 0.0F});
    const BoxF  panel(top_left, top_left + box().size());

    renderer_.renderRect(IRenderer::Color::DARK_GRAY, panel);
    renderer_.renderRectLines(IRenderer::Color::GRAY, panel);

    for (std::size_t i = 0; i < COLORS.size(); ++i)
    {
        const BoxF local_b = swatchBox(i);
        const BoxF global_b(top_left + local_b.topLeft(), top_left + local_b.bottomRight());

        const IRenderer::Color col = COLORS[i];
        if (col.a == 0)
        {
            renderer_.renderRect(IRenderer::Color::LIGHT_GRAY, global_b);
            renderer_.renderLine(IRenderer::Color::RED, global_b.topLeft(), global_b.bottomRight());
            renderer_.renderLine(IRenderer::Color::RED, global_b.topRight(), global_b.bottomLeft());
        }
        else
        {
            renderer_.renderRect(col, global_b);
        }

        if (i == selected_index_)
        {
            renderer_.renderRectLines(IRenderer::Color::BLUE, global_b);
            const BoxF inner(global_b.left() + 1.0F, global_b.top() + 1.0F,
                             global_b.right() - 1.0F, global_b.bottom() - 1.0F);
            renderer_.renderRectLines(IRenderer::Color::WHITE, inner);
        }
        else
        {
            renderer_.renderRectLines(IRenderer::Color::BLACK, global_b);
        }
    }
}

Palette::Palette()
{
    auto& core = ApplicationCore::instance();

    IRenderer* renderer = nullptr;
    if (core.queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer)) != ApplicationCore::Error::SUCCESS ||
        renderer == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Palette requires Renderer");
        return;
    }

    IDesktop* desktop = nullptr;
    if (core.queryInterface(IID_DESKTOP, reinterpret_cast<void**>(&desktop)) != ApplicationCore::Error::SUCCESS ||
        desktop == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Palette requires Desktop");
        return;
    }

    auto widget = std::make_unique<TPaletteWidget>(Vec2F{1600.0F - 16.0F, 16.0F}, *renderer);
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

Palette::~Palette()
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

Palette& Palette::instance()
{
    static Palette inst;
    return inst;
}

IRenderer::Color Palette::selectedColor() const
{
    if (!initialized_ || widget_ == nullptr)
    {
        return IRenderer::Color::WHITE;
    }
    return widget_->selectedColor();
}

} // namespace Murugov

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    auto& palette = Murugov::Palette::instance();
    if (!palette.isValid())
    {
        return false;
    }

    return ApplicationCore::instance().registerInterface(IID_PALETTE, static_cast<Murugov::IPalette*>(&palette)) ==
           ApplicationCore::Error::SUCCESS;
}