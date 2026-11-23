#include "Palette.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/Package.hpp"

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    const auto error = Yasha::initializePalette(BoxF(Vec2F{1'000.0F, 1'000.0F}));
    if (error != ApplicationCore::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Palette initialization failed: %s",
                             ApplicationCore::strError(error));
        return false;
    }
    return true;
}
