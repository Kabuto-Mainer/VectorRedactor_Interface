#include "Document.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDesktop.hpp"
#include "Interfaces/Package.hpp"

#include <algorithm>
#include <iterator>
#include <memory>
#include <optional>
#include <utility>

namespace Kabuto
{

namespace Document
{

void CDocument::addShape(std::unique_ptr<TShape> shape)
{
    if (!shape)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Document can't add null shape");
        return;
    }

    addChild(std::move(shape));
}

void CDocument::removeShape(TObject::ID shape_id)
{
    children().remove_if([shape_id](const std::unique_ptr<TObject>& child)
                         { return child != nullptr && child->id() == shape_id; });
}

std::optional<TObject::ID> CDocument::hitTest(const Vec2F& pos)
{
    for (auto child = children().rbegin(); child != children().rend(); ++child)
    {
        auto* shape = dynamic_cast<TShape*>(child->get());
        if (shape != nullptr && shape->hitTest(pos))
        {
            return shape->id();
        }
    }

    return std::nullopt;
}

void CDocument::accept(Visitor& visitor)
{
    for (const auto& child : children())
    {
        auto* shape = dynamic_cast<TShape*>(child.get());
        if (shape != nullptr && !visitor.visit(*shape))
        {
            return;
        }
    }
}

void CDocument::accept(Visitor& visitor) const
{
    for (const auto& child : children())
    {
        const auto* shape = dynamic_cast<const TShape*>(child.get());
        if (shape != nullptr && !visitor.visit(*shape))
        {
            return;
        }
    }
}

bool CDocument::processEvent(const IInputPump::Event& /*event*/)
{
    return false;
}

void CDocument::render()
{
    renderChildren();
}

Vec2F CDocument::mapLocalToGlobal(const Vec2F& pos) const
{
    if (parent() == nullptr)
    {
        return pos;
    }

    return parent()->mapLocalToGlobal(pos);
}

Vec2F CDocument::mapGlobalToLocal(const Vec2F& pos) const
{
    if (parent() == nullptr)
    {
        return pos;
    }

    return parent()->mapGlobalToLocal(pos);
}

}; // namespace Document

}; // namespace Kabuto

namespace
{

void moveToBackground(TObject& parent, TObject* child)
{
    auto& siblings = parent.children();
    if (siblings.size() < 2)
    {
        return;
    }

    const auto position =
        std::find_if(siblings.begin(), siblings.end(),
                     [child](const std::unique_ptr<TObject>& sibling) { return sibling.get() == child; });
    if (position != siblings.end() && position != siblings.begin())
    {
        siblings.splice(siblings.begin(), siblings, position);
    }
}

} // namespace

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    ApplicationCore& core = ApplicationCore::instance();

    auto                         document      = std::make_unique<Kabuto::Document::CDocument>();
    Kabuto::Document::CDocument* document_ptr  = document.get();
    auto*                        interface_ptr = static_cast<IDocument*>(document_ptr);

    IDesktop* desktop = nullptr;
    if (core.queryInterface(IID_DESKTOP, reinterpret_cast<void**>(&desktop)) == ApplicationCore::Error::SUCCESS &&
        desktop != nullptr)
    {
        if (desktop->add(std::move(document)) != IDesktop::Error::SUCCESS)
        {
            ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Document can't be added to desktop");
            return false;
        }

        // The document is a canvas: render it below the desktop windows.
        TObject* parent = document_ptr->parent();
        if (parent != nullptr)
        {
            moveToBackground(*parent, document_ptr);
        }
    }
    else
    {
        TObject* root = core.root();
        if (root == nullptr)
        {
            ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Document can't find the root object");
            return false;
        }

        root->children().push_front(std::move(document));
        document_ptr->setParent(root);
    }

    const ApplicationCore::Error error = core.registerInterface(IID_IDOCUMENT, static_cast<void*>(interface_ptr));
    if (error != ApplicationCore::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Failed to register IDocument: %s",
                             ApplicationCore::strError(error));

        TObject* parent = document_ptr->parent();
        if (parent != nullptr)
        {
            parent->children().remove_if([document_ptr](const std::unique_ptr<TObject>& child)
                                         { return child.get() == document_ptr; });
        }
        return false;
    }

    return true;
}
