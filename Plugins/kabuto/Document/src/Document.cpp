#include "Document.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDesktop.hpp"
#include "Interfaces/IRenderer.hpp"
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

/* DocumentNode */

void DocumentNode::addShape(std::unique_ptr<IDocument::TShape> shape)
{
    if (!shape)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Document can't add null shape");
        return;
    }

    addChild(std::move(shape));
}

void DocumentNode::removeShape(TObject::ID shape_id)
{
    children().remove_if([shape_id](const std::unique_ptr<TObject>& child)
                         { return child != nullptr && child->id() == shape_id; });
}

std::optional<TObject::ID> DocumentNode::hitTest(const Vec2F& pos)
{
    for (auto child = children().rbegin(); child != children().rend(); ++child)
    {
        auto* shape = dynamic_cast<IDocument::TShape*>(child->get());
        if (shape != nullptr && shape->hitTest(pos))
        {
            return shape->id();
        }
    }

    return std::nullopt;
}

void DocumentNode::accept(IDocument::Visitor& visitor)
{
    for (const auto& child : children())
    {
        auto* shape = dynamic_cast<IDocument::TShape*>(child.get());
        if (shape != nullptr && !visitor.visit(*shape))
        {
            return;
        }
    }
}

void DocumentNode::accept(IDocument::Visitor& visitor) const
{
    for (const auto& child : children())
    {
        const auto* shape = dynamic_cast<const IDocument::TShape*>(child.get());
        if (shape != nullptr && !visitor.visit(*shape))
        {
            return;
        }
    }
}

bool DocumentNode::processEvent(const IInputPump::Event& /*event*/)
{
    return false;
}

void DocumentNode::render()
{
    renderChildren();
}

Vec2F DocumentNode::mapLocalToGlobal(const Vec2F& pos) const
{
    if (parent() == nullptr)
    {
        return pos;
    }

    return parent()->mapLocalToGlobal(pos);
}

Vec2F DocumentNode::mapGlobalToLocal(const Vec2F& pos) const
{
    if (parent() == nullptr)
    {
        return pos;
    }

    return parent()->mapGlobalToLocal(pos);
}

/* CDocument */

CDocument& CDocument::instance()
{
    static CDocument document;
    return document;
}

void CDocument::bindNode(DocumentNode& node)
{
    node_ = &node;
}

void CDocument::unbindNode()
{
    node_ = nullptr;
}

DocumentNode* CDocument::node() const
{
    if (node_ == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Document is used before its view node is bound");
    }

    return node_;
}

void CDocument::addShape(std::unique_ptr<TShape> shape)
{
    if (DocumentNode* document_node = node())
    {
        document_node->addShape(std::move(shape));
    }
}

void CDocument::removeShape(TObject::ID shape_id)
{
    if (DocumentNode* document_node = node())
    {
        document_node->removeShape(shape_id);
    }
}

std::optional<TObject::ID> CDocument::hitTest(const Vec2F& pos)
{
    if (DocumentNode* document_node = node())
    {
        return document_node->hitTest(pos);
    }

    return std::nullopt;
}

void CDocument::accept(Visitor& visitor)
{
    if (DocumentNode* document_node = node())
    {
        document_node->accept(visitor);
    }
}

void CDocument::accept(Visitor& visitor) const
{
    if (const DocumentNode* document_node = node())
    {
        document_node->accept(visitor);
    }
}

DocumentNode* CDocument::view() const
{
    return node_;
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

    IRenderer* renderer = nullptr;
    if (core.queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer)) != ApplicationCore::Error::SUCCESS ||
        renderer == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Document can't find the renderer");
        return false;
    }

    IDesktop* desktop = nullptr;
    if (core.queryInterface(IID_DESKTOP, reinterpret_cast<void**>(&desktop)) != ApplicationCore::Error::SUCCESS ||
        desktop == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Document can't find the desktop");
        return false;
    }

    auto  document_node = std::make_unique<Kabuto::Document::DocumentNode>();
    auto* node_ptr      = document_node.get();

    Kabuto::Document::CDocument& document = Kabuto::Document::CDocument::instance();
    document.bindNode(*node_ptr);

    if (desktop->add(std::move(document_node)) != IDesktop::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Document can't be added to the desktop");
        document.unbindNode();
        return false;
    }

    // The document is a canvas: render it below the desktop windows.
    TObject* parent = node_ptr->parent();
    if (parent != nullptr)
    {
        moveToBackground(*parent, node_ptr);
    }

    const ApplicationCore::Error error =
        core.registerInterface(IID_IDOCUMENT, static_cast<void*>(static_cast<IDocument*>(&document)));
    if (error != ApplicationCore::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Failed to register IDocument: %s",
                             ApplicationCore::strError(error));

        if (parent != nullptr)
        {
            parent->children().remove_if([node_ptr](const std::unique_ptr<TObject>& child)
                                         { return child.get() == node_ptr; });
        }
        document.unbindNode();
        return false;
    }

    const ApplicationCore::Error view_error =
        core.registerInterface(IID_DOCUMENT_VIEW, static_cast<void*>(static_cast<IDocumentView*>(&document)));
    if (view_error != ApplicationCore::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Failed to register IDocumentView: %s",
                             ApplicationCore::strError(view_error));
        return false;
    }

    return true;
}
