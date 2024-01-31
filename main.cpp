#include <SDL.h>
#include <SDL_video.h>
#include <iostream>

int main() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Create a window
    SDL_Window* window = SDL_CreateWindow("Video Player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Create a renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Load a video
    SDL_Surface* videoSurface = SDL_LoadBMP("path/to/your/video.bmp");
    if (!videoSurface) {
        std::cerr << "Video loading failed: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Texture* videoTexture = SDL_CreateTextureFromSurface(renderer, videoSurface);
    SDL_FreeSurface(videoSurface);

    if (!videoTexture) {
        std::cerr << "Texture creation failed: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Main loop
    bool quit = false;
    SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }

        // Clear the renderer
        SDL_RenderClear(renderer);

        // Render the video texture
        SDL_RenderCopy(renderer, videoTexture, NULL, NULL);

        // Update the renderer
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    SDL_DestroyTexture(videoTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
