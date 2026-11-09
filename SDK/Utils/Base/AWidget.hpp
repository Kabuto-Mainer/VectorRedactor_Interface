#ifndef A_WIDGET_HPP
#define A_WIDGET_HPP

#include "Utils/Base/AObject.hpp"
#include "Utils/Math/BoxF.hpp"
#include "Utils/Math/Vec2F.hpp"

class AWidget : public AObject
{

public:
    Vec2F mapLocalToGlobal(const Vec2F& pos) const override final;
    Vec2F mapGlobalToLocal(const Vec2F& pos) const override final;

    BoxF box() const;
    void setBox(const BoxF& box);

private:
    BoxF box_;

}; // class AWidget

BoxF AWidget::box() const
{
    return box_;
}

void AWidget::setBox(const BoxF& box)
{
    box_ = box;
}

Vec2F AWidget::mapGlobalToLocal(const Vec2F& pos) const
{
    if (this->parent() == nullptr)
    {
        return pos;
    }
    return parent()->mapGlobalToLocal(pos) - box().topLeft();
}

Vec2F AWidget::mapLocalToGlobal(const Vec2F& pos) const
{
    if (this->parent() == nullptr)
    {
        return pos;
    }
    return parent()->mapLocalToGlobal(pos + box().topLeft());
}

#endif // A_WIDGET_HPP
