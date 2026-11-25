#pragma once

#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IID.hpp"

constexpr static const IID IID_PALETTE = 6;

namespace Murugov
{

class IPalette
{
public:
    IPalette()          = default;
    virtual ~IPalette() = default;

    IPalette(const IPalette&)            = delete;
    IPalette(IPalette&&)                 = delete;
    IPalette& operator=(const IPalette&) = delete;
    IPalette& operator=(IPalette&&)      = delete;

    virtual IRenderer::Color selectedColor() const = 0;
}; // IPalette

} // namespace Murugov