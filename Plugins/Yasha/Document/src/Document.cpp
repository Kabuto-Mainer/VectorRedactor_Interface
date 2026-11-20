#include "Document.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDesktop.hpp"

#include <memory>
#include <utility>

namespace Yasha {
void Document::addShape(std::unique_ptr<TShape> shape)
{
    if (shape == nullptr || shape->parent() != nullptr)
    {
        return;
    }
    addChild(std::move(shape));
}

void Document::removeShape(TObject::ID shape_id)
{
    for (auto it = children().begin(); it != children().end(); ++it)
    {
        if ((*it)->id() == shape_id)
        {
            children().erase(it);
            return;
        }
    }
}

std::optional<TObject::ID> Document::hitTest(const Vec2F& pos)
{
    for (auto it = children().rbegin(); it != children().rend(); ++it)
    {
        auto* shape = dynamic_cast<TShape*>(it->get());
        if (shape != nullptr && shape->hitTest(pos))
        {
            return shape->id();
        }
    }
    return std::nullopt;
}

void Document::accept(Visitor& visitor) const
{
    for (const auto& child : children())
    {
        if (const auto* shape = dynamic_cast<const TShape*>(child.get()); shape != nullptr && !visitor.visit(*shape))
        {
            return;
        }
    }
}

bool Document::processEvent(const IInputPump::Event& event)
{
    return processEventChildren(event);
}

void Document::render()
{
    renderChildren();
}

ApplicationCore::Error initializeDocument()
{
    auto&     core    = ApplicationCore::instance();
    IDesktop* desktop = nullptr;
    if (core.queryInterface(IID_DESKTOP, reinterpret_cast<void**>(&desktop)) != ApplicationCore::Error::SUCCESS ||
        desktop == nullptr)
    {
        return ApplicationCore::Error::INTERFACE_NOT_FOUND;
    }

    auto              document  = std::make_unique<Document>();
    const TObject::ID id        = document->id();
    auto*             interface = static_cast<IDocument*>(document.get());
    if (desktop->add(std::move(document)) != IDesktop::Error::SUCCESS)
    {
        return ApplicationCore::Error::INVALID_ARGS;
    }

    const auto error = core.registerInterface(IID_IDOCUMENT, interface);
    if (error != ApplicationCore::Error::SUCCESS)
    {
        desktop->remove(id);
    }
    return error;
}
} // namespace Yasha
