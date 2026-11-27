#include "MainMenu.hpp"

#include "Interfaces/IDesktop.hpp"
#include "Interfaces/IDocument.hpp"

#include <algorithm>
#include <fstream>
#include <memory>
#include <regex>
#include <sstream>
#include <utility>
#include <variant>
#include <dlfcn.h>
#include <filesystem>

namespace Murugov
{

namespace
{

class AllShapesCollector final : public IDocument::Visitor
{
public:
    bool visit(IDocument::TShape& shape) override
    {
        ids_.push_back(shape.id());
        return true;
    }

    bool visit(const IDocument::TShape& shape) override
    {
        ids_.push_back(shape.id());
        return true;
    }

    const std::vector<TObject::ID>& ids() const { return ids_; }

private:
    std::vector<TObject::ID> ids_;
};

void clearDocumentShapes()
{
    IDocument* document = nullptr;
    if (ApplicationCore::instance().queryInterface(IID_IDOCUMENT, reinterpret_cast<void**>(&document)) !=
            ApplicationCore::Error::SUCCESS ||
        document == nullptr)
    {
        return;
    }

    AllShapesCollector collector;
    document->accept(collector);
    for (TObject::ID id : collector.ids())
    {
        document->removeShape(id);
    }
}

void centerAllShapes()
{
    IDocument* document = nullptr;
    if (ApplicationCore::instance().queryInterface(IID_IDOCUMENT, reinterpret_cast<void**>(&document)) !=
            ApplicationCore::Error::SUCCESS ||
        document == nullptr)
    {
        return;
    }

    AllShapesCollector collector;
    document->accept(collector);
    const auto& ids = collector.ids();
    if (ids.empty())
    {
        return;
    }

    Vec2F avg{0.0F, 0.0F};
    int   count = 0;
    for (TObject::ID id : ids)
    {
        if (auto* shape = dynamic_cast<IDocument::TShape*>(ApplicationCore::instance().findObject(id)))
        {
            avg = avg + shape->translation();
            ++count;
        }
    }

    if (count == 0)
    {
        return;
    }

    const Vec2F center{800.0F, 450.0F};
    const Vec2F shift = center - (avg / static_cast<float>(count));
    for (TObject::ID id : ids)
    {
        if (auto* shape = dynamic_cast<IDocument::TShape*>(ApplicationCore::instance().findObject(id)))
        {
            shape->translate(shift);
        }
    }
}

} // namespace

/* ====================== TMainMenuWidget ====================== */

TMainMenuWidget::TMainMenuWidget(float window_width, float bar_height, IRenderer& renderer, IRenderer::FontID font_id)
    : renderer_(renderer), font_id_(font_id), bar_height_(bar_height)
{
    setBox(BoxF(Vec2F{0.0F, 0.0F}, Vec2F{window_width, bar_height_}));
}

void TMainMenuWidget::addMenuItem(const std::string& category, const std::string& title,
                                  IMainMenu::ActionCallback callback)
{
    auto it = std::find_if(categories_.begin(), categories_.end(),
                           [&category](const MenuCategory& c) { return c.name == category; });

    if (it == categories_.end())
    {
        categories_.push_back(MenuCategory{.name = category});
        it = std::prev(categories_.end());
    }

    it->items.push_back(MenuItem{.title = title, .callback = std::move(callback)});
    recalculateLayout();
}

void TMainMenuWidget::recalculateLayout()
{
    float cursor_x = 8.0F;
    for (auto& cat : categories_)
    {
        cat.x_left = cursor_x;
        cat.width  = std::max(56.0F, static_cast<float>(cat.name.size()) * 9.5F + CATEGORY_PAD_X * 2.0F);
        cursor_x += cat.width + 4.0F;
    }
}

BoxF TMainMenuWidget::categoryBox(std::size_t cat_idx) const
{
    const auto& cat = categories_[cat_idx];
    return BoxF(Vec2F{cat.x_left, 2.0F}, Vec2F{cat.x_left + cat.width, bar_height_ - 2.0F});
}

BoxF TMainMenuWidget::dropdownBox(std::size_t cat_idx) const
{
    const auto& cat = categories_[cat_idx];
    const float h   = static_cast<float>(cat.items.size()) * ITEM_HEIGHT + 4.0F;
    return BoxF(Vec2F{cat.x_left, bar_height_}, Vec2F{cat.x_left + DROPDOWN_WIDTH, bar_height_ + h});
}

BoxF TMainMenuWidget::dropdownItemBox(std::size_t cat_idx, std::size_t item_idx) const
{
    const auto& cat = categories_[cat_idx];
    const float y   = bar_height_ + 2.0F + static_cast<float>(item_idx) * ITEM_HEIGHT;
    return BoxF(Vec2F{cat.x_left + 2.0F, y}, Vec2F{cat.x_left + DROPDOWN_WIDTH - 2.0F, y + ITEM_HEIGHT});
}

bool TMainMenuWidget::processEvent(const IInputPump::Event& event)
{
    const auto* mouse = std::get_if<IInputPump::EventMouse>(&event);
    if (mouse == nullptr)
    {
        return false;
    }

    const Vec2F pos = mouse->cursorPos();
    using enum IInputPump::EventMouse::Type;

    if (mouse->type() == MOTION)
    {
        hovered_category_.reset();
        hovered_item_.reset();

        for (std::size_t i = 0; i < categories_.size(); ++i)
        {
            if (categoryBox(i).contains(pos))
            {
                hovered_category_ = i;
                if (open_category_.has_value())
                {
                    open_category_ = i;
                }
                return true;
            }
        }

        if (open_category_.has_value())
        {
            const std::size_t cat_idx = *open_category_;
            if (dropdownBox(cat_idx).contains(pos))
            {
                for (std::size_t j = 0; j < categories_[cat_idx].items.size(); ++j)
                {
                    if (dropdownItemBox(cat_idx, j).contains(pos))
                    {
                        hovered_item_ = j;
                        break;
                    }
                }
                return true;
            }
        }

        return false;
    }

    if (mouse->type() == LEFT_BUTTON_PRESSING)
    {
        // 1. We are testing clicks on category headings in the top bar.
        for (std::size_t i = 0; i < categories_.size(); ++i)
        {
            if (categoryBox(i).contains(pos))
            {
                if (open_category_ == i)
                {
                    open_category_.reset();
                }
                else
                {
                    open_category_ = i;
                }
                return true;
            }
        }

        // 2. If a drop-down menu is open, we check for clicks on its items.
        if (open_category_.has_value())
        {
            const std::size_t cat_idx = *open_category_;
            if (dropdownBox(cat_idx).contains(pos))
            {
                for (std::size_t j = 0; j < categories_[cat_idx].items.size(); ++j)
                {
                    if (dropdownItemBox(cat_idx, j).contains(pos))
                    {
                        auto cb = categories_[cat_idx].items[j].callback;
                        open_category_.reset();
                        hovered_item_.reset();
                        if (cb)
                        {
                            cb();
                        }
                        return true;
                    }
                }
                return true;
            }

            // Clicking outside an open menu closes it
            open_category_.reset();
            hovered_item_.reset();
            return true;
        }

        // Click on an empty space in the topmost menu bar
        if (box().contains(pos))
        {
            return true;
        }
    }

    return false;
}

void TMainMenuWidget::render()
{
    const BoxF bar = box();

    // Top menu bar background
    renderer_.renderRect(IRenderer::Color::DARK_GRAY, bar);
    renderer_.renderLine(IRenderer::Color::GRAY, bar.bottomLeft(), bar.bottomRight());

    IRenderer::TextFormat fmt{
        .font            = font_id_,
        .font_size       = 16,
        .overflow_action = IRenderer::TextOverflowAction::CUTBYBOX,
        .h_align         = IRenderer::TextHAlign::CENTER,
        .w_align         = IRenderer::TextWAlign::CENTER,
        .padding         = 4.0F,
        .line_spacing    = 1.0F,
        .wrap            = false,
    };

    // Rendering category headings
    for (std::size_t i = 0; i < categories_.size(); ++i)
    {
        const BoxF c_box = categoryBox(i);
        if (open_category_ == i)
        {
            renderer_.renderRect(IRenderer::Color::BLUE, c_box);
        }
        else if (hovered_category_ == i)
        {
            renderer_.renderRect(IRenderer::Color::GRAY, c_box);
        }

        renderer_.renderText(IRenderer::Color::WHITE, c_box, categories_[i].name, fmt);
    }

    // Rendering an open drop-down list
    if (open_category_.has_value())
    {
        const std::size_t cat_idx = *open_category_;
        const BoxF        d_box   = dropdownBox(cat_idx);

        renderer_.renderRect(IRenderer::Color::DARK_GRAY, d_box);
        renderer_.renderRectLines(IRenderer::Color::GRAY, d_box);

        fmt.h_align = IRenderer::TextHAlign::LEFT;
        fmt.padding = 10.0F;

        for (std::size_t j = 0; j < categories_[cat_idx].items.size(); ++j)
        {
            const BoxF item_box = dropdownItemBox(cat_idx, j);
            if (hovered_item_ == j)
            {
                renderer_.renderRect(IRenderer::Color::BLUE, item_box);
            }
            renderer_.renderText(IRenderer::Color::WHITE, item_box, categories_[cat_idx].items[j].title, fmt);
        }
    }
}

/* ====================== MainMenu Singleton ====================== */

MainMenu::MainMenu()
{
    auto& core = ApplicationCore::instance();

    IRenderer* renderer = nullptr;
    if (core.queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer)) != ApplicationCore::Error::SUCCESS ||
        renderer == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "MainMenu requires Renderer");
        return;
    }

    IDesktop* desktop = nullptr;
    if (core.queryInterface(IID_DESKTOP, reinterpret_cast<void**>(&desktop)) != ApplicationCore::Error::SUCCESS ||
        desktop == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "MainMenu requires Desktop");
        return;
    }
    IRenderer::FontID font_id = 0;
    if (auto font = renderer->loadFont("/System/Library/Fonts/Helvetica.ttc"); font.has_value())
    {
        font_id = *font;
    }

    std::filesystem::path config_path = "main_menu.json";
    
    Dl_info info;
    if (dladdr(reinterpret_cast<void*>(&MainMenu::instance), &info) != 0 && info.dli_fname != nullptr)
    {
        config_path = std::filesystem::path(info.dli_fname).parent_path() / "main_menu.json";
    }
    
    const Config cfg = loadConfig(config_path.string());
    
    auto widget      = std::make_unique<TMainMenuWidget>(cfg.width, cfg.height, *renderer, font_id);
    widget_          = widget.get();
    widget_id_       = widget->id();

    if (desktop->add(std::move(widget)) != IDesktop::Error::SUCCESS)
    {
        widget_    = nullptr;
        widget_id_ = 0;
        return;
    }

    registerDefaultItems();
    initialized_ = true;
}

MainMenu::~MainMenu()
{
    if (!initialized_ || widget_id_ == 0)
    {
        return;
    }

    IDesktop* desktop = nullptr;
    if (ApplicationCore::instance().queryInterface(IID_DESKTOP, reinterpret_cast<void**>(&desktop)) ==
            ApplicationCore::Error::SUCCESS &&
        desktop != nullptr)
    {
        desktop->remove(widget_id_);
    }
}

void MainMenu::registerDefaultItems()
{
    addMenuItem("File", "Clear Canvas", []() { clearDocumentShapes(); });
    addMenuItem("File", "Exit", []() { ApplicationCore::instance().stop(); });

    addMenuItem("Edit", "Delete All", []() { clearDocumentShapes(); });

    addMenuItem("View", "Center Shapes", []() { centerAllShapes(); });
}

MainMenu::Config MainMenu::loadConfig(const std::string& filepath)
{
    Config cfg;
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "main_menu.json not found, using default size");
        return cfg;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string content = buffer.str();

    auto extractFloat = [&content](const std::string& key, float default_val) -> float
    {
        const std::regex pattern("\"" + key + "\"\\s*:\\s*([-+]?[0-9]*\\.?[0-9]+)");
        std::smatch match;
        if (std::regex_search(content, match, pattern) && match.size() > 1)
        {
            try
            {
                return std::stof(match[1].str());
            }
            catch (...)
            {
                ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "Invalid number format in main_menu.json");
            }
        }
        return default_val;
    };

    cfg.width  = extractFloat("width", cfg.width);
    cfg.height = extractFloat("height", cfg.height);

    return cfg;
}

MainMenu& MainMenu::instance()
{
    static MainMenu inst;
    return inst;
}

void MainMenu::addMenuItem(const std::string& category, const std::string& title, ActionCallback callback)
{
    if (widget_ != nullptr)
    {
        widget_->addMenuItem(category, title, std::move(callback));
    }
}

} // namespace Murugov

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    auto& menu = Murugov::MainMenu::instance();
    if (!menu.isValid())
    {
        return false;
    }

    return ApplicationCore::instance().registerInterface(Murugov::IID_MAIN_MENU,
                                                         static_cast<Murugov::IMainMenu*>(&menu)) ==
           ApplicationCore::Error::SUCCESS;
}