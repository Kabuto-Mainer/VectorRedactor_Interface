#ifndef A_WIDGET_HPP
#define A_WIDGET_HPP

#include "Utils/TObject.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Vec2F.hpp"

class AWidget : public TObject
{

public:
    Vec2F mapLocalToGlobal(const Vec2F& pos) const override final;
    Vec2F mapGlobalToLocal(const Vec2F& pos) const override final;

    BoxF box() const;
    void setBox(const BoxF& box);

private:
    BoxF box_;

}; // class AWidget

inline BoxF AWidget::box() const
{
    return box_;
}

inline void AWidget::setBox(const BoxF& box)
{
    box_ = box;
}

inline Vec2F AWidget::mapGlobalToLocal(const Vec2F& pos) const
{
    if (this->parent() == nullptr)
    {
        return pos;
    }
    return parent()->mapGlobalToLocal(pos) - box().topLeft();
}

inline Vec2F AWidget::mapLocalToGlobal(const Vec2F& pos) const
{
    if (this->parent() == nullptr)
    {
        return pos;
    }
    return parent()->mapLocalToGlobal(pos + box().topLeft());
}

#endif // A_WIDGET_HPP
