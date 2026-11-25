#pragma once

#include "AddPolygonTool.hpp"
#include "AddShapeTool.hpp"
#include "CameraTool.hpp"
#include "DeleteShapeTool.hpp"
#include "EditPolygonTool.hpp"
#include "Interfaces/IToolManager.hpp"
#include "SetColorTool.hpp"

namespace Kabuto
{

namespace Tools
{

constexpr IToolManager::ATool::ID ADD_POLYGON_TOOL_ID  = AddPolygonTool::ID;
constexpr IToolManager::ATool::ID ADD_SHAPE_TOOL_ID    = AddShapeTool::ID;
constexpr IToolManager::ATool::ID CAMERA_TOOL_ID       = CameraTool::ID;
constexpr IToolManager::ATool::ID DELETE_SHAPE_TOOL_ID = DeleteShapeTool::ID;
constexpr IToolManager::ATool::ID EDIT_POLYGON_TOOL_ID = EditPolygonTool::ID;
constexpr IToolManager::ATool::ID SET_COLOR_TOOL_ID    = SetColorTool::ID;

}; // namespace Tools

}; // namespace Kabuto
