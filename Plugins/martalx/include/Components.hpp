#pragma once

#include "Interfaces/IDocument.hpp"
#include "Interfaces/IRenderer.hpp"

static constexpr IDocument::AShapeComponent::ID ID_COLOR_COMPONENT = 1;

class ColorComponent : public IDocument::AShapeComponent
{
	public:
		virtual IRenderer::Color color() const = 0;
		virtual void setColor(const IRenderer::Color &color) = 0;
};

