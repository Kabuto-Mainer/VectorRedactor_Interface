#include "Palette.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDesktop.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Vec2F.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace Kabuto
{

namespace Palette
{

namespace
{

constexpr uint8_t OPAQUE_ALPHA = 255;

constexpr float PANEL_PADDING       = 8.0F;
constexpr float PANEL_PADDING_TOTAL = PANEL_PADDING + PANEL_PADDING;
constexpr float PREVIEW_HEIGHT      = 24.0F;
constexpr float CONTENT_GAP         = 8.0F;
constexpr float SWATCH_GAP          = 4.0F;
constexpr float BORDER_INSET        = 1.0F;
constexpr float BORDER_INSET_TOTAL  = BORDER_INSET + BORDER_INSET;

constexpr std::size_t SWATCH_COLUMNS = 8;
constexpr std::size_t SWATCH_ROWS    = 4;
constexpr std::size_t SWATCH_COUNT   = SWATCH_COLUMNS * SWATCH_ROWS;

constexpr float DEFAULT_LEFT   = 24.0F;
constexpr float DEFAULT_TOP    = 24.0F;
constexpr float DEFAULT_WIDTH  = 264.0F;
constexpr float DEFAULT_HEIGHT = 208.0F;

constexpr IRenderer::Color PANEL_COLOR{36, 36, 36, OPAQUE_ALPHA};
constexpr IRenderer::Color PANEL_BORDER{92, 92, 92, OPAQUE_ALPHA};
constexpr IRenderer::Color SWATCH_BORDER{64, 64, 64, OPAQUE_ALPHA};
constexpr IRenderer::Color HOVER_BORDER{255, 179, 71, OPAQUE_ALPHA};
constexpr IRenderer::Color SELECTION_LIGHT{255, 255, 255, OPAQUE_ALPHA};
constexpr IRenderer::Color SELECTION_DARK{0, 0, 0, OPAQUE_ALPHA};
constexpr IRenderer::Color DEFAULT_COLOR{0, 122, 255, OPAQUE_ALPHA};

constexpr IRenderer::Color opaque(uint8_t red, uint8_t green, uint8_t blue)
{
    return IRenderer::Color{red, green, blue, OPAQUE_ALPHA};
}

constexpr std::array<IRenderer::Color, SWATCH_COUNT> SWATCHES = {
    opaque(0, 0, 0),       opaque(32, 32, 32),    opaque(64, 64, 64),    opaque(96, 96, 96),
    opaque(128, 128, 128), opaque(160, 160, 160), opaque(208, 208, 208), opaque(255, 255, 255),

    opaque(235, 64, 52),   opaque(255, 145, 0),   opaque(255, 214, 0),   opaque(52, 199, 89),
    opaque(0, 199, 190),   opaque(0, 122, 255),   opaque(88, 86, 214),   opaque(175, 82, 222),

    opaque(139, 0, 0),     opaque(139, 69, 19),   opaque(128, 128, 0),   opaque(34, 139, 34),
    opaque(0, 128, 128),   opaque(0, 0, 128),     opaque(75, 0, 130),    opaque(128, 0, 128),

    opaque(255, 192, 203), opaque(255, 218, 185), opaque(255, 255, 153), opaque(144, 238, 144),
    opaque(127, 255, 212), opaque(135, 206, 250), opaque(216, 191, 216), opaque(211, 211, 211),
};

IRenderer* queryRenderer()
{
    IRenderer* renderer = nullptr;
    if (ApplicationCore::instance().queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer)) !=
        ApplicationCore::Error::SUCCESS)
    {
        return nullptr;
    }
    return renderer;
}

bool sameRgb(const IRenderer::Color& left, const IRenderer::Color& right)
{
    return left.r == right.r && left.g == right.g && left.b == right.b;
}

BoxF makeBox(const Vec2F& top_left, const Vec2F& size)
{
    return BoxF(top_left, top_left + size);
}

BoxF globalBoxOf(const TWidget& widget)
{
    const Vec2F origin = widget.mapLocalToGlobal(Vec2F(0.0F, 0.0F));
    return makeBox(origin, widget.box().size());
}

BoxF insetBox(const BoxF& box, float amount)
{
    return BoxF(box.topLeft() + Vec2F(amount), box.bottomRight() - Vec2F(amount));
}

BoxF previewBox(const BoxF& bounds)
{
    return makeBox(bounds.topLeft() + Vec2F(PANEL_PADDING),
                   Vec2F(bounds.size().x() - PANEL_PADDING_TOTAL, PREVIEW_HEIGHT));
}

void drawDoubleBorder(IRenderer& renderer, const BoxF& box)
{
    renderer.renderRectLines(SELECTION_LIGHT, box);
    if (box.size().x() > BORDER_INSET_TOTAL && box.size().y() > BORDER_INSET_TOTAL)
    {
        renderer.renderRectLines(SELECTION_DARK, insetBox(box, BORDER_INSET));
    }
}

} // namespace

CPalette::CPalette(IUIFramework& framework) : color_(DEFAULT_COLOR)
{
    setBox(BoxF(DEFAULT_LEFT, DEFAULT_TOP, DEFAULT_LEFT + DEFAULT_WIDTH, DEFAULT_TOP + DEFAULT_HEIGHT));
    buildSwitchers(framework);
    layoutWidgets();
}

IPalette::Color CPalette::color() const
{
    return color_;
}

void CPalette::setColor(const Color& new_color)
{
    color_ = new_color;
}

BoxF CPalette::box() const
{
    return TWidget::box();
}

void CPalette::setBox(const BoxF& new_box)
{
    TWidget::setBox(new_box);
    layoutWidgets();
}

bool CPalette::processEvent(const IInputPump::Event& event)
{
    const auto* mouse = std::get_if<IInputPump::EventMouse>(&event);
    if (mouse == nullptr)
    {
        return processEventChildren(event);
    }

    using enum IInputPump::EventMouse::Type;

    const Vec2F                 position = mouse->cursorPos();
    const bool                  inside   = globalBox().contains(position);
    const std::optional<size_t> hovered  = swatchAt(position);
    hovered_                             = hovered;

    bool consume = false;
    switch (mouse->type())
    {
        case MOTION:
            if (pressed_ && hovered)
            {
                selectSwatch(*hovered);
            }
            consume = pressed_;
            break;

        case LEFT_BUTTON_PRESSING:
            pressed_ = inside;
            consume  = inside;
            if (hovered)
            {
                selectSwatch(*hovered);
            }
            break;

        case LEFT_BUTTON_RELEASING:
            consume  = pressed_;
            pressed_ = false;
            break;

        default:
            break;
    }

    const bool handled = processEventChildren(event);
    return handled || consume;
}

void CPalette::render()
{
    IRenderer* renderer = queryRenderer();
    if (renderer == nullptr)
    {
        return;
    }

    const BoxF bounds = globalBox();
    if (bounds.size().x() <= PANEL_PADDING_TOTAL || bounds.size().y() <= PANEL_PADDING_TOTAL)
    {
        return;
    }

    renderer->renderRect(PANEL_COLOR, bounds);
    renderer->renderRectLines(PANEL_BORDER, bounds);

    const BoxF preview = previewBox(bounds);
    renderer->renderRect(color_, preview);
    renderer->renderRectLines(PANEL_BORDER, preview);

    renderChildren();

    const std::optional<std::size_t> selected = matchingSwatch();
    for (const Swatch& swatch : swatches_)
    {
        if (swatch.switcher == nullptr || swatch.color_index >= SWATCH_COUNT)
        {
            continue;
        }

        const BoxF swatch_box = globalBoxOf(*swatch.switcher);
        if (swatch_box.size().x() <= 0.0F || swatch_box.size().y() <= 0.0F)
        {
            continue;
        }

        renderer->renderRect(SWATCHES[swatch.color_index], swatch_box);
        renderer->renderRectLines(SWATCH_BORDER, swatch_box);

        if (selected && *selected == swatch.color_index)
        {
            drawDoubleBorder(*renderer, swatch_box);
        }
        else if (hovered_ && *hovered_ == swatch.color_index)
        {
            renderer->renderRectLines(HOVER_BORDER, swatch_box);
        }
    }
}

void CPalette::buildSwitchers(IUIFramework& framework)
{
    IUIFramework::SwitcherPoolDescription pool_description;
    pool_description.min_point = Vec2F(0.0F, 0.0F);
    pool_description.size      = Vec2F(1.0F, 1.0F);
    pool_description.direction = IUIFramework::Direction::HORIZONTAL;
    pool_description.role      = IUIFramework::Role::DEFAULT;

    std::unique_ptr<IUIFramework::SwitcherPool> pool = framework.create(pool_description);
    if (!pool)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Palette can't create a switcher pool");
        return;
    }

    for (std::size_t color_index = 0; color_index < SWATCH_COUNT; ++color_index)
    {
        IUIFramework::SwitcherDescription description;
        description.role = IUIFramework::Role::DEFAULT;
        description.on_active.callbacks.emplace_back([this, color_index] { selectSwatch(color_index); });

        std::unique_ptr<IUIFramework::Switcher> switcher = framework.create(description);
        if (!switcher)
        {
            ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Palette can't create a switcher");
            break;
        }

        swatches_.push_back(Swatch{switcher.get(), color_index});
        pool->add(std::move(switcher));
    }

    pool_ = pool.get();
    addChild(std::move(pool));
}

void CPalette::layoutWidgets()
{
    if (pool_ == nullptr)
    {
        return;
    }

    const BoxF  bounds       = box();
    const Vec2F content_size = bounds.size() - Vec2F(PANEL_PADDING_TOTAL);
    const float grid_height  = content_size.y() - PREVIEW_HEIGHT - CONTENT_GAP;

    const auto clear_layout = [this]
    {
        pool_->setBox(BoxF());
        for (auto& child : pool_->children())
        {
            static_cast<TWidget&>(*child).setBox(BoxF());
        }
    };

    if (content_size.x() <= 0.0F || grid_height <= 0.0F)
    {
        clear_layout();
        return;
    }

    // Child boxes are relative to the palette, not to its global position.
    const Vec2F grid_origin(PANEL_PADDING, PANEL_PADDING + PREVIEW_HEIGHT + CONTENT_GAP);
    const Vec2F grid_size(content_size.x(), grid_height);
    pool_->setBox(makeBox(grid_origin, grid_size));

    const float cell_width =
        (grid_size.x() - (SWATCH_GAP * static_cast<float>(SWATCH_COLUMNS - 1))) / static_cast<float>(SWATCH_COLUMNS);
    const float cell_height =
        (grid_size.y() - (SWATCH_GAP * static_cast<float>(SWATCH_ROWS - 1))) / static_cast<float>(SWATCH_ROWS);

    if (cell_width <= 0.0F || cell_height <= 0.0F)
    {
        clear_layout();
        return;
    }

    std::size_t index = 0;
    for (auto& child : pool_->children())
    {
        if (index >= SWATCH_COUNT)
        {
            break;
        }

        const std::size_t row    = index / SWATCH_COLUMNS;
        const std::size_t column = index % SWATCH_COLUMNS;

        const Vec2F cell_origin(static_cast<float>(column) * (cell_width + SWATCH_GAP),
                                static_cast<float>(row) * (cell_height + SWATCH_GAP));
        static_cast<TWidget&>(*child).setBox(makeBox(cell_origin, Vec2F(cell_width, cell_height)));
        ++index;
    }
}

BoxF CPalette::globalBox() const
{
    return globalBoxOf(*this);
}

std::optional<std::size_t> CPalette::swatchAt(const Vec2F& global_point) const
{
    for (const Swatch& swatch : swatches_)
    {
        if (swatch.switcher == nullptr)
        {
            continue;
        }

        const BoxF swatch_box = globalBoxOf(*swatch.switcher);
        if (swatch_box.size().x() > 0.0F && swatch_box.size().y() > 0.0F && swatch_box.contains(global_point))
        {
            return swatch.color_index;
        }
    }

    return std::nullopt;
}

std::optional<std::size_t> CPalette::matchingSwatch() const
{
    for (std::size_t index = 0; index < SWATCH_COUNT; ++index)
    {
        if (sameRgb(SWATCHES[index], color_))
        {
            return index;
        }
    }

    return std::nullopt;
}

void CPalette::selectSwatch(std::size_t index)
{
    if (index < SWATCH_COUNT)
    {
        color_ = SWATCHES[index];
    }
}

} // namespace Palette

} // namespace Kabuto

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    ApplicationCore& core = ApplicationCore::instance();

    IUIFramework* framework = nullptr;
    if (core.queryInterface(IID_IUIFramework, reinterpret_cast<void**>(&framework)) !=
            ApplicationCore::Error::SUCCESS ||
        framework == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Palette can't find the UI framework");
        return false;
    }

    IDesktop* desktop = nullptr;
    if (core.queryInterface(IID_DESKTOP, reinterpret_cast<void**>(&desktop)) != ApplicationCore::Error::SUCCESS ||
        desktop == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Palette can't find the desktop");
        return false;
    }

    auto                       widget     = std::make_unique<Kabuto::Palette::CPalette>(*framework);
    Kabuto::Palette::CPalette* widget_ptr = widget.get();
    const TObject::ID          widget_id  = widget_ptr->id();

    if (desktop->add(std::move(widget)) != IDesktop::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Palette can't be added to the desktop");
        return false;
    }

    const ApplicationCore::Error error =
        core.registerInterface(IID_PALETTE, static_cast<void*>(static_cast<IPalette*>(widget_ptr)));
    if (error != ApplicationCore::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Failed to register IPalette: %s",
                             ApplicationCore::strError(error));
        desktop->remove(widget_id);
        return false;
    }

    return true;
}
