#pragma once

#include "Interfaces/IID.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Utils/BoxF.hpp"

constexpr static const IID IID_PALETTE = 0x00'07;

/**
 * Color palette service.
 *
 * The plugin creates a palette widget and adds it to the desktop. Other plugins
 * can query this interface by IID_PALETTE to read the selected color or set it
 * programmatically.
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

    virtual BoxF box() const                 = 0;
    virtual void setBox(const BoxF& new_box) = 0;
}; // class IPalette
