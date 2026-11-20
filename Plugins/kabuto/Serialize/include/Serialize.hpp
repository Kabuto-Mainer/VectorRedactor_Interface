#pragma once

#include "Interfaces/IDocument.hpp"
#include "export/ISerialize.hpp"

#include <cmath>
#include <fstream>

namespace Kabuto
{

namespace Serialize
{

class Saver final : public IDocument::Visitor
{
public:
    bool visit(IDocument::TShape& shape) override;
    bool visit(const IDocument::TShape& shape) override;

    Saver(const std::string& path);

    bool isValid() const;

private:
    const std::string& path_;
    std::ofstream      file_;
};

class CSerialize final : public ISerialize
{
public:
    void                                            saveToJSON(const std::string& path) override;
    std::vector<std::unique_ptr<IDocument::TShape>> loadFrom(const std::string& path) override;

    static CSerialize& instance();
}; // class CSerialize

} // namespace Serialize

} // namespace Kabuto
