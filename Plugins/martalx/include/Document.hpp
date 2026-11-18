#pragma once

#include "Interfaces/IDocument.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/Package.hpp"
#include "Utils/TWidget.hpp"

#include <memory>

extern "C" InitPackageFunc INIT_PACKAGE_FUNC_NAME;

class Document final : public IDocument
{
	private:
		Document();

		class Node;
		// TODO: INVALID_ID
		TObject::ID node_id_ = 0;

	public:
		Document(const Document&)            = delete;
		Document(Document&&)                 = delete;
		Document& operator=(const Document&) = delete;
		Document& operator=(Document&&)      = delete;

		static Document& instance();

		void addShape(std::unique_ptr<TShape> shape) override;
		void removeShape(TObject::ID shape_id) override;

		std::optional<TObject::ID> hitTest(const Vec2F& pos) const override;
		void accept(Visitor& visitor) const override;
};

class Document::Node final : public TWidget
{
	public:
		void render() override;
		bool processEvent(const IInputPump::Event& event) override;

		void addShape(std::unique_ptr<TShape> shape);
		void removeShape(TObject::ID shape_id);
		std::optional<TObject::ID> hitTest(const Vec2F& pos) const;
		void accept(Visitor& visitor) const;
};
