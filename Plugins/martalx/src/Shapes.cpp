#include "Shapes.hpp"
#include "Components.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Vec2F.hpp"
#include <memory>
#include <utility>

static BoxF globalBox(const IDocument::TShape &shape)
{
	return BoxF { shape.mapLocalToGlobal(Vec2F { 0.0F, 0.0F }), shape.mapLocalToGlobal(Vec2F { 1.0F, 1.0F }) };
}

/* Rectangle */

Rectangle::Rectangle()
	: color_(std::make_unique<Color>())
{ }

bool Rectangle::processEvent(const IInputPump::Event &event)
{
	// FIXME: maybe process something...
	return false;
}

void Rectangle::render()
{
	IRenderer *renderer = nullptr;
	ApplicationCore::Error error = ApplicationCore::instance().queryInterface(IID_RENDERER, reinterpret_cast<void **>(&renderer));
	assert(error == ApplicationCore::Error::SUCCESS);

	renderer->renderRect(color_->color(), ::globalBox(*this), rotation());
}

bool Rectangle::hitTest(const Vec2F &pos) const
{
	// FIXME: implement
	return false;
}

BoxF Rectangle::axesAlignedBoundingBox() const
{
	// FIXME: implement
	ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "FIXME: %s not implemented", __func__);
	return BoxF {};
}

Rectangle::TypeID Rectangle::tid() const
{
	return Rectangle::TYPE_ID;
}

Rectangle::Error Rectangle::queryComponent(IDocument::AShapeComponent::ID coid, IDocument::AShapeComponent **component)
{
	return std::as_const(*this).queryComponent(coid, const_cast<const IDocument::AShapeComponent **>(component));
}

Rectangle::Error Rectangle::queryComponent(IDocument::AShapeComponent::ID coid, const IDocument::AShapeComponent **component) const
{
	if(coid != ID_COLOR_COMPONENT)
	{
		return Error::COMPONENT_NOT_FOUND;
		*component = nullptr;
	}

	*component = color_.get();
	return Error::SUCCESS;
}

/* Rectangle::Color */

IRenderer::Color Rectangle::Color::color() const
{
	return color_;
}

void Rectangle::Color::setColor(const IRenderer::Color &color)
{
	color_ = color;
}

