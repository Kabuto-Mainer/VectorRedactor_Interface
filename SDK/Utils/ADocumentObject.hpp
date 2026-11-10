#ifndef A_DOCUMENT_OBJECT_HPP
#define A_DOCUMENT_OBJECT_HPP

#include "Core/ApplicationCore.hpp"
#include "Utils/ADocumentObjectComponent.hpp"
#include "Utils/AObject.hpp"
#include "Utils/Matrix3F.hpp"
#include "Utils/TransformComponent.hpp"
#include "Utils/Vec2F.hpp"

#include <cassert>
#include <memory>
#include <unordered_map>

// forward declaration
class BoxF;
struct Color;

class ADocumentObject : public AObject
{
public:
    ADocumentObject();
    ~ADocumentObject() override = 0;

    Vec2F mapLocalToGlobal(Vec2F const& pos) const override final;
    Vec2F mapGlobalToLocal(Vec2F const& pos) const override final;

    // COM-like component access. The caller receives a borrowed pointer to
    // the component registered under coid; ownership stays with the object.
    ApplicationCore::Error queryComponent(CoID coid, void** component);
    ApplicationCore::Error queryComponent(CoID coid, void const** component) const;

    template<typename T>
    T* queryComponent()
    {
        void* component = nullptr;
        if (queryComponent(T::COID, &component) != ApplicationCore::Error::SUCCESS)
        {
            return nullptr;
        }

        return static_cast<T*>(component);
    }

    template<typename T>
    T const* queryComponent() const
    {
        void const* component = nullptr;
        if (queryComponent(T::COID, &component) != ApplicationCore::Error::SUCCESS)
        {
            return nullptr;
        }

        return static_cast<T const*>(component);
    }

    virtual void setColor(Color color) = 0;

    virtual bool hitTest(Vec2F const& pos) const = 0;
    virtual BoxF axesAlignedBoundingBox() const  = 0;

    // Convenience forwarding to the built-in transform component.
    Vec2F const&    scaling() const;
    float           rotation() const;
    Vec2F const&    translation() const;
    Matrix3F const& transform() const;

    void setScaling(Vec2F const& new_scaling);
    void setRotation(float new_rotation);
    void setTranslation(Vec2F const& new_translation);
    void setTransform(Matrix3F const& new_transform);

    void scale(Vec2F const& factor);
    void rotate(float angle);
    void translate(Vec2F const& offset);
    void applyTransform(Matrix3F const& additional_transform);

protected:
    bool addComponent(std::unique_ptr<ADocumentObjectComponent> component);
    bool removeComponent(CoID coid);

private:
    std::unordered_map<CoID, std::unique_ptr<ADocumentObjectComponent>> components_;
}; // class ADocumentObject

/* ====================== Implementation ====================== */

inline ADocumentObject::ADocumentObject()
{
    addComponent(std::make_unique<TransformComponent>());
}

inline ADocumentObject::~ADocumentObject() = default;

inline Vec2F ADocumentObject::mapLocalToGlobal(Vec2F const& pos) const
{
    return parent()->mapLocalToGlobal(transform().transformPoint(pos));
}

inline Vec2F ADocumentObject::mapGlobalToLocal(Vec2F const& pos) const
{
    return transform().inverted().transformPoint(parent()->mapGlobalToLocal(pos));
}

inline ApplicationCore::Error ADocumentObject::queryComponent(CoID coid, void** component)
{
    if (component == nullptr)
    {
        return ApplicationCore::Error::INVALID_ARGS;
    }

    *component = nullptr;

    auto const found = components_.find(coid);
    if (found == components_.end() || !found->second)
    {
        return ApplicationCore::Error::INTERFACE_NOT_FOUND;
    }

    *component = found->second.get();
    return ApplicationCore::Error::SUCCESS;
}

inline ApplicationCore::Error ADocumentObject::queryComponent(CoID coid, void const** component) const
{
    if (component == nullptr)
    {
        return ApplicationCore::Error::INVALID_ARGS;
    }

    *component = nullptr;

    auto const found = components_.find(coid);
    if (found == components_.end() || !found->second)
    {
        return ApplicationCore::Error::INTERFACE_NOT_FOUND;
    }

    *component = found->second.get();
    return ApplicationCore::Error::SUCCESS;
}

inline bool ADocumentObject::addComponent(std::unique_ptr<ADocumentObjectComponent> component)
{
    if (!component)
    {
        return false;
    }

    CoID const coid = component->coid();
    return components_.emplace(coid, std::move(component)).second;
}

inline bool ADocumentObject::removeComponent(CoID coid)
{
    // The transform component is a built-in invariant of every document object.
    if (coid == TransformComponent::COID)
    {
        return false;
    }

    return components_.erase(coid) > 0;
}

inline Vec2F const& ADocumentObject::scaling() const
{
    TransformComponent const* component = queryComponent<TransformComponent>();
    assert(component != nullptr);

    return component->scaling();
}

inline float ADocumentObject::rotation() const
{
    TransformComponent const* component = queryComponent<TransformComponent>();
    assert(component != nullptr);

    return component->rotation();
}

inline Vec2F const& ADocumentObject::translation() const
{
    TransformComponent const* component = queryComponent<TransformComponent>();
    assert(component != nullptr);

    return component->translation();
}

inline Matrix3F const& ADocumentObject::transform() const
{
    TransformComponent const* component = queryComponent<TransformComponent>();
    assert(component != nullptr);

    return component->transform();
}

inline void ADocumentObject::setScaling(Vec2F const& new_scaling)
{
    TransformComponent* component = queryComponent<TransformComponent>();
    assert(component != nullptr);

    component->setScaling(new_scaling);
}

inline void ADocumentObject::setRotation(float const new_rotation)
{
    TransformComponent* component = queryComponent<TransformComponent>();
    assert(component != nullptr);

    component->setRotation(new_rotation);
}

inline void ADocumentObject::setTranslation(Vec2F const& new_translation)
{
    TransformComponent* component = queryComponent<TransformComponent>();
    assert(component != nullptr);

    component->setTranslation(new_translation);
}

inline void ADocumentObject::setTransform(Matrix3F const& new_transform)
{
    TransformComponent* component = queryComponent<TransformComponent>();
    assert(component != nullptr);

    component->setTransform(new_transform);
}

inline void ADocumentObject::scale(Vec2F const& factor)
{
    TransformComponent* component = queryComponent<TransformComponent>();
    assert(component != nullptr);

    component->scale(factor);
}

inline void ADocumentObject::rotate(float const angle)
{
    TransformComponent* component = queryComponent<TransformComponent>();
    assert(component != nullptr);

    component->rotate(angle);
}

inline void ADocumentObject::translate(Vec2F const& offset)
{
    TransformComponent* component = queryComponent<TransformComponent>();
    assert(component != nullptr);

    component->translate(offset);
}

inline void ADocumentObject::applyTransform(Matrix3F const& additional_transform)
{
    TransformComponent* component = queryComponent<TransformComponent>();
    assert(component != nullptr);

    component->applyTransform(additional_transform);
}

#endif // A_DOCUMENT_OBJECT_HPP
