#pragma once

#include "Interfaces/IID.hpp"
#include "Interfaces/IRenderer.hpp"


namespace Kabuto
{

constexpr static const IID IID_PALETTE = 0x00'07;

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

}; // namespace Kabuto
