#include <SDL2/SDL.h>
#include <X11/Xlib.h>
#include <Imgui.h>

struct MainWindow {
    SDL_Window* sdl_window;
    SDL_Renderer* sdl_renderer;
    Imgui imgui;

    // FIXME: errdefer.
    static ErrorOr<MainWindow> create(c_string name, u32 width, u32 height)
    {
        auto x = SDL_WINDOWPOS_CENTERED;
        auto y = SDL_WINDOWPOS_CENTERED;

        auto window = SDL_CreateWindow(name, x, y, width, height,
            SDL_WINDOW_HIDDEN);
        if (!window)
            return Error::from_string_literal(SDL_GetError());
        auto renderer = SDL_CreateRenderer(window, -1,
            SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
        if (!renderer)
            return Error::from_string_literal(SDL_GetError());

        auto imgui = TRY(Imgui::create(window, renderer));

        return MainWindow { window, renderer, imgui };
    }

    constexpr void destroy() const
    {
        if (is_valid()) {
            imgui.destroy();
            SDL_DestroyRenderer(sdl_renderer);
            SDL_DestroyWindow(sdl_window);
            mutable_self().invalidate();
        }
    }

    void show() const
    {
        SDL_ShowWindow(sdl_window);
    }

private:
    constexpr bool is_valid() const { return sdl_window && sdl_renderer; }

    constexpr MainWindow& mutable_self() const
    {
        return *const_cast<MainWindow*>(this);
    }

    constexpr void invalidate()
    {
        sdl_window = nullptr;
        sdl_renderer = nullptr;
    }
};

