#include "MainPlugin.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDesktop.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"

#include <memory>
#include <utility>
#include <variant>

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    auto container = std::make_unique<DesktopNode>();

    static MainPlugin desktop(container->id());

    const ApplicationCore::Error error =
        ApplicationCore::instance().registerInterface(IID_DESKTOP,
                                                      static_cast<void*>(static_cast<IDesktop*>(&desktop)));
    if (error != ApplicationCore::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Failed to register IDesktop: %s",
                             ApplicationCore::strError(error));
        return false;
    }

    // Desktop is the bottom-most child of root: every other object is rendered above it
    TObject* root = ApplicationCore::instance().root();
    container->setParent(root);
    root->children().push_front(std::move(container));

    return true;
}

bool DesktopNode::processEvent(const IInputPump::Event& event)
{
    if (std::holds_alternative<IInputPump::EventQuit>(event))
    {
        ApplicationCore::instance().stop();
        return true;
    }

    return processEventChildren(event);
}

void DesktopNode::render()
{
    renderChildren();
}

Vec2F DesktopNode::mapLocalToGlobal(const Vec2F& pos) const
{
    return parent() ? parent()->mapLocalToGlobal(pos) : pos;
}

Vec2F DesktopNode::mapGlobalToLocal(const Vec2F& pos) const
{
    return parent() ? parent()->mapGlobalToLocal(pos) : pos;
}

MainPlugin::MainPlugin(TObject::ID container_id) : container_id_(container_id)
{
}

IDesktop::Error MainPlugin::add(std::unique_ptr<TObject> object)
{
    if (object == nullptr)
    {
        return Error::INVALID_ARGS;
    }

    TObject* desktop_node = container();
    if (desktop_node == nullptr || object.get() == desktop_node)
    {
        return Error::INVALID_ARGS;
    }

    desktop_node->addChild(std::move(object));
    return Error::SUCCESS;
}

IDesktop::Error MainPlugin::remove(TObject::ID object_id)
{
    TObject* desktop_node = container();
    if (desktop_node == nullptr)
    {
        return Error::OBJECT_NOT_FOUND;
    }

    TObject* object = desktop_node->find(object_id);
    if (object == nullptr || object == desktop_node || object->parent() == nullptr)
    {
        return Error::OBJECT_NOT_FOUND;
    }

    object->parent()->children().remove_if([object](const std::unique_ptr<TObject>& child)
                                           { return child.get() == object; });
    return Error::SUCCESS;
}

TObject* MainPlugin::container() const
{
    TObject* root = ApplicationCore::instance().root();
    if (root == nullptr)
    {
        return nullptr;
    }

    return root->find(container_id_);
}
