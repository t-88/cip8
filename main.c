#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#include "cip8.h"

#define PRO_SIZE 8
#define UNPACK_HEX(hex) ((hex & 0xFF000000) >> 8 * 3),\
                        ((hex & 0x00FF0000) >> 8 * 2),\
                        ((hex & 0x0000FF00) >> 8 * 1),\
                        ((hex & 0x000000FF) >> 8 * 0)

#define RENDER_SDL 1
#define FPS 2000.f

int main() {


    // load  0
    // draw  0
    // cmp  v0 64

    OpCode program[PRO_SIZE] = {
            0xF029,
            0xD015,
            0x7004,
            0x4040,
            0x7105,
            0x4040,
            0x6000,
            0x1202
    };

    Cip8 cip;


    cip8_init(&cip);    
    cip8_load_program(&cip,PRO_SIZE,program);
    
    Uint32 end ,start;
    end = SDL_GetTicks();
    start = end;
    double delay = 0;

#if RENDER_SDL
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Event event;
    SDL_Renderer* renderer;
    SDL_Window* window;

    window = SDL_CreateWindow("Cip8 Emulator",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,64 * 10,32*10,0);
    renderer = SDL_CreateRenderer(window,0,0);
    
    SDL_Surface* display_surface = SDL_CreateRGBSurface(0,64,32,32,0,0,0,0);
    SDL_Texture* display_texture = SDL_CreateTextureFromSurface(renderer,display_surface);

    bool done = false; 
    SDL_Rect rect = (SDL_Rect){.x = 0,.y = 0, .w = 64 * 10, .h = 32 * 10};
    while (!done) {       
        start = SDL_GetTicks();
        delay = start - end; 
        if(delay <= 1000/FPS) {
            continue;
        }

        // printf("fps: %f\n",1000/delay);
        end = start;


        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) {
                done = true;
            }
            if(event.type == SDL_KEYDOWN) {
                if(event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    done = true;
                }
            }
        } 
        
        
        if(cip.delay_timer > 0) {
            cip.delay_timer -= 1/60;
            cip.delay_timer = SDL_max(cip.delay_timer,0);
        }
        cip8_step(&cip);
        if(cip.halted) {
            done = true;
        }        

        // if(cip.display_changed) {
            cip8_sdl_from_mem_to_texture(cip,display_surface,display_texture);
            SDL_RenderCopyEx(renderer,display_texture,0,&rect,0,0,0);
            cip.display_changed = false;
            SDL_RenderPresent(renderer);
        // }
    }
#else
    SDL_Init(SDL_INIT_TIMER);

    while (!cip.halted) {
        start = SDL_GetTicks();
        delay = start - end; 
        if(delay <= 1000/FPS) {
            continue;
        }

        end = start;

        
        cip8_step(&cip);
        cip8_from_mem_to_terminal(cip);
        if(cip.delay_timer > 0) {
            cip.delay_timer -= 1/60;
            cip.delay_timer = SDL_max(cip.delay_timer,0);
        }

        // getc(stdin);
    }
    
#endif

    cip8_clear_program(&cip,PRO_SIZE);

#if RENDER_SDL
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
#endif
    return 0;
}