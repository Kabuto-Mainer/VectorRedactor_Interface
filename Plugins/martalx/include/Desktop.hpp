#pragma once

#include "Interfaces/IDesktop.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Utils/TObject.hpp"
#include "Utils/TWidget.hpp"

#include <memory>

class Desktop final : public IDesktop
{
	private:
		Desktop();

		class Node;
		// TODO: INVALID_ID
		TObject::ID node_id_ = 0;

	public:
		Desktop(const Desktop&)            = delete;
		Desktop(Desktop&&)                 = delete;
		Desktop& operator=(const Desktop&) = delete;
		Desktop& operator=(Desktop&&)      = delete;

		static Desktop& instance();

		// FIXME: Error -> void
		Error add(std::unique_ptr<TObject> object) override;
		[[deprecated]] Error remove(TObject::ID object_id) override;
};

class Desktop::Node final : public TWidget
{
	public:
		void render() override;
		bool processEvent(const IInputPump::Event& event) override;
};
