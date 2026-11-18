#include "Document.hpp"

#include "Interfaces/IDesktop.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/Package.hpp"
#include "Utils/TObject.hpp"

#include <memory>
#include <optional>

/* Document */

Document::Document()
{
	IDesktop *desktop = nullptr;
	ApplicationCore::Error error = ApplicationCore::instance().queryInterface(IID_DESKTOP, reinterpret_cast<void **>(&desktop));
	assert(error == ApplicationCore::Error::SUCCESS);

	auto node = std::make_unique<Node>();
	node_id_ = node->id();
	desktop->add(std::move(node));
}

Document& Document::instance()
{
	static Document instance;
	return instance;
}

void Document::addShape(std::unique_ptr<TShape> shape)
{
	auto *node = ApplicationCore::instance().findObject(node_id_);
	static_cast<Node *>(node)->addShape(std::move(shape));
}

void Document::removeShape(TObject::ID shape_id)
{
	auto *node = ApplicationCore::instance().findObject(node_id_);
	static_cast<Node *>(node)->removeShape(shape_id);
}

std::optional<TObject::ID> Document::hitTest(const Vec2F& pos) const
{
	auto *node = ApplicationCore::instance().findObject(node_id_);
	return static_cast<Node *>(node)->hitTest(pos);
}

void Document::accept(Visitor& visitor) const
{

	auto *node = ApplicationCore::instance().findObject(node_id_);
	static_cast<Node *>(node)->accept(visitor);
}

/* Document::Node */

void Document::Node::render()
{
	renderChildren();
}

bool Document::Node::processEvent(const IInputPump::Event& event)
{
	return processEventChildren(event);
}

void Document::Node::addShape(std::unique_ptr<TShape> shape)
{
	children().push_back(std::move(shape));
}

void Document::Node::removeShape(TObject::ID shape_id)
{
	ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "FIXME: %s is stub calls ApplicationCore::deleteObject for now", __func__);
	ApplicationCore::instance().deleteObject(shape_id);
}

std::optional<TObject::ID> Document::Node::hitTest(const Vec2F &pos) const
{
	for (auto child = children().rbegin(); child != children().rend(); child++)
	{
		if (static_cast<TShape *>(child->get())->hitTest(pos))
		{
			return (*child)->id();
		}
	}

	return std::nullopt;
}

void Document::Node::accept(Visitor &visitor) const
{
	for (auto child = children().begin(); child != children().end(); child++)
	{
		visitor.visit(static_cast<TShape&>(**child));
	}
}

/* InitPackage */

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
	ApplicationCore::Error error = ApplicationCore::instance().registerInterface(IID_DESKTOP, reinterpret_cast<void *>(&Document::instance()));
	if(error != ApplicationCore::Error::SUCCESS)
	{
		ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Failed to register IDocument: %s", ApplicationCore::strError(error));
		return false;
	}

	return true;
}

