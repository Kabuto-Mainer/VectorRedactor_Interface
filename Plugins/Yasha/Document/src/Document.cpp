#include "Document.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDesktop.hpp"

#include <json/json.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{
constexpr const char* DOCUMENT_FILE = "drawing.json";

bool readNumber(const Json::Value& object, const char* key, float& result)
{
    if (!object.isMember(key) || !object[key].isNumeric())
    {
        return false;
    }
    result = object[key].asFloat();
    return std::isfinite(result);
}

bool readColor(const Json::Value& value, IRenderer::Color& color)
{
    if (!value.isArray() || value.size() != 4)
    {
        return false;
    }
    unsigned char channels[4]{};
    for (Json::ArrayIndex i = 0; i < 4; ++i)
    {
        if (!value[i].isIntegral() || value[i].asDouble() < 0 || value[i].asDouble() > 255)
        {
            return false;
        }
        channels[i] = static_cast<unsigned char>(value[i].asInt());
    }
    color = {channels[0], channels[1], channels[2], channels[3]};
    return true;
}
} // namespace

namespace Yasha {
bool Document::registerShapeType(TShape::TypeID id, std::string name, ShapeFactory factory, ShapeColor color)
{
    if (name.empty() || factory == nullptr || color == nullptr || shape_types_.contains(id))
    {
        return false;
    }
    for (const auto& [other_id, other] : shape_types_)
    {
        if (other.name == name)
        {
            return false;
        }
    }
    shape_types_.emplace(id, ShapeType{std::move(name), factory, color});
    return true;
}

bool Document::save() const
{
    Json::Value root;
    root["version"] = 1;
    root["shapes"] = Json::arrayValue;
    for (const auto& child : children())
    {
        const auto* shape = dynamic_cast<const TShape*>(child.get());
        if (shape == nullptr)
        {
            continue;
        }
        const auto it = shape_types_.find(shape->tid());
        if (it == shape_types_.end())
        {
            ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Cannot save unknown shape type");
            return false;
        }
        const auto color = it->second.color(*shape);
        Json::Value item;
        item["type"] = it->second.name;
        item["x"] = shape->translation().x();
        item["y"] = shape->translation().y();
        item["scale_x"] = shape->scaling().x();
        item["scale_y"] = shape->scaling().y();
        item["rotation"] = shape->rotation();
        item["color"] = Json::arrayValue;
        item["color"].append(color.r);
        item["color"].append(color.g);
        item["color"].append(color.b);
        item["color"].append(color.a);
        root["shapes"].append(std::move(item));
    }

    const std::string temporary = std::string(DOCUMENT_FILE) + ".tmp";
    std::ofstream file(temporary, std::ios::trunc);
    if (!file)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Cannot open %s for writing", temporary.c_str());
        return false;
    }
    Json::StreamWriterBuilder writer;
    file << Json::writeString(writer, root);
    file.close();
    if (!file)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Cannot write %s", temporary.c_str());
        return false;
    }
    std::error_code error;
    std::filesystem::rename(temporary, DOCUMENT_FILE, error);
    if (error)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Cannot save %s: %s", DOCUMENT_FILE,
                             error.message().c_str());
        return false;
    }
    ApplicationCore::log(ApplicationCore::LogLevel::OK, "Saved %s", DOCUMENT_FILE);
    return true;
}

bool Document::load()
{
    std::ifstream file(DOCUMENT_FILE);
    if (!file)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Cannot open %s", DOCUMENT_FILE);
        return false;
    }
    Json::Value root;
    Json::CharReaderBuilder reader;
    std::string errors;
    if (!Json::parseFromStream(reader, file, &root, &errors) || !root.isObject() ||
        !root["version"].isInt() || root["version"].asInt() != 1 || !root["shapes"].isArray())
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Invalid %s: %s", DOCUMENT_FILE, errors.c_str());
        return false;
    }

    std::vector<std::unique_ptr<TShape>> loaded;
    for (const auto& item : root["shapes"])
    {
        if (!item.isObject() || !item["type"].isString())
        {
            ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Invalid shape in %s", DOCUMENT_FILE);
            return false;
        }
        const auto it = std::find_if(shape_types_.begin(), shape_types_.end(),
                                     [&item](const auto& entry) { return entry.second.name == item["type"].asString(); });
        float x{}, y{}, scale_x{}, scale_y{}, rotation{};
        IRenderer::Color color;
        if (it == shape_types_.end() || !readNumber(item, "x", x) || !readNumber(item, "y", y) ||
            !readNumber(item, "scale_x", scale_x) || !readNumber(item, "scale_y", scale_y) ||
            !readNumber(item, "rotation", rotation) || !readColor(item["color"], color))
        {
            ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Invalid shape in %s", DOCUMENT_FILE);
            return false;
        }
        auto shape = it->second.factory(color);
        if (!shape)
        {
            return false;
        }
        shape->setScaling(Vec2F{scale_x, scale_y});
        shape->setRotation(rotation);
        shape->setTranslation(Vec2F{x, y});
        loaded.push_back(std::move(shape));
    }

    children().clear();
    for (auto& shape : loaded)
    {
        addShape(std::move(shape));
    }
    ApplicationCore::log(ApplicationCore::LogLevel::OK, "Loaded %s", DOCUMENT_FILE);
    return true;
}

void Document::addShape(std::unique_ptr<TShape> shape)
{
    if (shape == nullptr || shape->parent() != nullptr)
    {
        return;
    }
    addChild(std::move(shape));
}

void Document::removeShape(TObject::ID shape_id)
{
    for (auto it = children().begin(); it != children().end(); ++it)
    {
        if ((*it)->id() == shape_id)
        {
            children().erase(it);
            return;
        }
    }
}

std::optional<TObject::ID> Document::hitTest(const Vec2F& pos)
{
    for (auto it = children().rbegin(); it != children().rend(); ++it)
    {
        auto* shape = dynamic_cast<TShape*>(it->get());
        if (shape != nullptr && shape->hitTest(pos))
        {
            return shape->id();
        }
    }
    return std::nullopt;
}

void Document::accept(Visitor& visitor) const
{
    for (const auto& child : children())
    {
        if (const auto* shape = dynamic_cast<const TShape*>(child.get()); shape != nullptr && !visitor.visit(*shape))
        {
            return;
        }
    }
}

bool Document::processEvent(const IInputPump::Event& event)
{
    return processEventChildren(event);
}

void Document::render()
{
    renderChildren();
}

ApplicationCore::Error initializeDocument()
{
    auto&     core    = ApplicationCore::instance();
    IDesktop* desktop = nullptr;
    if (core.queryInterface(IID_DESKTOP, reinterpret_cast<void**>(&desktop)) != ApplicationCore::Error::SUCCESS ||
        desktop == nullptr)
    {
        return ApplicationCore::Error::INTERFACE_NOT_FOUND;
    }

    auto              document  = std::make_unique<Document>();
    const TObject::ID id        = document->id();
    auto*             interface = static_cast<IDocument*>(document.get());
    if (desktop->add(std::move(document)) != IDesktop::Error::SUCCESS)
    {
        return ApplicationCore::Error::INVALID_ARGS;
    }

    const auto error = core.registerInterface(IID_IDOCUMENT, interface);
    if (error != ApplicationCore::Error::SUCCESS)
    {
        desktop->remove(id);
    }
    return error;
}
} // namespace Yasha
