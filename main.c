#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#include "cip8.h"

#define PRO_SIZE 7


#define RENDER_SDL 1
#define RENDER_TERMINAL 0
#define FPS 60.f


bool limit_fps(int fps,Uint32 end,double* dt) {
    Uint32 start = SDL_GetTicks();
    *dt = start - end; 
    // *dt /= 1000;
    return *dt <= (1/(double)fps);
}


void renderer_sdl(Cip8* cip) {
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
    Uint32 end = SDL_GetTicks();
    double dt = 0;  
    while (!done) {

       while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) {
                done = true;
            }
            if(event.type == SDL_KEYDOWN) {
                if(event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    done = true;
                }

                switch (event.key.keysym.scancode) {
                    case SDL_SCANCODE_KP_0:cip->keyboard[0] = 1;  break;
                    case SDL_SCANCODE_KP_1:cip->keyboard[1] = 1;  break;
                    case SDL_SCANCODE_KP_2:cip->keyboard[2] = 1;  break;
                    case SDL_SCANCODE_KP_3:cip->keyboard[3] = 1;  break;
                    case SDL_SCANCODE_KP_4:cip->keyboard[4] = 1;  break;
                    case SDL_SCANCODE_KP_5:cip->keyboard[5] = 1;  break;
                    case SDL_SCANCODE_KP_6:cip->keyboard[6] = 1;  break;
                    case SDL_SCANCODE_KP_7:cip->keyboard[7] = 1;  break;
                    case SDL_SCANCODE_KP_8:cip->keyboard[8] = 1;  break;
                    case SDL_SCANCODE_KP_9:cip->keyboard[9] = 1;  break;
                    case SDL_SCANCODE_A:cip->keyboard[10] =   1; break;
                    case SDL_SCANCODE_B:cip->keyboard[11] =   1; break;
                    case SDL_SCANCODE_C:cip->keyboard[12] =   1; break;
                    case SDL_SCANCODE_D:cip->keyboard[13] =   1; break;
                    case SDL_SCANCODE_E:cip->keyboard[14] =   1; break;
                    case SDL_SCANCODE_F:cip->keyboard[15] =   1; break;
                    default: break;
                }
            }
            if(event.type == SDL_KEYUP) {
                switch (event.key.keysym.scancode) {
                    case SDL_SCANCODE_KP_0:cip->keyboard[0] = 0;  break;
                    case SDL_SCANCODE_KP_1:cip->keyboard[1] = 0;  break;
                    case SDL_SCANCODE_KP_2:cip->keyboard[2] = 0;  break;
                    case SDL_SCANCODE_KP_3:cip->keyboard[3] = 0;  break;
                    case SDL_SCANCODE_KP_4:cip->keyboard[4] = 0;  break;
                    case SDL_SCANCODE_KP_5:cip->keyboard[5] = 0;  break;
                    case SDL_SCANCODE_KP_6:cip->keyboard[6] = 0;  break;
                    case SDL_SCANCODE_KP_7:cip->keyboard[7] = 0;  break;
                    case SDL_SCANCODE_KP_8:cip->keyboard[8] = 0;  break;
                    case SDL_SCANCODE_KP_9:cip->keyboard[9] = 0;  break;
                    case SDL_SCANCODE_A:cip->keyboard[10] =   0; break;
                    case SDL_SCANCODE_B:cip->keyboard[11] =   0; break;
                    case SDL_SCANCODE_C:cip->keyboard[12] =   0; break;
                    case SDL_SCANCODE_D:cip->keyboard[13] =   0; break;
                    case SDL_SCANCODE_E:cip->keyboard[14] =   0; break;
                    case SDL_SCANCODE_F:cip->keyboard[15] =   0; break;
                    default: break;
                }
            }

        }         
        if(limit_fps(FPS,end,&dt)) {
            continue;
        }
        end = SDL_GetTicks();
        // printf("%f\n",dt);

 

        cip8_step(cip);
        if(cip->delay_timer > 0) {
            cip->delay_timer -= dt;
            cip->delay_timer = SDL_max(cip->delay_timer,0);
        }
        if(cip->halted) {
            done = true;
        }        

        if(cip->display_changed) {
            cip8_sdl_from_mem_to_texture(*cip,display_surface,display_texture);
            SDL_RenderCopyEx(renderer,display_texture,0,&rect,0,0,0);
            cip->display_changed = false;
            SDL_RenderPresent(renderer);
        }

    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}
void renderer_terminal(Cip8* cip ) {
    Uint32 end = SDL_GetTicks();
    double dt = 0;    
    SDL_Init(SDL_INIT_TIMER);
    while (!cip->halted) {
        if(limit_fps(FPS,end,&dt)) {
            continue;
        }
        end = SDL_GetTicks();

        cip8_step(cip);
        cip8_from_mem_to_terminal(*cip);
        if(cip->delay_timer > 0) {
            cip->delay_timer -= 1/60;
            cip->delay_timer = SDL_max(cip->delay_timer,0);
        }
    }
}


int main() {
    int prog_size;
    OpCode* program = cip8_load_from_file("tests/6-keypad.ch8",&prog_size);
    
    // OpCode program[] = {0x15D0, 0x0012}; 
    // prog_size = 2;

    assert(prog_size > 0);

    Cip8 cip;
    cip8_init(&cip);    

    cip8_load_program(&cip,prog_size,program);
    // free(program);

#if RENDER_SDL
    renderer_sdl(&cip);
#elif RENDER_TERMINAL
    renderer_terminal(&cip);
#endif
 
    SDL_Quit();
    return 0;
}