#ifndef A_OBJECT_HPP
#define A_OBJECT_HPP

#include "Utils/Event.hpp"

#include <cstdint>
#include <list>
#include <memory>
#include <random>

// forward declaration
class Vec2F;

using ObjectID = uint64_t;

class AObject
{
public:
    virtual ~AObject() = 0;

    AObject& operator=(const AObject&) = delete;
    AObject& operator=(AObject&&)      = delete;

    ObjectID id() const;

    AObject*       parent();
    AObject const* parent() const;
    void           setParent(AObject* new_parent);

    std::list<std::unique_ptr<AObject>>&       children();
    const std::list<std::unique_ptr<AObject>>& children() const;
    void                                       addChild(std::unique_ptr<AObject> child);

    AObject*       find(ObjectID oid);
    const AObject* find(ObjectID oid) const;

    virtual Vec2F mapLocalToGlobal(const Vec2F& pos) const = 0;
    virtual Vec2F mapGlobalToLocal(const Vec2F& pos) const = 0;

    virtual bool processEvent(const Event& event) = 0;
    virtual void render() const                   = 0;

protected:
    AObject();

    AObject(const AObject&) = delete;
    AObject(AObject&&);

private:
    static ObjectID generateID();

private:
    ObjectID const                      id_{};
    AObject*                            parent_{nullptr};
    std::list<std::unique_ptr<AObject>> children_{};
};

/* ====================== Implementation ====================== */

inline ObjectID AObject::generateID()
{
    static std::mt19937_64                         generator{std::random_device{}()};
    static std::uniform_int_distribution<ObjectID> distribution{};

    return distribution(generator);
}

inline AObject::AObject() : id_{generateID()}
{
}

inline AObject::~AObject()         = default;
inline AObject::AObject(AObject&&) = default;

inline ObjectID AObject::id() const
{
    return id_;
}

inline AObject* AObject::parent()
{
    return parent_;
}

inline AObject const* AObject::parent() const
{
    return parent_;
}

inline void AObject::setParent(AObject* const new_parent)
{
    parent_ = new_parent;
}

inline std::list<std::unique_ptr<AObject>>& AObject::children()
{
    return children_;
}

inline const std::list<std::unique_ptr<AObject>>& AObject::children() const
{
    return children_;
}

inline void AObject::addChild(std::unique_ptr<AObject> child)
{
    child->setParent(this);
    children_.push_back(std::move(child));
}

inline AObject* AObject::find(ObjectID const oid)
{
    if (id() == oid)
    {
        return this;
    }

    for (auto& child : children_)
    {
        if (AObject* const result = child->find(oid))
        {
            return result;
        }
    }

    return nullptr;
}

inline const AObject* AObject::find(ObjectID const oid) const
{
    if (id() == oid)
    {
        return this;
    }

    for (const auto& child : children_)
    {
        if (const AObject* const result = child->find(oid))
        {
            return result;
        }
    }

    return nullptr;
}

#endif
