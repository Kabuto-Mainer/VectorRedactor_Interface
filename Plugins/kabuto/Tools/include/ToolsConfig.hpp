#pragma once

#include "Interfaces/IRenderer.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace Kabuto
{

namespace Tools
{

struct OverlaySettings
{
    float            handle_radius  = 5.0F;
    IRenderer::Color handle_color   = {42U, 202U, 236U, 255U};
    IRenderer::Color selected_color = {255U, 145U, 0U, 255U};
};

struct AddShapeSettings
{
    float min_drag_size    = 2.0F;
    float menu_item_width  = 140.0F;
    float menu_item_height = 28.0F;
};

struct AddPolygonSettings
{
    float close_distance        = 12.0F;
    float duplicate_distance    = 1.0F;
    float double_click_distance = 8.0F;
    float double_click_seconds  = 0.35F;
};

struct EditPolygonSettings
{
    float vertex_pick_radius    = 10.0F;
    float edge_pick_radius      = 8.0F;
    float double_click_distance = 8.0F;
    float double_click_seconds  = 0.35F;
};

struct CameraSettings
{
    float zoom_step       = 0.1F;
    float pan_step        = 50.0F;
    float fast_pan_factor = 4.0F;
};

struct ShapeOptionConfig
{
    std::string type;
    std::string label;
    std::string icon;
};

struct ToolsConfig
{
    std::string active_tool;

    AddShapeSettings    add_shape;
    AddPolygonSettings  add_polygon;
    EditPolygonSettings edit_polygon;
    CameraSettings      camera;
    OverlaySettings     overlay;

    std::unordered_map<std::string, std::string> icons;
    std::vector<std::string>                     toolbar;
    std::vector<ShapeOptionConfig>               shapes;
};

}; // namespace Tools

}; // namespace Kabuto
