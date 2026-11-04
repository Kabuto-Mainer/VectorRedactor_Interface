#ifndef A_OBJECT_HPP
#define A_OBJECT_HPP

#include "Utils/BoxF.hpp"
#include "Utils/Event.hpp"
#include "Utils/Vec2F.hpp"

#include <cstdint>
#include <iostream>
#include <list>
#include <memory>
#include <random>

using ObjectID = uint64_t;

class AObject
{

public:
    virtual ~AObject() = default;

    ObjectID id() const;
    AObject* parent() const;
    void     setParent(AObject* new_parent);

    const std::list<std::unique_ptr<AObject>>& children() const;
    std::list<std::unique_ptr<AObject>>&       children();
    void                                       addChild(std::unique_ptr<AObject> child);

    AObject*       find(ObjectID id);
    const AObject* find(ObjectID id) const;

    virtual bool processEvent(const Event& event) = 0;
    virtual void render()                         = 0;

    virtual Vec2F mapLocalToGlobal(const Vec2F& pos) const = 0;
    virtual Vec2F mapGlobalToLocal(const Vec2F& pos) const = 0;

    AObject(const AObject&)            = delete;
    AObject(AObject&&)                 = delete;
    AObject& operator=(const AObject&) = delete;
    AObject& operator=(AObject&&)      = delete;

protected:
    AObject();

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

ObjectID AObject::id() const
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

AObject* AObject::find(ObjectID id)
{
    if (id() == id)
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

const AObject* AObject::find(ObjectID id_) const
{
    if (id() == id_)
    {
        return this;
    }

    for (const auto& child : children_)
    {
        if (const AObject* result = child->find(id_))
        {
            return result;
        }
    }

    return nullptr;
}

#endif
