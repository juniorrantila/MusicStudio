#pragma once
#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_sdlrenderer.h>
#include <imgui/imgui.h>
#include <JR/ErrorOr.h>

struct Imgui {
    ImGuiContext* context;
    SDL_Window* window;
    SDL_Renderer* renderer;
    
    static ErrorOr<Imgui> create(SDL_Window* window, SDL_Renderer* renderer)
    {
        IMGUI_CHECKVERSION();
        auto context = ImGui::CreateContext();
        ImGui_ImplSDL2_InitForSDLRenderer(context, window, renderer);
        ImGui_ImplSDLRenderer_Init(context, renderer);
        return Imgui { context, window, renderer };
    }

    void destroy() const
    {
        if (is_valid()) {
            ImGui_ImplSDL2_Shutdown(context);
            ImGui_ImplSDLRenderer_Shutdown(context);
            ImGui::DestroyContext(context);
            mutable_self().invalidate();
        }
    }

    constexpr bool is_valid() const { return context && window && renderer; }

    void begin_frame();
    void end_frame();

    void process_event(SDL_Event* event) const;

private:

    Imgui& mutable_self() const
    {
        return *const_cast<Imgui*>(this);
    }

    void invalidate()
    {
        context = nullptr;
        window = nullptr;
        renderer = nullptr;
    }
};
