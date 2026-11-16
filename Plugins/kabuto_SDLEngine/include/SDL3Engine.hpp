#pragma once

#include "Interfaces/IInputPump.hpp"
#include "Interfaces/Package.hpp"
#include "SDL3Renderer.hpp"

#include <optional>

extern "C" InitPackageFunc INIT_PACKAGE_FUNC_NAME;

/* NOTE:
 * This singleton provides access to both IRenderer and IInputPump.
 * Rendering lives in the SDL3Renderer base class.
 */
class SDL3Engine final : public SDL3Renderer, public IInputPump
{
private:
    SDL3Engine();
    ~SDL3Engine() override;

public:
    SDL3Engine(const SDL3Engine&)            = delete;
    SDL3Engine(SDL3Engine&&)                 = delete;
    SDL3Engine& operator=(const SDL3Engine&) = delete;
    SDL3Engine& operator=(SDL3Engine&&)      = delete;

    static SDL3Engine& instance();

    /* from IInputPump */

    std::optional<Event> pollEvent() override;
};
