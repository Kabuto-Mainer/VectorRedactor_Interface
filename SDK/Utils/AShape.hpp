#ifndef A_SHAPE_HPP
#define A_SHAPE_HPP

#include "Interfaces/IRenderer.hpp"
#include "Utils/ADocumentObject.hpp"

class AShape : public ADocumentObject
{
public:
    AShape();
    ~AShape() override = 0;

    Color color() const;
    void  setColor(Color color) override final;

private:
    Color color_{};
};

/* ====================== Implementation ====================== */

inline AShape::AShape()  = default;
inline AShape::~AShape() = default;

inline Color AShape::color() const
{
    return color_;
}

inline void AShape::setColor(Color const color)
{
    color_ = color;
}

#endif
