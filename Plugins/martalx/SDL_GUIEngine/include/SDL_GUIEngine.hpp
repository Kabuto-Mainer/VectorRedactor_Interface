#pragma once

#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/Package.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Vec2F.hpp"

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

extern "C" InitPackageFunc INIT_PACKAGE_FUNC_NAME;

namespace MartAlx
{

class SDL_GUIEngine final : public IRenderer, public IInputPump
{
	private:
		struct Font
		{
			std::string                        path;
			std::unordered_map<int, TTF_Font*> instances;
		};

		SDL_Window*   window_   = nullptr;
		SDL_Renderer* renderer_ = nullptr;

		TextureID next_texture_id_ = 1;
		FontID    next_font_id_    = 1;

		std::unordered_map<TextureID, SDL_Texture*> textures_;
		std::unordered_map<FontID, Font>            fonts_;

		SDL_GUIEngine();
		~SDL_GUIEngine() override;

		void      openWindow(const Vec2F& size);
		void      closeWindow();
		void      setRenderColor(const Color& color);
		void      drawPolygon(const std::vector<Vec2F>& points, const Color& color, bool filled);
		TTF_Font* fontInstance(FontID font_id, int size);

	public:
		SDL_GUIEngine(const SDL_GUIEngine&)            = delete;
		SDL_GUIEngine(SDL_GUIEngine&&)                 = delete;
		SDL_GUIEngine& operator=(const SDL_GUIEngine&) = delete;
		SDL_GUIEngine& operator=(SDL_GUIEngine&&)      = delete;

		static SDL_GUIEngine& instance();

		bool isValid() const;

		/* from IRenderer */

		std::optional<TextureID> loadTexture(const std::string& filename) override;
		std::optional<FontID>    loadFont(const std::string& filename) override;

		void renderRect(const Color& color, const BoxF& box, float angle = 0) override;
		void renderRectLines(const Color& color, const BoxF& box, float angle = 0) override;
		void renderTexture(const TextureID& texture, const BoxF& box, float angle = 0) override;
		void renderEllipse(const Color& color, const BoxF& box, float angle = 0) override;
		void renderEllipseLines(const Color& color, const BoxF& box, float angle = 0) override;
		void renderLine(const Color& color, const Vec2F& origin, const Vec2F& end) override;
		void renderVector(const Color& color, const Vec2F& origin, const Vec2F& vector) override;
		void renderPixel(const Color& color, const Vec2F& pos) override;

		void update() override;
		void clear() override;
		void renderText(const Color& color, const BoxF& box, const std::string& text, const TextFormat& format,
				float angle = 0) override;

		/* from IInputPump */

		std::optional<Event> pollEvent() override;
};

}
