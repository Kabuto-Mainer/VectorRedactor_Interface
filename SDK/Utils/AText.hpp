#ifndef A_TEXT_HPP
#define A_TEXT_HPP

#include "Utils/DocumentObject.hpp"
#include "Utils/TextGraphic.hpp"
#include "Utils/TransformComponent.hpp"

class AText : public DocumentObject, public TransformComponent, public TextGraphic
{
public:
    virtual ~AText() = default;

protected:
    explicit AText(Vec2F const& size) : TransformComponent(size)
    {
    }
};

#endif
