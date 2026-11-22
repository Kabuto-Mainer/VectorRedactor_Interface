#pragma once

#include "Components/Color.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IRenderer.hpp"
#include <memory>

namespace MartAlx::Shapes
{

class Rectangle final : public IDocument::TShape
{
	private:
		static constexpr TypeID TYPE_ID = 0;

		class Color;
		std::unique_ptr<Color> color_;

	public:
		Rectangle();

		bool processEvent(const IInputPump::Event &event) override;
		void render() override;

		bool hitTest(Vec2F const& pos) const override;
		BoxF axesAlignedBoundingBox() const override;
		TypeID tid() const override;

		// ISSUE: only const without const** (in Core make the same signature)
		[[deprecated]] Error queryComponent(IDocument::AShapeComponent::ID coid, IDocument::AShapeComponent** component) override;
		Error queryComponent(IDocument::AShapeComponent::ID coid, IDocument::AShapeComponent const** component) const override;
};

class Rectangle::Color final : public Components::Color
{
	private:
		IRenderer::Color color_ = IRenderer::Color::TRANSPARENT;

	public:
		IRenderer::Color color() const override;
		void setColor(const IRenderer::Color &color) override;
};

}

