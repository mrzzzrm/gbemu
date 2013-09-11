#include "sys/sys.h"
#include "video.h"
#include <stdarg.h>
#include <SDL/SDL.h>
#include <assert.h>
#include "core/cpu.h"
#include "core/rtc.h"
#include "core/mbc.h"
#include "core/lcd.h"
#include "core/moo.h"
#include "core/joy.h"
#include "core/sound.h"
#include "util/state.h"
#include "input.h"
#include "audio.h"
#include "util/framerate.h"
#include "util/performance.h"

#define SCALING_STRECHED 0
#define SCALING_PROPORTIONAL 1
#define SCALING_PROPORTIONAL_FULL 2
#define SCALING_NONE 3

#define min(a, b) ((a) < (b) ? (a) : (b))

sys_t sys;

void sys_init(int argc, const char** argv) {
    memset(&sys, 0x00, sizeof(sys));

    sys.sound_on = 1;
    sys.sound_freq = 22050;
    sys.quantum_length = 1000;
    sys.bits_per_pixel = 15;
    moo.state = MOO_RUNNING_BIT;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

#ifdef PANDORA
    SDL_SetVideoMode(800, 480, sys.bits_per_pixel, SDL_FULLSCREEN);
    SDL_ShowCursor(0);
#else
    //SDL_SetVideoMode(1680, 1050, sys.bits_per_pixel, SDL_FULLSCREEN);
    SDL_SetVideoMode(680, 300, sys.bits_per_pixel, 0);
#endif

    sys.scalingmode = 0;
    sys.num_scalingmodes = 4;
    sys.scalingmode_names = malloc(sizeof(*sys.scalingmode_names) * sys.num_scalingmodes);
    sys.scalingmode_names[SCALING_STRECHED] = strdup("Streched");
    sys.scalingmode_names[SCALING_PROPORTIONAL] = strdup("Proportional");
    sys.scalingmode_names[SCALING_PROPORTIONAL_FULL] = strdup("Full Proportional");
    sys.scalingmode_names[SCALING_NONE] = strdup("None");

    audio_init();
    video_init();
    framerate_init();
    performance_init();
    input_init();
}

void sys_reset() {
    sys.ticks = 0;
    sys.invoke_cc = 0;
    framerate_begin();
    performance_begin();
}

void sys_close() {
    int s;
    for(s = 0; s < sys.num_scalingmodes; s++) {
        free(sys.scalingmode_names[s]);
    }
    free(sys.scalingmode_names);

    SDL_Quit();
}

void sys_pause() {
    sys.pause_start = SDL_GetTicks();
}

void sys_begin() {
    sys.ticks_diff = sys.ticks - (long long)SDL_GetTicks();
    sys_play_audio(sys.sound_on);
}

void sys_continue() {
    sys.ticks_diff -= (long long)SDL_GetTicks() - sys.pause_start;
    sys_play_audio(sys.sound_on);
}

SDL_Rect none_scaling_area() {
    SDL_Rect area;
    area.x = SDL_GetVideoSurface()->w/2 - 80;
    area.y = SDL_GetVideoSurface()->h/2 - 72;
    area.w = 160;
    area.h = 144;
    return area;
}

SDL_Rect proportional_scaling_area() {
    SDL_Rect area;
    int fw, fh, f;
    fw = SDL_GetVideoSurface()->w / 160;
    fh = SDL_GetVideoSurface()->h / 144;
    f = min(fw, fh);
    area.w = f * 160;
    area.h = f * 144;
    area.x = SDL_GetVideoSurface()->w/2 - area.w/2;
    area.y = SDL_GetVideoSurface()->h/2 - area.h/2;
    return area;
}

SDL_Rect proportional_full_scaling_area() {
    SDL_Rect area;
    float fw, fh, f;
    fw = SDL_GetVideoSurface()->w / 160.0f;
    fh = SDL_GetVideoSurface()->h / 144.0f;
    f = min(fw, fh);
    area.w = f * 160;
    area.h = f * 144;
    area.x = SDL_GetVideoSurface()->w/2 - area.w/2;
    area.y = SDL_GetVideoSurface()->h/2 - area.h/2;
    return area;
}

SDL_Rect streched_scaling_area() {
    SDL_Rect area;
    area.x = 0;
    area.y = 0;
    area.w = SDL_GetVideoSurface()->w;
    area.h = SDL_GetVideoSurface()->h;
    return area;
}

static void render() {
    SDL_Rect area;

    SDL_FillRect(SDL_GetVideoSurface(), NULL, 0);

    switch(sys.scalingmode) {
        case SCALING_NONE: area = none_scaling_area(); break;
        case SCALING_PROPORTIONAL: area = proportional_scaling_area(); break;
        case SCALING_PROPORTIONAL_FULL: area = proportional_full_scaling_area(); break;
        case SCALING_STRECHED: area = streched_scaling_area(); break;
        default: moo_errorf("No valid scalingmode selected"); return;
    }

    video_render(SDL_GetVideoSurface(), area);
    SDL_BlitSurface(performance.statuslabel, NULL, SDL_GetVideoSurface(), NULL);
    SDL_Flip(SDL_GetVideoSurface());
}

void sys_delay(int ticks) {
    SDL_Delay(ticks);
}

void sys_handle_events(void (*input_handle)(int, int)) {
   SDL_Event event;

    while(SDL_PollEvent(&event)) {
        if(event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
            input_handle(event.type, event.key.keysym.sym);
        }
        if(event.type == SDL_QUIT) {
            moo_quit();
        }
    }
}

void sys_invoke() {
    sys.ticks = SDL_GetTicks() + sys.ticks_diff;
    if(sys.fb_ready) {
        if(!framerate_skip()) {
            render();
        }

        sys.fb_ready = 0;
        performance.frames++;
    }
    framerate_curb();

    sys_handle_events(input_event);
    performance_invoked();
    //sys_serial_step();
}

void sys_fb_ready() {
    sys.fb_ready = 1;
}

void sys_play_audio(int on) {
    SDL_PauseAudio(!on);
}

