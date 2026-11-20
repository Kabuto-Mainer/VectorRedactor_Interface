#pragma once

#include "Interfaces/IDocument.hpp"
#include "Interfaces/IID.hpp"

#include <memory>
#include <string>

namespace Kabuto
{

constexpr static const IID IID_SERIALIZE = 1'130;

class ISerialize
{
public:
    virtual void                                            saveToJSON(const std::string& path) = 0;
    virtual std::vector<std::unique_ptr<IDocument::TShape>> loadFrom(const std::string& path)   = 0;
}; // class ISerialize

} // namespace Kabuto
