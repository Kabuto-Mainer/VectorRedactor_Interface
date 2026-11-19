#include "Desktop.hpp"
#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDesktop.hpp"
#include "Interfaces/Package.hpp"

#include <memory>
#include <utility>
#include <variant>

/* Desktop::Node */

void Desktop::Node::render()
{
	renderChildren();
}

bool Desktop::Node::processEvent(const IInputPump::Event& event)
{
	if(std::holds_alternative<IInputPump::EventQuit>(event))
	{
		ApplicationCore::instance().stop();
		return true;
	}

	return processEventChildren(event);
}

/* Desktop */

Desktop::Desktop()
{
	auto node = std::make_unique<Node>();
	node_id_ = node->id();
	ApplicationCore::instance().pushModal(std::move(node));
}

Desktop& Desktop::instance()
{
	static Desktop instance;
	return instance;
}

Desktop::Error Desktop::add(std::unique_ptr<TObject> object)
{
	auto *node = ApplicationCore::instance().findObject(node_id_);
	node->addChild(std::move(object));
	return Error::SUCCESS;
}

Desktop::Error Desktop::remove(TObject::ID object_id)
{
	ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "Use `ApplicationCore::deleteObject` instead of `%s`", __func__);
	ApplicationCore::instance().removeObject(object_id);
	return Error::SUCCESS;
}

bool INIT_PACKAGE_FUNC_NAME()
{
	ApplicationCore::Error error = ApplicationCore::instance().registerInterface(IID_DESKTOP, reinterpret_cast<void *>(&Desktop::instance()));
	if(error != ApplicationCore::Error::SUCCESS)
	{
		ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Failed to register IDesktop: %s", ApplicationCore::strError(error));
		return false;
	}

	return true;
}

