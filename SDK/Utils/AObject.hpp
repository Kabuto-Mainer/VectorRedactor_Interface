#ifndef A_OBJECT_HPP
#define A_OBJECT_HPP

#include "Utils/BoxF.hpp"
#include "Utils/Event.hpp"

#include <iostream>
#include <list>
#include <memory>

class AObject
{
private:
    BoxF                                box_;
    std::list<std::unique_ptr<AObject>> children_;
    AObject*                            parent_ = nullptr;

public:
    AObject()          = default;
    virtual ~AObject() = default;

    AObject* parent() const;
    void     setParent(AObject* new_parent);

    const std::list<std::unique_ptr<AObject>>& children() const;
    std::list<std::unique_ptr<AObject>>&       children();
    void                                       addChild(std::unique_ptr<AObject> child);

    /* default impl = send to children */
    virtual bool processEvent(const Event& event);
    /* default impl = render children */
    virtual void render();
};

/* ====================== Implementation ====================== */

void AObject::setParent(AObject* new_parent)
{
    parent_ = new_parent;
}

AObject* AObject::parent() const
{
    return parent_;
}

const std::list<std::unique_ptr<AObject>>& AObject::children() const
{
    return children_;
}

std::list<std::unique_ptr<AObject>>& AObject::children()
{
    return children_;
}

void AObject::addChild(std::unique_ptr<AObject> child)
{
    child->setParent(this);
    children_.push_back(std::move(child));
}

bool AObject::processEvent(const Event& event)
{
    for (auto child = children_.rbegin(); child != children_.rend(); child++)
    {
        if ((*child)->processEvent(event))
        {
            return true;
        }
    }

    return false;
}

void AObject::render()
{
    for (auto child = children_.begin(); child != children_.end(); child++)
    {
        (*child)->render();
    }
}

#endif
