#pragma once

#include "Core/ApplicationCore.hpp"
#include "IMurugovMainMenu.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/Package.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/TWidget.hpp"
#include "Utils/Vec2F.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

extern "C" InitPackageFunc INIT_PACKAGE_FUNC_NAME;

namespace Murugov
{

class TMainMenuWidget final : public TWidget
{
public:
    struct MenuItem
    {
        std::string               title;
        IMainMenu::ActionCallback callback;
    };

    struct MenuCategory
    {
        std::string           name;
        float                 x_left = 0.0F;
        float                 width  = 70.0F;
        std::vector<MenuItem> items;
    };

    static constexpr float ITEM_HEIGHT     = 28.0F;
    static constexpr float DROPDOWN_WIDTH  = 180.0F;
    static constexpr float CATEGORY_PAD_X  = 14.0F;

    TMainMenuWidget(float window_width, float bar_height, IRenderer& renderer, IRenderer::FontID font_id);

    void addMenuItem(const std::string& category, const std::string& title, IMainMenu::ActionCallback callback);

    bool processEvent(const IInputPump::Event& event) override;
    void render() override;

private:
    void  recalculateLayout();
    BoxF  categoryBox(std::size_t cat_idx) const;
    BoxF  dropdownBox(std::size_t cat_idx) const;
    BoxF  dropdownItemBox(std::size_t cat_idx, std::size_t item_idx) const;

    IRenderer&                renderer_;
    IRenderer::FontID         font_id_    = 0;
    float                     bar_height_ = 30.0F;
    std::vector<MenuCategory> categories_;

    std::optional<std::size_t> open_category_;
    std::optional<std::size_t> hovered_category_;
    std::optional<std::size_t> hovered_item_;
};

class MainMenu final : public IMainMenu
{
private:
    struct Config
    {
        float width  = 1600.0F;
        float height = 30.0F;
    };

    bool             initialized_ = false;
    TMainMenuWidget* widget_      = nullptr;
    TObject::ID      widget_id_   = 0;

    MainMenu();
    ~MainMenu() override;

    void          registerDefaultItems();
    static Config loadConfig(const std::string& filepath);

public:
    MainMenu(const MainMenu&)            = delete;
    MainMenu(MainMenu&&)                 = delete;
    MainMenu& operator=(const MainMenu&) = delete;
    MainMenu& operator=(MainMenu&&)      = delete;

    static MainMenu& instance();

    bool isValid() const { return initialized_; }

    /* from IMainMenu */

    void addMenuItem(const std::string& category, const std::string& title, ActionCallback callback) override;
};

} // namespace Murugov