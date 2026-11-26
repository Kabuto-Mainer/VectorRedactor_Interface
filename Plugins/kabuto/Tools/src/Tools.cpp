#include "AddPolygonTool.hpp"
#include "AddShapeTool.hpp"
#include "CameraTool.hpp"
#include "Core/ApplicationCore.hpp"
#include "DeleteShapeTool.hpp"
#include "EditPolygonTool.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolBar.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Interfaces/Package.hpp"
#include "Plugins/kabuto/ToolManager/export/IToolManagerPro.hpp"
#include "SetColorTool.hpp"
#include "ToolIds.hpp"
#include "ToolsConfig.hpp"
#include "Utils/Vec2F.hpp"
#include "export/EllipseShape.hpp"
#include "export/ICamera.hpp"
#include "export/IPalette.hpp"
#include "export/RectangleShape.hpp"

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <json/json.h>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{

using Kabuto::Tools::ShapeOptionConfig;
using Kabuto::Tools::ToolsConfig;

constexpr const char* TOOLS_CONFIG_FILENAME = "Tools.json";

constexpr uint8_t OPAQUE_ALPHA = 255;

constexpr std::size_t COLOR_CHANNEL_COUNT = 3;
constexpr std::size_t HEX_COLOR_LENGTH    = 1 + 2 * COLOR_CHANNEL_COUNT;

struct ToolDescriptor
{
    const char*             key;
    IToolManager::ATool::ID id;
    const char*             icon_key;
}; // struct ToolDescriptor

constexpr ToolDescriptor TOOL_DESCRIPTORS[] = {
    {   "add_shape",    Kabuto::Tools::ADD_SHAPE_TOOL_ID,    "rectangle"},
    { "add_polygon",  Kabuto::Tools::ADD_POLYGON_TOOL_ID,      "polygon"},
    {"edit_polygon", Kabuto::Tools::EDIT_POLYGON_TOOL_ID, "edit_polygon"},
    {"delete_shape", Kabuto::Tools::DELETE_SHAPE_TOOL_ID,       "delete"},
    {   "set_color",    Kabuto::Tools::SET_COLOR_TOOL_ID,        "color"},
    {      "camera",       Kabuto::Tools::CAMERA_TOOL_ID,       "camera"},
};

ToolsConfig defaultToolsConfig()
{
    ToolsConfig config;

    config.active_tool = "add_shape";
    config.icons       = {
        {   "rectangle",    "assets/rectangle.png"},
        {     "ellipse",      "assets/ellipse.png"},
        {     "polygon",      "assets/polygon.png"},
        {"edit_polygon", "assets/edit_polygon.png"},
        {      "delete",        "assets/clean.png"},
        {       "color",         "assets/fill.png"},
        {      "camera",       "assets/camera.png"},
    };
    config.toolbar = {"add_shape", "add_polygon", "edit_polygon", "delete_shape", "set_color", "camera"};
    config.shapes  = {
        {"rectangle", "Rectangle", "rectangle"},
        {  "ellipse",   "Ellipse",   "ellipse"},
    };

    return config;
}

const ToolDescriptor* findTool(const std::string& key)
{
    for (const ToolDescriptor& descriptor : TOOL_DESCRIPTORS)
    {
        if (key == descriptor.key)
        {
            return &descriptor;
        }
    }

    return nullptr;
}

std::filesystem::path executableDirectory()
{
    std::error_code             error;
    const std::filesystem::path self = std::filesystem::read_symlink("/proc/self/exe", error);
    if (!error && !self.empty())
    {
        return self.parent_path();
    }

    return std::filesystem::current_path(error);
}

std::filesystem::path configFilePath()
{
    return executableDirectory() / "config" / TOOLS_CONFIG_FILENAME;
}

void logInvalidConfigField(const std::filesystem::path& path, const char* field)
{
    ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "Tools config '%s': invalid '%s', using default",
                         path.string().c_str(), field);
}

float readFloat(const Json::Value& object, const char* field, float fallback, bool positive_only,
                const std::filesystem::path& path)
{
    const Json::Value& value = object[field];

    if (value.isNull())
    {
        return fallback;
    }

    if (!value.isNumeric())
    {
        logInvalidConfigField(path, field);
        return fallback;
    }

    constexpr double FLOAT_MAX = static_cast<double>(std::numeric_limits<float>::max());

    const double number = value.asDouble();
    if (!std::isfinite(number) || number > FLOAT_MAX || number < 0.0 || (positive_only && number <= 0.0))
    {
        logInvalidConfigField(path, field);
        return fallback;
    }

    return static_cast<float>(number);
}

int hexDigit(char symbol)
{
    if (symbol >= '0' && symbol <= '9')
    {
        return symbol - '0';
    }
    if (symbol >= 'a' && symbol <= 'f')
    {
        return symbol - 'a' + 10;
    }
    if (symbol >= 'A' && symbol <= 'F')
    {
        return symbol - 'A' + 10;
    }

    return -1;
}

std::optional<IRenderer::Color> parseColor(std::string_view text)
{
    if ((text.size() != HEX_COLOR_LENGTH && text.size() != HEX_COLOR_LENGTH + 2) || text.front() != '#')
    {
        return std::nullopt;
    }

    uint8_t channels[COLOR_CHANNEL_COUNT] = {};
    for (std::size_t i = 0; i < COLOR_CHANNEL_COUNT; ++i)
    {
        const int high = hexDigit(text[1 + 2 * i]);
        const int low  = hexDigit(text[2 + 2 * i]);
        if (high < 0 || low < 0)
        {
            return std::nullopt;
        }

        channels[i] = static_cast<uint8_t>(high * 16 + low);
    }

    uint8_t alpha = OPAQUE_ALPHA;
    if (text.size() == HEX_COLOR_LENGTH + 2)
    {
        const int high = hexDigit(text[7]);
        const int low  = hexDigit(text[8]);
        if (high < 0 || low < 0)
        {
            return std::nullopt;
        }
        alpha = static_cast<uint8_t>(high * 16 + low);
    }

    return IRenderer::Color{channels[0], channels[1], channels[2], alpha};
}

IRenderer::Color readColor(const Json::Value& object, const char* field, const IRenderer::Color& fallback,
                           const std::filesystem::path& path)
{
    const Json::Value& value = object[field];

    if (value.isNull())
    {
        return fallback;
    }

    if (!value.isString())
    {
        logInvalidConfigField(path, field);
        return fallback;
    }

    const std::optional<IRenderer::Color> color = parseColor(value.asString());
    if (!color)
    {
        logInvalidConfigField(path, field);
        return fallback;
    }

    return *color;
}

const Json::Value* objectField(const Json::Value& root, const char* field, const std::filesystem::path& path)
{
    const Json::Value& value = root[field];

    if (value.isNull())
    {
        return nullptr;
    }

    if (!value.isObject())
    {
        logInvalidConfigField(path, field);
        return nullptr;
    }

    return &value;
}

void readIcons(const Json::Value& root, const std::filesystem::path& path, ToolsConfig& config)
{
    const Json::Value& icons = root["icons"];

    if (icons.isNull())
    {
        return;
    }

    if (!icons.isObject())
    {
        logInvalidConfigField(path, "icons");
        return;
    }

    for (const std::string& key : icons.getMemberNames())
    {
        const Json::Value& value = icons[key];
        if (!value.isString() || value.asString().empty())
        {
            ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "Tools config '%s': invalid 'icons.%s', skipped",
                                 path.string().c_str(), key.c_str());
            continue;
        }

        config.icons[key] = value.asString();
    }
}

void readToolbar(const Json::Value& root, const std::filesystem::path& path, ToolsConfig& config)
{
    const Json::Value& toolbar = root["toolbar"];

    if (toolbar.isNull())
    {
        return;
    }

    if (!toolbar.isArray())
    {
        logInvalidConfigField(path, "toolbar");
        return;
    }

    std::vector<std::string> parsed;
    for (const Json::Value& item : toolbar)
    {
        if (!item.isString() || findTool(item.asString()) == nullptr)
        {
            ApplicationCore::log(ApplicationCore::LogLevel::WARNING,
                                 "Tools config '%s': invalid 'toolbar' entry, skipped", path.string().c_str());
            continue;
        }

        parsed.push_back(item.asString());
    }

    if (parsed.empty())
    {
        ApplicationCore::log(ApplicationCore::LogLevel::WARNING,
                             "Tools config '%s': 'toolbar' is empty, using defaults", path.string().c_str());
        return;
    }

    config.toolbar = std::move(parsed);
}

std::string defaultShapeLabel(const std::string& type)
{
    return type == "rectangle" ? "Rectangle" : "Ellipse";
}

void readShapes(const Json::Value& add_shape, const std::filesystem::path& path, ToolsConfig& config)
{
    const Json::Value& shapes = add_shape["shapes"];

    if (shapes.isNull())
    {
        return;
    }

    if (!shapes.isArray())
    {
        logInvalidConfigField(path, "add_shape.shapes");
        return;
    }

    std::vector<ShapeOptionConfig> parsed;
    for (const Json::Value& item : shapes)
    {
        if (!item.isObject())
        {
            ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "Tools config '%s': invalid shape entry, skipped",
                                 path.string().c_str());
            continue;
        }

        const Json::Value& type = item["type"];
        if (!type.isString() || (type.asString() != "rectangle" && type.asString() != "ellipse"))
        {
            ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "Tools config '%s': invalid shape type, skipped",
                                 path.string().c_str());
            continue;
        }

        ShapeOptionConfig shape;
        shape.type  = type.asString();
        shape.label = defaultShapeLabel(shape.type);
        shape.icon  = shape.type;

        const Json::Value& label = item["label"];
        if (label.isString() && !label.asString().empty())
        {
            shape.label = label.asString();
        }
        else if (!label.isNull())
        {
            logInvalidConfigField(path, "add_shape.shapes[].label");
        }

        const Json::Value& icon = item["icon"];
        if (icon.isString() && !icon.asString().empty())
        {
            shape.icon = icon.asString();
        }
        else if (!icon.isNull())
        {
            logInvalidConfigField(path, "add_shape.shapes[].icon");
        }

        parsed.push_back(std::move(shape));
    }

    if (parsed.empty())
    {
        ApplicationCore::log(ApplicationCore::LogLevel::WARNING,
                             "Tools config '%s': 'add_shape.shapes' is empty, using defaults", path.string().c_str());
        return;
    }

    config.shapes = std::move(parsed);
}

ToolsConfig loadToolsConfig(const std::filesystem::path& path)
{
    ToolsConfig config = defaultToolsConfig();

    std::ifstream file(path);
    if (!file.is_open())
    {
        ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "Tools config '%s' not found, using defaults",
                             path.string().c_str());
        return config;
    }

    Json::CharReaderBuilder builder;
    Json::Value             root;
    std::string             errors;

    if (!Json::parseFromStream(builder, file, &root, &errors) || !root.isObject())
    {
        ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "Tools config '%s' is invalid (%s), using defaults",
                             path.string().c_str(), errors.empty() ? "root is not an object" : errors.c_str());
        return config;
    }

    const Json::Value& active_tool = root["active_tool"];
    if (active_tool.isString() && !active_tool.asString().empty())
    {
        if (findTool(active_tool.asString()) != nullptr)
        {
            config.active_tool = active_tool.asString();
        }
        else
        {
            logInvalidConfigField(path, "active_tool");
        }
    }
    else if (!active_tool.isNull())
    {
        logInvalidConfigField(path, "active_tool");
    }

    readIcons(root, path, config);
    readToolbar(root, path, config);

    if (const Json::Value* add_shape = objectField(root, "add_shape", path))
    {
        config.add_shape.min_drag_size =
            readFloat(*add_shape, "min_drag_size", config.add_shape.min_drag_size, false, path);
        config.add_shape.menu_item_width =
            readFloat(*add_shape, "menu_item_width", config.add_shape.menu_item_width, true, path);
        config.add_shape.menu_item_height =
            readFloat(*add_shape, "menu_item_height", config.add_shape.menu_item_height, true, path);
        readShapes(*add_shape, path, config);
    }

    if (const Json::Value* add_polygon = objectField(root, "add_polygon", path))
    {
        config.add_polygon.close_distance =
            readFloat(*add_polygon, "close_distance", config.add_polygon.close_distance, false, path);
        config.add_polygon.duplicate_distance =
            readFloat(*add_polygon, "duplicate_distance", config.add_polygon.duplicate_distance, false, path);
        config.add_polygon.double_click_distance =
            readFloat(*add_polygon, "double_click_distance", config.add_polygon.double_click_distance, false, path);
        config.add_polygon.double_click_seconds =
            readFloat(*add_polygon, "double_click_seconds", config.add_polygon.double_click_seconds, false, path);
    }

    if (const Json::Value* edit_polygon = objectField(root, "edit_polygon", path))
    {
        config.edit_polygon.vertex_pick_radius =
            readFloat(*edit_polygon, "vertex_pick_radius", config.edit_polygon.vertex_pick_radius, false, path);
        config.edit_polygon.edge_pick_radius =
            readFloat(*edit_polygon, "edge_pick_radius", config.edit_polygon.edge_pick_radius, false, path);
        config.edit_polygon.double_click_distance =
            readFloat(*edit_polygon, "double_click_distance", config.edit_polygon.double_click_distance, false, path);
        config.edit_polygon.double_click_seconds =
            readFloat(*edit_polygon, "double_click_seconds", config.edit_polygon.double_click_seconds, false, path);
    }

    if (const Json::Value* camera = objectField(root, "camera", path))
    {
        config.camera.zoom_step = readFloat(*camera, "zoom_step", config.camera.zoom_step, true, path);
        config.camera.pan_step  = readFloat(*camera, "pan_step", config.camera.pan_step, true, path);
        config.camera.fast_pan_factor =
            readFloat(*camera, "fast_pan_factor", config.camera.fast_pan_factor, true, path);
    }

    if (const Json::Value* overlay = objectField(root, "overlay", path))
    {
        config.overlay.handle_radius  = readFloat(*overlay, "handle_radius", config.overlay.handle_radius, true, path);
        config.overlay.handle_color   = readColor(*overlay, "handle_color", config.overlay.handle_color, path);
        config.overlay.selected_color = readColor(*overlay, "selected_color", config.overlay.selected_color, path);
    }

    return config;
}

template<typename T>
T* queryInterface(const IID& iid)
{
    void* raw = nullptr;
    if (ApplicationCore::instance().queryInterface(iid, &raw) != ApplicationCore::Error::SUCCESS)
    {
        return nullptr;
    }

    return static_cast<T*>(raw);
}

/**
 * Keeps a deactivation callback for every tool that owns auxiliary objects.
 * The tool manager only knows the common ATool interface, so the plugin itself
 * maps tool ids to the concrete tools it has created.
 */
class ToolDeactivationRegistry final
{
public:
    using Callback = std::function<void()>;

    void remember(IToolManager::ATool::ID tool_id, Callback callback)
    {
        callbacks_.insert_or_assign(tool_id, std::move(callback));
    }

    void forget(IToolManager::ATool::ID tool_id)
    {
        callbacks_.erase(tool_id);
    }

    void deactivate(IToolManager::ATool::ID tool_id) const
    {
        const auto found = callbacks_.find(tool_id);
        if (found != callbacks_.end())
        {
            found->second();
        }
    }

    void deactivateAll() const
    {
        for (const auto& [tool_id, callback] : callbacks_)
        {
            callback();
        }
    }

    void clear()
    {
        callbacks_.clear();
    }

private:
    std::unordered_map<IToolManager::ATool::ID, Callback> callbacks_;
}; // class ToolDeactivationRegistry

ToolDeactivationRegistry& deactivationRegistry()
{
    static ToolDeactivationRegistry registry;
    return registry;
}

class ToolRegistrationGuard final
{
public:
    ~ToolRegistrationGuard()
    {
        deactivationRegistry().deactivateAll();
        deactivationRegistry().clear();

        IToolManager* manager = queryInterface<IToolManager>(IID_TOOL_MANAGER);
        if (manager == nullptr)
        {
            return;
        }

        for (const IToolManager::ATool::ID tool_id : ids_)
        {
            manager->removeTool(tool_id);
        }
    }

    void remember(IToolManager::ATool::ID tool_id)
    {
        ids_.push_back(tool_id);
    }

private:
    std::vector<IToolManager::ATool::ID> ids_;
}; // class ToolRegistrationGuard

ToolRegistrationGuard& registrationGuard()
{
    static ToolRegistrationGuard guard;
    return guard;
}

void subscribeToToolSwitches()
{
    Kabuto::IToolManagerPro* manager = queryInterface<Kabuto::IToolManagerPro>(Kabuto::IID_TOOL_MANAGER_PRO);
    if (manager == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::WARNING,
                             "Tools can't observe tool switches: IToolManagerPro is not available");
        return;
    }

    manager->followSwitchTool(
        [](Kabuto::IToolManagerPro::ID previous, Kabuto::IToolManagerPro::ID current)
        {
            /* NO_TOOL means the previous tool is being removed and has already
               been destroyed, so only the registration guard may clean it up. */
            if (current == Kabuto::IToolManagerPro::NO_TOOL)
            {
                return;
            }

            deactivationRegistry().deactivate(previous);
        }
    );

    manager->followRemoveTool([](Kabuto::IToolManagerPro::ID tool_id) { deactivationRegistry().forget(tool_id); });
}

std::optional<IRenderer::TextureID> loadIcon(IRenderer& renderer, const char* path)
{
    std::optional<IRenderer::TextureID> texture = renderer.loadTexture(path);
    if (!texture)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "Tools can't load the icon '%s'", path);
    }

    return texture;
}

Kabuto::Tools::AddShapeTool::Factory rectangleFactory()
{
    return [](const Vec2F& size) -> std::unique_ptr<IDocument::TShape>
    { return std::make_unique<Kabuto::Shapes::RectangleShape>(size); };
}

Kabuto::Tools::AddShapeTool::Factory ellipseFactory()
{
    return [](const Vec2F& size) -> std::unique_ptr<IDocument::TShape>
    { return std::make_unique<Kabuto::Shapes::EllipseShape>(size); };
}

std::unique_ptr<Kabuto::Tools::AddShapeTool> makeAddShapeTool(IDocument& document, Kabuto::IPalette& palette,
                                                              Kabuto::ICamera& camera, IRenderer& renderer,
                                                              const ToolsConfig& config)
{
    std::vector<Kabuto::Tools::AddShapeTool::ShapeOption> options;
    options.reserve(config.shapes.size());

    for (const Kabuto::Tools::ShapeOptionConfig& shape : config.shapes)
    {
        Kabuto::Tools::AddShapeTool::Factory factory;

        if (shape.type == "rectangle")
        {
            factory = rectangleFactory();
        }
        else if (shape.type == "ellipse")
        {
            factory = ellipseFactory();
        }
        else
        {
            ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "Tools config: unknown shape type '%s', skipped",
                                 shape.type.c_str());
            continue;
        }

        std::optional<IRenderer::TextureID> icon;
        const std::string                   icon_key = shape.icon.empty() ? shape.type : shape.icon;
        const auto                          found    = config.icons.find(icon_key);
        if (found != config.icons.end())
        {
            icon = loadIcon(renderer, found->second.c_str());
        }

        options.push_back(Kabuto::Tools::AddShapeTool::ShapeOption{shape.label, icon, factory});
    }

    return std::make_unique<Kabuto::Tools::AddShapeTool>(document, palette, camera, std::move(options),
                                                         config.add_shape);
}

} // namespace

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    IDocument*        document     = queryInterface<IDocument>(IID_IDOCUMENT);
    IToolManager*     tool_manager = queryInterface<IToolManager>(IID_TOOL_MANAGER);
    Kabuto::IPalette* palette      = queryInterface<Kabuto::IPalette>(Kabuto::IID_PALETTE);
    Kabuto::ICamera*  camera       = queryInterface<Kabuto::ICamera>(Kabuto::IID_ICAMERA);
    IToolBar*         tool_bar     = queryInterface<IToolBar>(IID_TOOL_BAR);
    IRenderer*        renderer     = queryInterface<IRenderer>(IID_RENDERER);

    if (document == nullptr ||
        tool_manager == nullptr ||
        palette == nullptr ||
        camera == nullptr ||
        tool_bar == nullptr ||
        renderer == nullptr)
    {
        ApplicationCore::log(
            ApplicationCore::LogLevel::CRITICAL,
            "Tools can't find the document, the tool manager, the palette, the camera, the tool bar or the renderer"
        );
        return false;
    }

    /* The registration guard deactivates the tools on unload, so its registry
       has to be constructed first and therefore destroyed last. */
    (void)deactivationRegistry();
    subscribeToToolSwitches();

    const ToolsConfig config = loadToolsConfig(configFilePath());

    std::unique_ptr<Kabuto::Tools::AddShapeTool> add_shape =
        makeAddShapeTool(*document, *palette, *camera, *renderer, config);
    Kabuto::Tools::AddShapeTool* add_shape_tool = add_shape.get();

    if (tool_manager->addTool(std::move(add_shape)) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't register the add shape tool");
        return false;
    }
    registrationGuard().remember(Kabuto::Tools::ADD_SHAPE_TOOL_ID);
    deactivationRegistry().remember(Kabuto::Tools::ADD_SHAPE_TOOL_ID,
                                    [add_shape_tool] { add_shape_tool->deactivate(); });

    auto add_polygon =
        std::make_unique<Kabuto::Tools::AddPolygonTool>(*document, *palette, *camera, config.add_polygon);
    Kabuto::Tools::AddPolygonTool* add_polygon_tool = add_polygon.get();
    if (tool_manager->addTool(std::move(add_polygon)) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't register the add polygon tool");
        return false;
    }
    registrationGuard().remember(Kabuto::Tools::ADD_POLYGON_TOOL_ID);
    deactivationRegistry().remember(Kabuto::Tools::ADD_POLYGON_TOOL_ID,
                                    [add_polygon_tool] { add_polygon_tool->deactivate(); });

    auto edit_polygon =
        std::make_unique<Kabuto::Tools::EditPolygonTool>(*document, config.edit_polygon, config.overlay);
    Kabuto::Tools::EditPolygonTool* edit_polygon_tool = edit_polygon.get();
    if (tool_manager->addTool(std::move(edit_polygon)) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't register the edit polygon tool");
        return false;
    }
    registrationGuard().remember(Kabuto::Tools::EDIT_POLYGON_TOOL_ID);
    deactivationRegistry().remember(Kabuto::Tools::EDIT_POLYGON_TOOL_ID,
                                    [edit_polygon_tool] { edit_polygon_tool->deactivate(); });

    auto delete_shape = std::make_unique<Kabuto::Tools::DeleteShapeTool>(*document);
    if (tool_manager->addTool(std::move(delete_shape)) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't register the delete shape tool");
        return false;
    }
    registrationGuard().remember(Kabuto::Tools::DELETE_SHAPE_TOOL_ID);

    auto set_color = std::make_unique<Kabuto::Tools::SetColorTool>(*document, *palette);
    if (tool_manager->addTool(std::move(set_color)) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't register the set color tool");
        return false;
    }
    registrationGuard().remember(Kabuto::Tools::SET_COLOR_TOOL_ID);

    auto                       camera_tool     = std::make_unique<Kabuto::Tools::CameraTool>(*camera, config.camera);
    Kabuto::Tools::CameraTool* camera_tool_raw = camera_tool.get();
    if (tool_manager->addTool(std::move(camera_tool)) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't register the camera tool");
        return false;
    }
    registrationGuard().remember(Kabuto::Tools::CAMERA_TOOL_ID);
    deactivationRegistry().remember(Kabuto::Tools::CAMERA_TOOL_ID,
                                    [camera_tool_raw] { camera_tool_raw->deactivate(); });

    for (const std::string& key : config.toolbar)
    {
        const ToolDescriptor* descriptor = findTool(key);
        if (descriptor == nullptr)
        {
            continue;
        }

        const auto icon_path = config.icons.find(descriptor->icon_key);
        if (icon_path == config.icons.end())
        {
            continue;
        }

        const std::optional<IRenderer::TextureID> icon = loadIcon(*renderer, icon_path->second.c_str());
        if (!icon)
        {
            continue;
        }

        tool_bar->addTool(descriptor->id, *icon);
    }

    const ToolDescriptor*         active    = findTool(config.active_tool);
    const IToolManager::ATool::ID active_id = active != nullptr ? active->id : Kabuto::Tools::ADD_SHAPE_TOOL_ID;
    if (tool_manager->setActiveTool(active_id) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't activate the tool '%s'",
                             config.active_tool.c_str());
        return false;
    }

    return true;
}
