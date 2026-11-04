#ifndef A_OBJECT_HPP
#define A_OBJECT_HPP

#include "Utils/BoxF.hpp"
#include "Utils/Event.hpp"
#include "Vec2F.hpp"

#include <cstdint>
#include <iostream>
#include <list>
#include <memory>
#include <random>

using ObjectID = uint64_t;

class AObject
{

public:
    AObject();
    virtual ~AObject() = default;

    ObjectID ID() const;
    AObject* parent() const;
    void     setParent(AObject* new_parent);

    const std::list<std::unique_ptr<AObject>>& children() const;
    std::list<std::unique_ptr<AObject>>&       children();
    void                                       addChild(std::unique_ptr<AObject> child);

    AObject*       find(ObjectID id);
    const AObject* find(ObjectID id) const;

    virtual bool processEvent(const Event& event);
    virtual void render();

    virtual Vec2F mapLocalToGlobal(const Vec2F& pos) const = 0;
    virtual Vec2F mapGlobalToLocal(const Vec2F& pos) const = 0;

    AObject(const AObject&)            = delete;
    AObject(AObject&&)                 = delete;
    AObject& operator=(const AObject&) = delete;
    AObject& operator=(AObject&&)      = delete;

private:
    static ObjectID generateID();

    ObjectID                            id_;
    std::list<std::unique_ptr<AObject>> children_;
    AObject*                            parent_ = nullptr;
};

/* ====================== Implementation ====================== */

ObjectID AObject::generateID()
{
    static std::mt19937_64                         generator(std::random_device{}());
    static std::uniform_int_distribution<ObjectID> distribution;

    return distribution(generator);
}

AObject::AObject()
{
    id_ = generateID();
}

ObjectID AObject::ID() const
{
    return id_;
}

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

AObject* AObject::find(ObjectID id)
{
    if (ID() == id)
    {
        return this;
    }

    for (auto& child : children_)
    {
        if (AObject* result = child->find(id))
        {
            return result;
        }
    }

    return nullptr;
}

const AObject* AObject::find(ObjectID id) const
{
    if (ID() == id)
    {
        return this;
    }

    for (const auto& child : children_)
    {
        if (const AObject* result = child->find(id))
        {
            return result;
        }
    }

    return nullptr;
}

#endif
