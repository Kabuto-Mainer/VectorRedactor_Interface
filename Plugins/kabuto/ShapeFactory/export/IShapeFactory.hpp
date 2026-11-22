#pragma once

#include "Interfaces/IDocument.hpp"
#include "Interfaces/IID.hpp"
#include <functional>
#include <istream>
#include <memory>

namespace Kabuto
{

constexpr static const IID IID_ISHAPE_FACTORY = 1236172;

class IShapeFactory
{
public:
    using Loader = std::function<std::unique_ptr<IDocument::TShape>(std::istream&)>;
    using Saver = std::function<void(IDocument::TShape&, std::ostream&)>;

    virtual std::unique_ptr<std::vector<std::unique_ptr<IDocument::TShape>>> loadFrom(const std::string & path) const = 0;
    virtual void saveTo(const std::string & path) const = 0;

    virtual void registry(IDocument::TShape::TypeID type_id, Loader loader, Saver saver) = 0;
}; // class IShapeFactory

} // namespace Kabuto

/**
 * Формат файла подразумевается следующий:
 * IID + ... + IID + ...+ IID
 *
 * После вызова Loader будет происходить скип всех пробельных символов.
 * Затем будет попытка прочитать значение как IID. Если не получается - ошибка загрузка - неправильный формат.
 * Иначе читается IID и вызывается соответствующая функция.
 */
