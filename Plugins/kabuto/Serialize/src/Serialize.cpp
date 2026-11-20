#include "include/Serialize.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/Package.hpp"
#include "dependens.hpp"
#include "export/ISerialize.hpp"
#include "include/ID.hpp"

#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Kabuto
{

namespace Serialize
{

namespace
{

/* ====================== JSON value ====================== */

struct JsonValue
{
    enum class Type
    {
        NULL_VALUE,
        BOOL,
        NUMBER,
        STRING,
        ARRAY,
        OBJECT,
    }; // enum class Type

    Type                             type = Type::NULL_VALUE;
    bool                             boolean{};
    double                           number{};
    std::string                      string;
    std::vector<JsonValue>           array;
    std::map<std::string, JsonValue> object;
}; // struct JsonValue

const JsonValue* findMember(const JsonValue& value, std::string_view key)
{
    if (value.type != JsonValue::Type::OBJECT)
    {
        return nullptr;
    }

    const auto member = value.object.find(std::string{key});
    if (member == value.object.end())
    {
        return nullptr;
    }

    return &member->second;
}

bool asFloat(const JsonValue* value, float& result)
{
    if (value == nullptr || value->type != JsonValue::Type::NUMBER)
    {
        return false;
    }

    result = static_cast<float>(value->number);
    return true;
}

bool asInt(const JsonValue* value, int& result)
{
    if (value == nullptr || value->type != JsonValue::Type::NUMBER)
    {
        return false;
    }

    result = static_cast<int>(value->number);
    return true;
}

bool asUInt64(const JsonValue* value, uint64_t& result)
{
    if (value == nullptr || value->type != JsonValue::Type::NUMBER || value->number < 0.0)
    {
        return false;
    }

    result = static_cast<uint64_t>(value->number);
    return true;
}

bool asBool(const JsonValue* value, bool& result)
{
    if (value == nullptr || value->type != JsonValue::Type::BOOL)
    {
        return false;
    }

    result = value->boolean;
    return true;
}

bool asVec2F(const JsonValue* value, Vec2F& result)
{
    if (value == nullptr || value->type != JsonValue::Type::ARRAY || value->array.size() < 2)
    {
        return false;
    }

    float x{};
    float y{};
    if (!asFloat(&value->array[0], x) || !asFloat(&value->array[1], y))
    {
        return false;
    }

    result = Vec2F{x, y};
    return true;
}

/* ====================== JSON parser ====================== */

class JsonParser final
{
public:
    explicit JsonParser(std::string_view text);

    bool parse(JsonValue& result);

private:
    bool parseValue(JsonValue& result);
    bool parseObject(JsonValue& result);
    bool parseArray(JsonValue& result);
    bool parseString(std::string& result);
    bool parseNumber(JsonValue& result);
    bool parseLiteral(std::string_view literal);

    void skipWhitespace();
    bool consume(char expected);
    bool fail();

private:
    std::string_view text_;
    std::size_t      position_ = 0;
    bool             failed_   = false;
}; // class JsonParser

inline JsonParser::JsonParser(std::string_view text) : text_(text)
{
}

inline bool JsonParser::parse(JsonValue& result)
{
    skipWhitespace();
    if (!parseValue(result))
    {
        return false;
    }

    skipWhitespace();
    if (position_ != text_.size())
    {
        return fail();
    }

    return !failed_;
}

inline bool JsonParser::parseValue(JsonValue& result)
{
    skipWhitespace();
    if (failed_ || position_ >= text_.size())
    {
        return fail();
    }

    switch (text_[position_])
    {
        case '{':
            return parseObject(result);

        case '[':
            return parseArray(result);

        case '"':
            result      = JsonValue{};
            result.type = JsonValue::Type::STRING;
            return parseString(result.string);

        case 't':
            result         = JsonValue{};
            result.type    = JsonValue::Type::BOOL;
            result.boolean = true;
            return parseLiteral("true");

        case 'f':
            result         = JsonValue{};
            result.type    = JsonValue::Type::BOOL;
            result.boolean = false;
            return parseLiteral("false");

        case 'n':
            result      = JsonValue{};
            result.type = JsonValue::Type::NULL_VALUE;
            return parseLiteral("null");

        default:
            return parseNumber(result);
    }
}

inline bool JsonParser::parseObject(JsonValue& result)
{
    if (!consume('{'))
    {
        return fail();
    }

    result      = JsonValue{};
    result.type = JsonValue::Type::OBJECT;

    skipWhitespace();
    if (consume('}'))
    {
        return true;
    }

    while (!failed_)
    {
        skipWhitespace();

        std::string key;
        if (!parseString(key))
        {
            return fail();
        }

        skipWhitespace();
        if (!consume(':'))
        {
            return fail();
        }

        JsonValue member;
        if (!parseValue(member))
        {
            return fail();
        }

        result.object.emplace(std::move(key), std::move(member));

        skipWhitespace();
        if (consume('}'))
        {
            return true;
        }

        if (!consume(','))
        {
            return fail();
        }
    }

    return fail();
}

inline bool JsonParser::parseArray(JsonValue& result)
{
    if (!consume('['))
    {
        return fail();
    }

    result      = JsonValue{};
    result.type = JsonValue::Type::ARRAY;

    skipWhitespace();
    if (consume(']'))
    {
        return true;
    }

    while (!failed_)
    {
        JsonValue element;
        if (!parseValue(element))
        {
            return fail();
        }

        result.array.push_back(std::move(element));

        skipWhitespace();
        if (consume(']'))
        {
            return true;
        }

        if (!consume(','))
        {
            return fail();
        }
    }

    return fail();
}

inline bool JsonParser::parseString(std::string& result)
{
    if (!consume('"'))
    {
        return fail();
    }

    result.clear();

    while (!failed_ && position_ < text_.size())
    {
        const char character = text_[position_++];

        if (character == '"')
        {
            return true;
        }

        if (character != '\\')
        {
            result.push_back(character);
            continue;
        }

        if (position_ >= text_.size())
        {
            return fail();
        }

        const char escaped = text_[position_++];
        switch (escaped)
        {
            case '"':
            case '\\':
            case '/':
                result.push_back(escaped);
                break;

            case 'b':
                result.push_back('\b');
                break;

            case 'f':
                result.push_back('\f');
                break;

            case 'n':
                result.push_back('\n');
                break;

            case 'r':
                result.push_back('\r');
                break;

            case 't':
                result.push_back('\t');
                break;

            case 'u':
            {
                if (text_.size() - position_ < 4)
                {
                    return fail();
                }

                uint32_t code_point = 0;
                for (int index = 0; index < 4; ++index)
                {
                    const char hex_digit = text_[position_++];
                    code_point <<= 4;

                    if (hex_digit >= '0' && hex_digit <= '9')
                    {
                        code_point |= static_cast<uint32_t>(hex_digit - '0');
                    }
                    else if (hex_digit >= 'a' && hex_digit <= 'f')
                    {
                        code_point |= static_cast<uint32_t>(hex_digit - 'a' + 10);
                    }
                    else if (hex_digit >= 'A' && hex_digit <= 'F')
                    {
                        code_point |= static_cast<uint32_t>(hex_digit - 'A' + 10);
                    }
                    else
                    {
                        return fail();
                    }
                }

                if (code_point <= 0x7F)
                {
                    result.push_back(static_cast<char>(code_point));
                }
                else if (code_point <= 0x7'FF)
                {
                    result.push_back(static_cast<char>(0xC0 | (code_point >> 6)));
                    result.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
                }
                else
                {
                    result.push_back(static_cast<char>(0xE0 | (code_point >> 12)));
                    result.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
                    result.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
                }
                break;
            }

            default:
                return fail();
        }
    }

    return fail();
}

inline bool JsonParser::parseNumber(JsonValue& result)
{
    const std::size_t start = position_;

    if (position_ < text_.size() && text_[position_] == '-')
    {
        ++position_;
    }

    if (position_ >= text_.size() || !std::isdigit(static_cast<unsigned char>(text_[position_])))
    {
        return fail();
    }

    while (position_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[position_])))
    {
        ++position_;
    }

    if (position_ < text_.size() && text_[position_] == '.')
    {
        ++position_;
        while (position_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[position_])))
        {
            ++position_;
        }
    }

    if (position_ < text_.size() && (text_[position_] == 'e' || text_[position_] == 'E'))
    {
        ++position_;
        if (position_ < text_.size() && (text_[position_] == '+' || text_[position_] == '-'))
        {
            ++position_;
        }

        while (position_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[position_])))
        {
            ++position_;
        }
    }

    const std::string token{text_.substr(start, position_ - start)};
    char*             end    = nullptr;
    const double      number = std::strtod(token.c_str(), &end);
    if (end == token.c_str() || *end != '\0')
    {
        return fail();
    }

    result        = JsonValue{};
    result.type   = JsonValue::Type::NUMBER;
    result.number = number;
    return true;
}

inline bool JsonParser::parseLiteral(std::string_view literal)
{
    if (text_.substr(position_, literal.size()) != literal)
    {
        return fail();
    }

    position_ += literal.size();
    return true;
}

inline void JsonParser::skipWhitespace()
{
    while (position_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[position_])) != 0)
    {
        ++position_;
    }
}

inline bool JsonParser::consume(char expected)
{
    if (position_ >= text_.size() || text_[position_] != expected)
    {
        return false;
    }

    ++position_;
    return true;
}

inline bool JsonParser::fail()
{
    failed_ = true;
    return false;
}

/* ====================== Serialization helpers ====================== */

constexpr std::streamoff FIRST_ELEMENT_POSITION = 2; // "[\n"

void writeFloat(std::ostream& stream, float value)
{
    stream << std::setprecision(std::numeric_limits<float>::max_digits10) << value;
}

void writeColor(std::ostream& stream, const IRenderer::Color& color)
{
    stream << "{\"r\":" << static_cast<unsigned int>(color.r) << ",\"g\":" << static_cast<unsigned int>(color.g)
           << ",\"b\":" << static_cast<unsigned int>(color.b) << ",\"a\":" << static_cast<unsigned int>(color.a) << '}';
}

void writeTextFormat(std::ostream& stream, const IRenderer::TextFormat& format)
{
    stream << "{\"font\":" << format.font << ",\"font_size\":" << format.font_size
           << ",\"overflow_action\":" << static_cast<int>(format.overflow_action)
           << ",\"h_align\":" << static_cast<int>(format.h_align) << ",\"w_align\":" << static_cast<int>(format.w_align)
           << ",\"padding\":";
    writeFloat(stream, format.padding);
    stream << ",\"line_spacing\":";
    writeFloat(stream, format.line_spacing);
    stream << ",\"wrap\":" << (format.wrap ? "true" : "false") << '}';
}

void writeTransform(std::ostream& stream, const IDocument::TShape& shape)
{
    const Vec2F& scaling     = shape.scaling();
    const Vec2F& translation = shape.translation();

    stream << "\"transform\":{\"scaling\":[";
    writeFloat(stream, scaling.x());
    stream << ',';
    writeFloat(stream, scaling.y());
    stream << "],\"rotation\":";
    writeFloat(stream, shape.rotation());
    stream << ",\"translation\":[";
    writeFloat(stream, translation.x());
    stream << ',';
    writeFloat(stream, translation.y());
    stream << "]}";
}

void writeComponents(std::ostream& stream, const IDocument::TShape& shape)
{
    stream << "\"components\":{";

    bool                              first_component = true;
    const IDocument::AShapeComponent* component       = nullptr;
    IDocument::TShape::Error          error{};

    error = shape.queryComponent(COLOR_COID, &component);
    if (error == IDocument::TShape::Error::SUCCESS && component != nullptr)
    {
        const auto* color_component = static_cast<const Document::ColorComponent*>(component);
        stream << "\"color\":";
        writeColor(stream, color_component->color());
        first_component = false;
    }

    component = nullptr;
    error     = shape.queryComponent(TEXT_COID, &component);
    if (error == IDocument::TShape::Error::SUCCESS && component != nullptr)
    {
        const auto* text_component = static_cast<const Document::TextComponent*>(component);
        if (!first_component)
        {
            stream << ',';
        }

        stream << "\"text\":";
        writeTextFormat(stream, text_component->getFormat());
        first_component = false;
    }

    stream << '}';
}

void writeShape(std::ostream& stream, const IDocument::TShape& shape)
{
    stream << '{';
    stream << "\"tid\":" << shape.tid() << ',';
    writeTransform(stream, shape);
    stream << ',';
    writeComponents(stream, shape);
    stream << '}';
}

/* ====================== Deserialization helpers ====================== */

std::unique_ptr<IDocument::TShape> makeShape(IDocument::TShape::TypeID type_id)
{
    switch (type_id)
    {
        case RECTANGLE_SHAPE_ID:
            return std::make_unique<Document::RectangleShape>();

        case ELLIPSE_SHAPE_ID:
            return std::make_unique<Document::EllipseShape>();

        default:
            return nullptr;
    }
}

void readTransform(const JsonValue& shape_value, IDocument::TShape& shape)
{
    const JsonValue* transform = findMember(shape_value, "transform");
    if (transform == nullptr || transform->type != JsonValue::Type::OBJECT)
    {
        return;
    }

    Vec2F value{};
    if (asVec2F(findMember(*transform, "scaling"), value))
    {
        shape.setScaling(value);
    }

    float rotation{};
    if (asFloat(findMember(*transform, "rotation"), rotation))
    {
        shape.setRotation(rotation);
    }

    if (asVec2F(findMember(*transform, "translation"), value))
    {
        shape.setTranslation(value);
    }
}

void readColor(const JsonValue& color_value, IDocument::TShape& shape)
{
    IDocument::AShapeComponent* component = nullptr;
    if (shape.queryComponent(COLOR_COID, &component) != IDocument::TShape::Error::SUCCESS || component == nullptr)
    {
        return;
    }

    IRenderer::Color color{};

    int channel{};
    if (asInt(findMember(color_value, "r"), channel))
    {
        color.r = static_cast<uint8_t>(std::clamp(channel, 0, 255));
    }
    if (asInt(findMember(color_value, "g"), channel))
    {
        color.g = static_cast<uint8_t>(std::clamp(channel, 0, 255));
    }
    if (asInt(findMember(color_value, "b"), channel))
    {
        color.b = static_cast<uint8_t>(std::clamp(channel, 0, 255));
    }
    if (asInt(findMember(color_value, "a"), channel))
    {
        color.a = static_cast<uint8_t>(std::clamp(channel, 0, 255));
    }

    static_cast<Document::ColorComponent*>(component)->setColor(color);
}

void readTextFormat(const JsonValue& text_value, IDocument::TShape& shape)
{
    IDocument::AShapeComponent* component = nullptr;
    if (shape.queryComponent(TEXT_COID, &component) != IDocument::TShape::Error::SUCCESS || component == nullptr)
    {
        return;
    }

    IRenderer::TextFormat format{};

    uint64_t font{};
    if (asUInt64(findMember(text_value, "font"), font))
    {
        format.font = static_cast<IRenderer::FontID>(font);
    }

    int integer{};
    if (asInt(findMember(text_value, "font_size"), integer))
    {
        format.font_size = integer;
    }
    if (asInt(findMember(text_value, "overflow_action"), integer))
    {
        format.overflow_action = static_cast<IRenderer::TextOverflowAction>(integer);
    }
    if (asInt(findMember(text_value, "h_align"), integer))
    {
        format.h_align = static_cast<IRenderer::TextHAlign>(integer);
    }
    if (asInt(findMember(text_value, "w_align"), integer))
    {
        format.w_align = static_cast<IRenderer::TextWAlign>(integer);
    }

    float floating{};
    if (asFloat(findMember(text_value, "padding"), floating))
    {
        format.padding = floating;
    }
    if (asFloat(findMember(text_value, "line_spacing"), floating))
    {
        format.line_spacing = floating;
    }

    bool wrap{};
    if (asBool(findMember(text_value, "wrap"), wrap))
    {
        format.wrap = wrap;
    }

    static_cast<Document::TextComponent*>(component)->setFormat(format);
}

void readComponents(const JsonValue& shape_value, IDocument::TShape& shape)
{
    const JsonValue* components = findMember(shape_value, "components");
    if (components == nullptr || components->type != JsonValue::Type::OBJECT)
    {
        return;
    }

    const JsonValue* color = findMember(*components, "color");
    if (color != nullptr && color->type == JsonValue::Type::OBJECT)
    {
        readColor(*color, shape);
    }

    const JsonValue* text = findMember(*components, "text");
    if (text != nullptr && text->type == JsonValue::Type::OBJECT)
    {
        readTextFormat(*text, shape);
    }
}

std::unique_ptr<IDocument::TShape> readShape(const JsonValue& shape_value)
{
    if (shape_value.type != JsonValue::Type::OBJECT)
    {
        return nullptr;
    }

    float type_id_value{};
    if (!asFloat(findMember(shape_value, "tid"), type_id_value))
    {
        return nullptr;
    }

    std::unique_ptr<IDocument::TShape> shape = makeShape(static_cast<IDocument::TShape::TypeID>(type_id_value));
    if (!shape)
    {
        return nullptr;
    }

    readTransform(shape_value, *shape);
    readComponents(shape_value, *shape);

    return shape;
}

} // namespace

/* ====================== Saver ====================== */

Saver::Saver(const std::string& path) : path_(path), file_(path_, std::ios::out | std::ios::trunc)
{
    if (!file_.is_open())
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Serializer can't open '%s' for writing\n",
                             path_.c_str());
        return;
    }

    file_ << "[\n";
}

bool Saver::isValid() const
{
    return file_.is_open();
}

bool Saver::visit(IDocument::TShape& shape)
{
    return visit(static_cast<const IDocument::TShape&>(shape));
}

bool Saver::visit(const IDocument::TShape& shape)
{
    if (!file_.is_open())
    {
        return false;
    }

    const std::streampos position = file_.tellp();
    if (position == std::streampos(-1))
    {
        return false;
    }

    if (static_cast<std::streamoff>(position) > FIRST_ELEMENT_POSITION)
    {
        file_ << ",\n";
    }

    writeShape(file_, shape);

    return static_cast<bool>(file_);
}

/* ====================== CSerialize ====================== */

void CSerialize::saveToJSON(const std::string& path)
{
    IDocument*                   document = nullptr;
    const ApplicationCore::Error error =
        ApplicationCore::instance().queryInterface(IID_IDOCUMENT, reinterpret_cast<void**>(&document));

    if (error != ApplicationCore::Error::SUCCESS || document == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Serializer can't find the document: %s\n",
                             ApplicationCore::strError(error));
        return;
    }

    {
        Saver saver(path);
        if (!saver.isValid())
        {
            return;
        }

        document->accept(saver);
    }

    std::ofstream file(path, std::ios::out | std::ios::app);
    if (!file.is_open())
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Serializer can't finish '%s'\n", path.c_str());
        return;
    }

    file << "\n]\n";
}

std::vector<std::unique_ptr<IDocument::TShape>> CSerialize::loadFrom(const std::string& path)
{
    std::vector<std::unique_ptr<IDocument::TShape>> shapes;

    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Serializer can't open '%s' for reading\n",
                             path.c_str());
        return shapes;
    }

    const std::string text{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};

    JsonParser parser(text);
    JsonValue  root;
    if (!parser.parse(root))
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Serializer can't parse '%s'\n", path.c_str());
        return shapes;
    }

    const JsonValue* shapes_value = &root;
    if (root.type == JsonValue::Type::OBJECT)
    {
        shapes_value = findMember(root, "shapes");
    }

    if (shapes_value == nullptr || shapes_value->type != JsonValue::Type::ARRAY)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Serializer can't find the shape list in '%s'\n",
                             path.c_str());
        return shapes;
    }

    shapes.reserve(shapes_value->array.size());
    for (const JsonValue& shape_value : shapes_value->array)
    {
        std::unique_ptr<IDocument::TShape> shape = readShape(shape_value);
        if (shape)
        {
            shapes.push_back(std::move(shape));
        }
    }

    return shapes;
}

CSerialize& CSerialize::instance()
{
    static CSerialize serialize;
    return serialize;
}

} // namespace Serialize

} // namespace Kabuto

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    IDocument* document = nullptr;
    if (ApplicationCore::instance().queryInterface(IID_IDOCUMENT, reinterpret_cast<void**>(&document)) !=
            ApplicationCore::Error::SUCCESS ||
        document == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Serialize can't find IDocument");
        return false;
    }

    Kabuto::Serialize::CSerialize& serialize = Kabuto::Serialize::CSerialize::instance();

    const ApplicationCore::Error error = ApplicationCore::instance().registerInterface(
        Kabuto::IID_SERIALIZE, reinterpret_cast<void*>(static_cast<Kabuto::ISerialize*>(&serialize))
    );
    if (error != ApplicationCore::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Failed to register ISerialize: %s",
                             ApplicationCore::strError(error));
        return false;
    }

    return true;
}
