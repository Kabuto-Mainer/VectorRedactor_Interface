#ifndef A_OBJECT_HPP
#define A_OBJECT_HPP

#include "Interfaces/IInputPump.hpp"
#include "Utils/Vec2F.hpp"

#include <cstdint>
#include <list>
#include <memory>
#include <random>
#include <utility>


class TObject
{
public:
    using ID = uint64_t;

    virtual ~TObject() = default;

    ID       id() const;
    TObject* parent() const;
    void     setParent(TObject* new_parent);

    const std::list<std::unique_ptr<TObject>>& children() const;
    std::list<std::unique_ptr<TObject>>&       children();
    void                                       addChild(std::unique_ptr<TObject> child);

    TObject*       find(ID object_id);
    const TObject* find(ID object_id) const;

    virtual bool processEvent(const IInputPump::Event& event) = 0;
    virtual void render()                                     = 0;

    virtual Vec2F mapLocalToGlobal(const Vec2F& pos) const = 0;
    virtual Vec2F mapGlobalToLocal(const Vec2F& pos) const = 0;

    TObject(const TObject&)            = delete;
    TObject(TObject&&)                 = delete;
    TObject& operator=(const TObject&) = delete;
    TObject& operator=(TObject&&)      = delete;

protected:
    TObject();

    void renderChildren();
    bool processEventChildren(const IInputPump::Event& event);

private:
    static ID generateID();

    ID                                  id_;
    std::list<std::unique_ptr<TObject>> children_;
    TObject*                            parent_ = nullptr;
};

/* ====================== Implementation ====================== */

inline void TObject::renderChildren()
{
    for (auto child = children_.begin(); child != children_.end(); child++)
    {
        (*child)->render();
    }
}

inline bool TObject::processEventChildren(const IInputPump::Event& event)
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

inline TObject::ID TObject::generateID()
{
    static std::mt19937_64                   generator(std::random_device{}());
    static std::uniform_int_distribution<ID> distribution;

    return distribution(generator);
}

inline TObject::TObject()
{
    id_ = generateID();
}

inline TObject::ID TObject::id() const
{
    return id_;
}

inline void TObject::setParent(TObject* new_parent)
{
    parent_ = new_parent;
}

inline TObject* TObject::parent() const
{
    return parent_;
}

inline const std::list<std::unique_ptr<TObject>>& TObject::children() const
{
    return children_;
}

inline std::list<std::unique_ptr<TObject>>& TObject::children()
{
    return children_;
}

inline void TObject::addChild(std::unique_ptr<TObject> child)
{
    child->setParent(this);
    children_.push_back(std::move(child));
}

inline TObject* TObject::find(ID object_id)
{
    return const_cast<TObject*>(std::as_const(*this).find(object_id));
}

inline const TObject* TObject::find(ID object_id) const
{
    if (id() == object_id)
    {
        return this;
    }

    for (const auto& child : children_)
    {
        if (const TObject* result = child->find(object_id))
        {
            return result;
        }
    }

    return nullptr;
}

#endif
