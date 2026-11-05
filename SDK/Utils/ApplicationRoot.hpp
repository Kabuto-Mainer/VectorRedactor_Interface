#ifndef APPLICATION_ROOT_HPP
#define APPLICATION_ROOT_HPP

#include "Utils/AObject.hpp"
#include "Utils/Vec2F.hpp"

class ApplicationRoot : public AObject
{
public:
    ApplicationRoot();
    ~ApplicationRoot() override final;

    bool processEvent(Event const& event) override final;
    void render() const override final;

    Vec2F mapLocalToGlobal(const Vec2F& pos) const override final;
    Vec2F mapGlobalToLocal(const Vec2F& pos) const override final;
};

/* ======================== Implementation ======================== */

inline ApplicationRoot::ApplicationRoot()  = default;
inline ApplicationRoot::~ApplicationRoot() = default;

inline bool ApplicationRoot::processEvent(Event const& event)
{
    auto const end_iterator{children().rend()};
    for (auto child_iterator{children().rbegin()}; child_iterator != end_iterator; ++child_iterator)
    {
        if ((*child_iterator)->processEvent(event))
        {
            return true;
        }
    }
    return false;
}

inline void ApplicationRoot::render() const
{
    auto const end_iterator{children().rend()};
    for (auto child_iterator{children().rbegin()}; child_iterator != end_iterator; ++child_iterator)
    {
        (*child_iterator)->render();
    }
}

inline Vec2F ApplicationRoot::mapLocalToGlobal(const Vec2F& pos) const
{
    return pos;
}

inline Vec2F ApplicationRoot::mapGlobalToLocal(const Vec2F& pos) const
{
    return pos;
}

#endif
