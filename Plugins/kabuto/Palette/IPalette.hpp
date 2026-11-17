#pragma once

#include "Interfaces/IID.hpp"
#include "Interfaces/IRenderer.hpp"

constexpr static const IID IID_PALETTE = 0x00'07;

/**
 * Color palette service.
 *
 * The interface implementation is separated from the scene tree: the palette
 * view is built on top of the UI framework's widgets, while this service only
 * stores the selected color. Other plugins can query this interface by
 * IID_PALETTE to read the selected color or set it programmatically.
 */
class IPalette
{
public:
    using Color = IRenderer::Color;

    IPalette()          = default;
    virtual ~IPalette() = default;

    IPalette(const IPalette&)            = delete;
    IPalette(IPalette&&)                 = delete;
    IPalette& operator=(const IPalette&) = delete;
    IPalette& operator=(IPalette&&)      = delete;

    virtual Color color() const                    = 0;
    virtual void  setColor(const Color& new_color) = 0;
}; // class IPalette
