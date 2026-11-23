#include "Desktop.hpp"
#include "Document.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IToolManager.hpp"

#include <memory>
#include <utility>
#include <variant>

IDesktop::Error Desktop::add(std::unique_ptr<TObject> object)
{
    if (object == nullptr || object->parent() != nullptr || object.get() == this)
    {
        return Error::INVALID_ARGS;
    }

    addChild(std::move(object));
    return Error::SUCCESS;
}

IDesktop::Error Desktop::remove(TObject::ID object_id)
{
    IDocument* registered_document = nullptr;
    if (ApplicationCore::instance().queryInterface(IID_IDOCUMENT, reinterpret_cast<void**>(&registered_document)) ==
        ApplicationCore::Error::SUCCESS)
    {
        const auto* document_object = dynamic_cast<const TObject*>(registered_document);
        if (document_object != nullptr && document_object->id() == object_id)
        {
            return Error::INVALID_ARGS;
        }
    }

    for (auto it = children().begin(); it != children().end(); ++it)
    {
        if ((*it)->id() == object_id)
        {
            children().erase(it);
            return Error::SUCCESS;
        }
    }
    return Error::OBJECT_NOT_FOUND;
}

bool Desktop::processEvent(const IInputPump::Event& event)
{
    if (std::holds_alternative<IInputPump::EventQuit>(event))
    {
        ApplicationCore::instance().stop();
        return true;
    }
    if (const auto* key = std::get_if<IInputPump::EventKeyboard>(&event);
        key != nullptr && key->type() == IInputPump::EventKeyboard::Type::KEY_DOWN &&
        key->key() == IInputPump::EventKeyboard::Key::S)
    {
        auto keyboard = *key;
        if (keyboard.hasModifiers(IInputPump::EventKeyboard::Modifier::LEFT_CTRL) ||
            keyboard.hasModifiers(IInputPump::EventKeyboard::Modifier::RIGHT_CTRL))
        {
            IDocument* interface = nullptr;
            if (ApplicationCore::instance().queryInterface(IID_IDOCUMENT, reinterpret_cast<void**>(&interface)) ==
                ApplicationCore::Error::SUCCESS)
            {
                if (auto* document = dynamic_cast<Yasha::Document*>(interface))
                {
                    document->save();
                }
            }
            return true;
        }
    }
    if (processEventChildren(event))
    {
        return true;
    }

    IToolManager* manager = nullptr;
    if (ApplicationCore::instance().queryInterface(IID_TOOL_MANAGER, reinterpret_cast<void**>(&manager)) ==
            ApplicationCore::Error::SUCCESS &&
        manager != nullptr)
    {
        return manager->processEvent(event);
    }
    return false;
}

void Desktop::render()
{
    renderChildren();
}

ApplicationCore::Error initializeDesktop()
{
    auto& core     = ApplicationCore::instance();
    void* existing = nullptr;

    if (core.queryInterface(IID_DESKTOP, &existing) == ApplicationCore::Error::SUCCESS)
    {
        return ApplicationCore::Error::INTERFACE_ALREADY_EXISTS;
    }

    auto       desktop   = std::make_unique<Desktop>();
    auto*      interface = static_cast<IDesktop*>(desktop.get());
    const auto error     = core.registerInterface(IID_DESKTOP, interface);

    if (error != ApplicationCore::Error::SUCCESS)
    {
        return error;
    }

    core.pushModal(std::move(desktop));
    return ApplicationCore::Error::SUCCESS;
}
