#include <Imgui.h>
#include <SDL2/SDL.h>

void Imgui::begin_frame()
{
    ImGui_ImplSDLRenderer_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void Imgui::end_frame()
{
    ImGui::Render();
    SDL_SetRenderDrawColor(renderer, 123, 32, 42, 255);
    SDL_RenderClear(renderer);
    auto draw_data = ImGui::GetDrawData();
    ImGui_ImplSDLRenderer_RenderDrawData(draw_data);
    SDL_RenderPresent(renderer);
}

void Imgui::process_event(SDL_Event* event) const
{
    ImGui_ImplSDL2_ProcessEvent(event);
}

