#pragma once

#include "Interfaces/IInputPump.hpp"
#include "Utils/Vec2F.hpp"

#include <cstdint>
#include <list>
#include <memory>
#include <random>
#include <utility>

class ApplicationCore;

class TObject
{
public:
    using ID = uint64_t;

    virtual ~TObject() = default;

    ID       id() const;
    TObject* parent() const;
    void     setParent(TObject* new_parent);

    void addChild(std::unique_ptr<TObject> child);

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

    const std::list<std::unique_ptr<TObject>>& children() const;
    std::list<std::unique_ptr<TObject>>&       children();
    void                                       renderChildren();
    bool                                       processEventChildren(const IInputPump::Event& event);

    TObject* findChild(ID child_id) const;
    void     removeChild(ID child_id);

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

TObject* TObject::findChild(ID child_id) const
{
    for (auto child = children_.begin(); child != children_.end(); child++)
    {
        if ((*child)->id_ == child_id)
        {
            return child->get();
        }

        auto* found = findChild(child_id);
        if (found != nullptr)
        {
            return found;
        }
    }

    return nullptr;
}

void TObject::removeChild(ID child_id)
{
    auto* child_to_remove = findChild(child_id);
    if (child_to_remove->parent_ == nullptr)
    {
        return;
    }

    child_to_remove->parent_->children_.remove_if([child_id](const std::unique_ptr<TObject>& object)
                                                  { return object->id_ == child_id; });
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
