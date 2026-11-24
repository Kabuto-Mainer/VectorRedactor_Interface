#pragma once

#include "Interfaces/IID.hpp"
#include "Utils/Vec2F.hpp"

namespace Kabuto
{

constexpr static const IID IID_ICAMERA = 01232134132;

class ICamera
{
public:
    static constexpr float MIN_SCALE = 0.05F;
    static constexpr float MAX_SCALE = 20.0F;

    ICamera()          = default;
    virtual ~ICamera() = default;

    ICamera(const ICamera&)            = delete;
    ICamera(ICamera&&)                 = delete;
    ICamera& operator=(const ICamera&) = delete;
    ICamera& operator=(ICamera&&)      = delete;

    /**
     * Adds the same scale increment to both axes.
     * The resulting scale is kept within [MIN_SCALE, MAX_SCALE].
     */
    virtual void addScale(float scale) = 0;
    /**
     * Same as addScale(), but the global point anchor stays in place,
     * which allows zooming around the cursor.
     */
    virtual void addScaleAt(float scale, const Vec2F& anchor) = 0;
    /**
     * Shifts the view by the given offset in global coordinates.
     */
    virtual void addShift(const Vec2F& shift) = 0;
    /**
     * Restores the default view: scale 1, no shift and no rotation.
     */
    virtual void reset() = 0;
    /**
     * Converts a global (screen) position to document coordinates.
     */
    virtual Vec2F mapGlobalToLocal(const Vec2F& pos) const = 0;
}; // class ICamera

} // namespace Kabuto
