#ifndef A_OBJECT_HPP
#define A_OBJECT_HPP

#include "Utils/Event.hpp"
#include "Utils/Vec2F.hpp"

#include <cstdint>
#include <list>
#include <memory>
#include <random>
#include <utility>

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

    void renderChildren();
    bool processEventChildren(const Event& event);

private:
    static ObjectID generateID();

    ObjectID                            id_;
    std::list<std::unique_ptr<AObject>> children_;
    AObject*                            parent_ = nullptr;
};

/* ====================== Implementation ====================== */

inline void AObject::renderChildren()
{
    for (auto child = children_.begin(); child != children_.end(); child++)
    {
        (*child)->render();
    }
}

inline bool AObject::processEventChildren(const Event& event)
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

inline ObjectID AObject::generateID()
{
    static std::mt19937_64                         generator(std::random_device{}());
    static std::uniform_int_distribution<ObjectID> distribution;

    return distribution(generator);
}

inline AObject::AObject()
{
    id_ = generateID();
}

inline ObjectID AObject::id() const
{
    return id_;
}

inline void AObject::setParent(AObject* new_parent)
{
    parent_ = new_parent;
}

inline AObject* AObject::parent() const
{
    return parent_;
}

inline const std::list<std::unique_ptr<AObject>>& AObject::children() const
{
    return children_;
}

inline std::list<std::unique_ptr<AObject>>& AObject::children()
{
    return children_;
}

inline void AObject::addChild(std::unique_ptr<AObject> child)
{
    child->setParent(this);
    children_.push_back(std::move(child));
}

inline AObject* AObject::find(ObjectID object_id)
{
    return const_cast<AObject*>(std::as_const(*this).find(object_id));
}

inline const AObject* AObject::find(ObjectID object_id) const
{
    if (id() == object_id)
    {
        return this;
    }

    for (const auto& child : children_)
    {
        if (const AObject* result = child->find(object_id))
        {
            return result;
        }
    }

    return nullptr;
}

#endif
