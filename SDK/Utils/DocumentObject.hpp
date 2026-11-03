#ifndef DOCUMENT_OBJECT_HPP
#define DOCUMENT_OBJECT_HPP

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Utils/AObject.hpp"
#include "Utils/Vec2F.hpp"

class DocumentObject : public AObject
{
public:
    virtual ~DocumentObject() = default;

    virtual bool hitTest(Vec2F const& point) const = 0;

protected:
    DocumentObject() = default;

    static IRenderer* renderer()
    {
        IRenderer* renderer = nullptr;
        ApplicationCore::instance().queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer));
        return renderer;
    }
};

#endif
