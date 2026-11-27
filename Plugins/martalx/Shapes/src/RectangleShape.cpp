#include "RectangleShape.hpp"
#include "Components/Color.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Core/ApplicationCore.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Matrix3F.hpp"
#include "Utils/Vec2F.hpp"
#include <algorithm>
#include <memory>
#include <utility>

using namespace MartAlx::Shapes;

// ISSUE: constexpr BoxF, Vec2F
static BoxF globalBox(const IDocument::TShape &shape)
{
	const auto origin = shape.mapLocalToGlobal(Vec2F(0.0F, 0.0F));
	const auto top_left = -0.5F*shape.scaling() + origin;
	const auto bottom_right = +0.5F*shape.scaling() + origin;

	return BoxF(top_left, bottom_right);
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
	return BoxF(-0.5F, -0.5F, 0.5F, 0.5F).contains(mapGlobalToLocal(pos));
}

BoxF Rectangle::axesAlignedBoundingBox() const
{
	const auto box_corners =
	{
		transform().transformPoint(Vec2F(-0.5F, -0.5F)),
		transform().transformPoint(Vec2F( 0.5F, -0.5F)),
		transform().transformPoint(Vec2F(-0.5F,  0.5F)),
		transform().transformPoint(Vec2F( 0.5F,  0.5F)),
	};

	const auto compare_x = [](const Vec2F &v1, const Vec2F &v2){ return v1.x() < v2.x(); };
	const auto compare_y = [](const Vec2F &v1, const Vec2F &v2){ return v1.y() < v2.y(); };

	return BoxF(std::min(box_corners, compare_x).x(), std::min(box_corners, compare_y).y(),
			std::max(box_corners, compare_x).x(), std::max(box_corners, compare_y).y());
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
	if(coid != Components::ID_COLOR)
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

