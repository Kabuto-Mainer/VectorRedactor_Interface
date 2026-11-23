#pragma once

#include "Interfaces/IDocument.hpp"

namespace Yasha::Tools
{

IDocument::TShape*       findShape(IDocument& document, TObject::ID id);
const IDocument::TShape* findShape(const IDocument& document, TObject::ID id);

} // namespace Yasha::Tools
