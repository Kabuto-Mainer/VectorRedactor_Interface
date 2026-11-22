#pragma once

#include "Interfaces/IDocument.hpp"
#include "Interfaces/IRenderer.hpp"

namespace MartAlx::Components
{

static constexpr IDocument::AShapeComponent::ID ID_COLOR = 1;

class Color : public IDocument::AShapeComponent
{
	public:
		virtual IRenderer::Color color() const = 0;
		virtual void setColor(const IRenderer::Color &color) = 0;
};

}
