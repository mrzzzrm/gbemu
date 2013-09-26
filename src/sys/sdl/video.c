#include <SDL/SDL.h>
#include <assert.h>
#include "core/defines.h"
#include "sys/sys.h"
#include "core/moo.h"
#include "core/lcd.h"


static u8 buf[32768]; // TODO: Dynamic
static u16 cgb_to_rgb_buf[0x8000];
static int acolumns_in_fbpixel_buf[160];
static int line_length;
static int line_byte_offset;
static int bytes_per_pixel;
static int bytes_per_line;
static SDL_Rect area;
static int rshift, gshift, bshift;

static u16 dmg_palette[4];

sys_t sys;


static inline u16 cgb_to_rgb(u16 cgb_color) {
    u16 r, g, b;

    r = (cgb_color >> 0) & 0x1F;
    g = (cgb_color >> 5) & 0x1F;
    b = (cgb_color >> 10) & 0x1F;

    return (r << rshift) | (g << gshift) | (b << bshift);
}

static inline u16 dmg_to_rgb(u16 dmg_color) {
    return dmg_palette[dmg_color];
}

static inline int alines_in_fbline(int aline, int aheight, int fbline) {
    return (((fbline + 1) * aheight) / 144) - aline;
}

static inline int acolumns_in_fbpixel(int acolumn, int awidth, int fbx) {
    return (((fbx + 1) * awidth) / 160) - acolumn;
}

static inline void fw_render_buffer(SDL_Surface *surface, void *buf, int buflines, int y) {
    memcpy(surface->pixels + bytes_per_line * y, buf, buflines * bytes_per_line);
}

static void cgb_fw_render_fbline(int line) {
    int fb_pixel, buf_pos, ax, ppos, fbx;
    u16 s_color;
    int pixels_to_set;

    buf_pos = line_byte_offset;
    fb_pixel = line * 160;
    fbx = 0;

    for(ax = 0; ax < area.w; fbx++) {
        s_color = cgb_to_rgb_buf[lcd.clean_fb[fb_pixel++]];
        pixels_to_set = acolumns_in_fbpixel_buf[fbx];
        ax += pixels_to_set;

        for(ppos = 0; ppos < pixels_to_set; ppos++) {
            buf[buf_pos++] = s_color & 0x00FF;
            buf[buf_pos++] = s_color >> 8;
        }
    }
}

static void dmg_fw_render_fbline(int line) {
    int fb_pixel, buf_pos, ax, ppos, fbx;
    u16 s_color;
    int pixels_to_set;

    buf_pos = line_byte_offset;
    fb_pixel = line * 160;
    fbx = 0;

    for(ax = 0; ax < area.w; fbx++) {
        s_color = dmg_to_rgb(lcd.clean_fb[fb_pixel++]);
        pixels_to_set = acolumns_in_fbpixel(ax, area.w, fbx);

        for(ppos = 0; ppos < pixels_to_set; ppos++, ax++) {
            buf[buf_pos++] = s_color & 0x00FF;
            buf[buf_pos++] = s_color >> 8;
        }
    }
}

static void render_fbline_to_buffer(int line) {
    if(moo.mode == CGB_MODE) {
        cgb_fw_render_fbline(line);
    }
    else {
        dmg_fw_render_fbline(line);
    }
}

static void fullwidth_render(SDL_Surface *surface) {
    int aline, buflines, fbline, alines_to_fill;

    buflines = 0;
    fbline = 0;
    line_length = surface->w;
    line_byte_offset = 0;
    bytes_per_line = surface->pitch;
    bytes_per_pixel = surface->format->BytesPerPixel;

    for(aline = 0; aline < area.h; fbline++) {
        alines_to_fill = alines_in_fbline(aline, area.h, fbline);
        if(alines_to_fill > 0) {
            render_fbline_to_buffer(fbline);

            for(buflines = 1; alines_to_fill >= buflines * 2; buflines *= 2) {
                memcpy(&buf[buflines * bytes_per_line], buf, buflines * bytes_per_line);
            }
            fw_render_buffer(surface, buf, buflines, aline);
            aline += buflines;
            if(alines_to_fill > buflines) {
                fw_render_buffer(surface, buf, alines_to_fill - buflines, aline);
            }
            aline += alines_to_fill - buflines;
        }
    }
}


static void area_render(SDL_Surface *surface) {
    int aline, buflines, fbline, alines_to_fill;

    buflines = 0;
    fbline = 0;
    bytes_per_line = surface->pitch;
    bytes_per_pixel = surface->format->BytesPerPixel;
    line_length = area.w;
    line_byte_offset = area.x * bytes_per_pixel;

    for(aline = 0; aline < area.h; fbline++) {
        alines_to_fill = alines_in_fbline(aline, area.h, fbline);
        if(alines_to_fill > 0) {
            render_fbline_to_buffer(fbline);

            for(buflines = 1; alines_to_fill >= buflines * 2; buflines *= 2) {
                memcpy(&buf[buflines * bytes_per_line], buf, buflines * bytes_per_line);
            }
            fw_render_buffer(surface, buf, buflines, aline + area.y);
            aline += buflines;
            if(alines_to_fill > buflines) {
                fw_render_buffer(surface, buf, alines_to_fill - buflines, aline + area.y);
            }
            aline += alines_to_fill - buflines;
        }
    }
}

void video_init() {

}

void video_switch_display_mode() {
    SDL_Surface *screen = SDL_GetVideoSurface();

    rshift = screen->format->Rshift + 3 - screen->format->Rloss;
    gshift = screen->format->Gshift + 3 - screen->format->Gloss;
    bshift = screen->format->Bshift + 3 - screen->format->Bloss;

    dmg_palette[0] = 0x1F<<rshift | 0x1F << gshift | 0x1F << bshift;
    dmg_palette[1] = 0x13<<rshift | 0x13 << gshift | 0x13 << bshift;
    dmg_palette[2] = 0x07<<rshift | 0x07 << gshift | 0x07 << bshift;
    dmg_palette[3] = 0x00<<rshift | 0x00 << gshift | 0x00 << bshift;

    int c;
    for(c = 0; c < 0x8000; c++) {
        cgb_to_rgb_buf[c] = cgb_to_rgb(c);
    }

    memset(buf, 0x00, sizeof(buf));
    SDL_FillRect(SDL_GetVideoSurface(), NULL, 0);
}

void video_set_area(SDL_Rect _area) {
    int fbx, ax;
    area = _area;
    for(ax = 0, fbx = 0; ax < area.w; fbx++) {
        acolumns_in_fbpixel_buf[fbx] = acolumns_in_fbpixel(ax, area.w, fbx);
        ax += acolumns_in_fbpixel_buf[fbx];
    }
}

void video_render(SDL_Surface *surface) {
    if(area.x == 0 && area.w == surface->w) {
        fullwidth_render(surface);
    }
    else {
        area_render(surface);
    }
}

