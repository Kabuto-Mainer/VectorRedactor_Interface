#include "include/Document.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDesktop.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Interfaces/Package.hpp"
#include "export/ICamera.hpp"
// #include "export/EllipseShape.hpp"
// #include "export/RectangleShape.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <memory>
#include <optional>
#include <utility>

namespace Kabuto
{

namespace Document
{

namespace
{

Vec2F clampScaling(const Vec2F& scaling)
{
    return Vec2F{std::clamp(scaling.x(), ICamera::MIN_SCALE, ICamera::MAX_SCALE),
                 std::clamp(scaling.y(), ICamera::MIN_SCALE, ICamera::MAX_SCALE)};
}

} // namespace

/* TDocument */

void TDocument::addShape(std::unique_ptr<IDocument::TShape> shape)
{
    if (!shape)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Document can't add null shape");
        return;
    }

    addChild(std::move(shape));
}

void TDocument::removeShape(TObject::ID shape_id)
{
    children().remove_if([shape_id](const std::unique_ptr<TObject>& child)
                         { return child != nullptr && child->id() == shape_id; });
}

std::optional<TObject::ID> TDocument::hitTest(const Vec2F& pos)
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

void TDocument::accept(IDocument::Visitor& visitor) const
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

bool TDocument::processEvent(const IInputPump::Event& event)
{
    IToolManager* tool_manager = nullptr;

    auto result = ApplicationCore::instance().queryInterface(IID_TOOL_MANAGER, reinterpret_cast<void**>(&tool_manager));
    if (result != ApplicationCore::Error::SUCCESS || tool_manager == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "IToolManager not founded");
        return false;
    }

    return tool_manager->processEvent(event);
}

void TDocument::render()
{
    renderChildren();
}

Vec2F TDocument::mapLocalToGlobal(const Vec2F& pos) const
{
    if (parent() == nullptr)
    {
        return pos;
    }

    return parent()->mapLocalToGlobal(transform().transformPoint(pos));
}

Vec2F TDocument::mapGlobalToLocal(const Vec2F& pos) const
{
    if (parent() == nullptr)
    {
        return pos;
    }
    return transform().inverted().transformPoint(parent()->mapGlobalToLocal(pos));
}

/* CDocument */

CDocument& CDocument::instance()
{
    static CDocument document;
    return document;
}

TDocument* CDocument::node() const
{
    auto node = ApplicationCore::instance().findObject(document_id_);

    if (node == nullptr || node != document_node_)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Disconnect between raw documentNode and documentID");
        return nullptr;
    }
    return static_cast<TDocument*>(node);
}

void CDocument::bindNode(TDocument& document)
{
    document_node_ = &document;
    document_id_   = document.id();
}

void CDocument::unbindNode()
{
    document_node_ = nullptr;
}

void CDocument::addShape(std::unique_ptr<TShape> shape)
{
    if (node() != nullptr)
    {
        document_node_->addShape(std::move(shape));
    }
}

void CDocument::removeShape(TObject::ID shape_id)
{
    if (node() != nullptr)
    {
        document_node_->removeShape(shape_id);
    }
}

std::optional<TObject::ID> CDocument::hitTest(const Vec2F& pos)
{
    if (node() != nullptr)
    {
        return document_node_->hitTest(pos);
    }

    return std::nullopt;
}

void CDocument::accept(Visitor& visitor) const
{
    if (node() != nullptr)
    {
        document_node_->accept(visitor);
    }
}

void CDocument::addScale(float scale)
{
    if (node() != nullptr)
    {
        auto past = node()->scaling();
        node()->setScaling(clampScaling(past + Vec2F(1, 1) * scale));
    }
}

void CDocument::addScaleAt(float scale, const Vec2F& anchor)
{
    if (node() == nullptr)
    {
        return;
    }

    const Vec2F past = node()->scaling();
    const Vec2F next = clampScaling(past + Vec2F(1, 1) * scale);

    const Vec2F translation = node()->translation();
    node()->setScaling(next);

    if (past.x() <= 0.0F)
    {
        return;
    }

    /* A content point l is rendered at t + s * l, so keeping the anchor p fixed
       requires t' = p - (p - t) * s' / s */
    node()->setTranslation(anchor - (anchor - translation) * (next.x() / past.x()));
}

void CDocument::addShift(const Vec2F& shift)
{
    if (node() != nullptr)
    {
        node()->translate(shift);
    }
}

void CDocument::reset()
{
    if (node() != nullptr)
    {
        node()->setScaling(Vec2F(1, 1));
        node()->setRotation(0.0F);
        node()->setTranslation(Vec2F(0, 0));
    }
}

Vec2F CDocument::mapGlobalToLocal(const Vec2F& pos) const
{
    if (node() == nullptr)
    {
        return pos;
    }

    return node()->mapGlobalToLocal(pos);
}

}; // namespace Document

}; // namespace Kabuto

namespace
{

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

    auto  document_node = std::make_unique<Kabuto::Document::TDocument>();
    auto* node_ptr      = document_node.get();

    Kabuto::Document::CDocument& document = Kabuto::Document::CDocument::instance();
    document.bindNode(*node_ptr);

    if (desktop->add(std::move(document_node)) != IDesktop::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Document can't be added to the desktop");
        document.unbindNode();
        return false;
    }

    const ApplicationCore::Error error_1 =
        core.registerInterface(IID_IDOCUMENT, static_cast<void*>(static_cast<IDocument*>(&document)));

    const ApplicationCore::Error error_2 =
        core.registerInterface(Kabuto::IID_ICAMERA, static_cast<void*>(static_cast<Kabuto::ICamera*>(&document)));

    if (error_1 != ApplicationCore::Error::SUCCESS || error_2 != ApplicationCore::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Failed to register IDocument: %s",
                             ApplicationCore::strError(error_1 != ApplicationCore::Error::SUCCESS ? error_1 : error_2));

        desktop->remove(document_node->id());
        document.unbindNode();
        return false;
    }
    return true;
}
