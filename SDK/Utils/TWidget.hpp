#ifndef T_WIDGET_HPP
#define T_WIDGET_HPP

#include "Utils/BoxF.hpp"
#include "Utils/TObject.hpp"

class TWidget : public TObject
{

public:
    Vec2F mapLocalToGlobal(const Vec2F& pos) const override final;
    Vec2F mapGlobalToLocal(const Vec2F& pos) const override final;

    BoxF box() const;
    void setBox(const BoxF& box);

private:
    BoxF box_;

}; // class TWidget


inline BoxF TWidget::box() const
{
    return box_;
}

inline void TWidget::setBox(const BoxF& box)
{
    box_ = box;
}

inline Vec2F TWidget::mapGlobalToLocal(const Vec2F& pos) const
{
    if (this->parent() == nullptr)
    {
        return pos;
    }
    return parent()->mapGlobalToLocal(pos) - box().topLeft();
}

inline Vec2F TWidget::mapLocalToGlobal(const Vec2F& pos) const
{
    if (this->parent() == nullptr)
    {
        return pos;
    }
    return parent()->mapLocalToGlobal(pos + box().topLeft());
}


#endif // T_WIDGET_HPP
