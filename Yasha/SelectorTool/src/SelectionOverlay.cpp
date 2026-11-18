#include "SelectionOverlay.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IRenderer.hpp"

#include <array>

namespace Yasha::Tools
{

SelectionOverlay::SelectionOverlay(Document& document, ToolManager& manager, IToolManager::ATool::ID tool_id) :
    document_(document), manager_(manager), tool_id_(tool_id)
{
}

bool SelectionOverlay::processEvent(const IInputPump::Event& /*event*/)
{
    return false;
}

std::optional<Vec2F> SelectionOverlay::rotationHandlePosition() const
{
    if (manager_.activeToolId() != tool_id_ || !document_.selectedShapeId())
    {
        return std::nullopt;
    }
    const auto* shape = document_.findShape(*document_.selectedShapeId());
    if (shape == nullptr)
    {
        return std::nullopt;
    }
    const Vec2F center = shape->mapLocalToGlobal(Vec2F{});
    const Vec2F top = shape->mapLocalToGlobal(Vec2F{0.0F, -1.0F});
    const Vec2F outward = top - center;
    if (outward.len2() < 1.0e-6F)
    {
        return std::nullopt;
    }
    return top + outward * (HANDLE_OFFSET / outward.len());
}

// std::optional<Vec2F> SelectionOverlay::rotationHandlePosition() const
bool SelectionOverlay::hitTestRotationHandle(const Vec2F& global_pos) const
{
    const auto handle = rotationHandlePosition();
    return handle && (global_pos - *handle).len2() <= HIT_RADIUS * HIT_RADIUS;
}

void SelectionOverlay::render()
{
    if (manager_.activeToolId() != tool_id_ || !document_.selectedShapeId())
    {
        return;
    }
    const auto* shape = document_.findShape(*document_.selectedShapeId());
    if (shape == nullptr)
    {
        return;
    }
    IRenderer* renderer = nullptr;
    if (ApplicationCore::instance().queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer)) !=
            ApplicationCore::Error::SUCCESS ||
        renderer == nullptr)
    {
        return;
    }

    constexpr IRenderer::Color outline{42, 202, 236, 255};
    const std::array<Vec2F, 4> corners{
        shape->mapLocalToGlobal(Vec2F{-1.0F, -1.0F}),
        shape->mapLocalToGlobal(Vec2F{1.0F, -1.0F}),
        shape->mapLocalToGlobal(Vec2F{1.0F, 1.0F}),
        shape->mapLocalToGlobal(Vec2F{-1.0F, 1.0F}),
    };

    const std::array<Vec2F, 4> centers{
        shape->mapLocalToGlobal(Vec2F{-1.0F,  0.0F}),
        shape->mapLocalToGlobal(Vec2F{ 1.0F,  0.0F}),
        shape->mapLocalToGlobal(Vec2F{ 0.0F,  1.0F}),
        shape->mapLocalToGlobal(Vec2F{ 0.0F, -1.0F}),
    };

    for (std::size_t index = 0; index < corners.size(); ++index)
    {
        renderer->renderLine(outline, corners[index], corners[(index + 1) % corners.size()]);
    }

    for (std::size_t index = 0; index < centers.size(); ++index)
    {
        renderer->renderPixel(IRenderer::Color::RED, corners[index]);
    }

    if (const auto handle = rotationHandlePosition())
    {
        const Vec2F top = shape->mapLocalToGlobal(Vec2F{0.0F, -1.0F});
        renderer->renderLine(outline, top, *handle);
        renderer->renderEllipse(IRenderer::Color::DARK_GRAY,
                                BoxF(*handle - Vec2F{HANDLE_RADIUS}, *handle + Vec2F{HANDLE_RADIUS}));
        renderer->renderEllipseLines(outline,
                                     BoxF(*handle - Vec2F{HANDLE_RADIUS}, *handle + Vec2F{HANDLE_RADIUS}));

        // renderer->renderLine(IRenderer::Color::RED, {0, 0}, *handle);
    }

    // Рисование точек для resize
    // if (const auto handle = rotationHandlePosition())
    // {
    //       
    // }
}

} // namespace Yasha::Tools
