#ifndef MENU_UTIL_H
#define MENU_UTIL_H

#include <SDL/SDL.h>

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#define KEY_ACCEPT SDLK_RETURN
#define KEY_BACK SDLK_BACKSPACE

typedef struct {
    SDL_Surface *surfaces[2];
} menu_label_t;

typedef struct menu_list_entry_s {
    menu_label_t *text;
    menu_label_t *val;
    int id;
    int is_visible;
    void (*accept_func)(void);
} menu_listentry_t;

typedef struct {
    menu_listentry_t **entries;
    int num_entries;
    menu_label_t *title;
    int selected;
    int num_visible;
    int first_visible;

    int scroll_state[2];
    time_t last_scroll[2];
} menu_list_t;

void menu_util_init();
void menu_util_close();
menu_label_t *menu_label(const char *text);
void menu_free_label(menu_label_t *label);
void menu_blit(SDL_Surface *s, int x, int y);

menu_list_t *menu_new_list(const char *title);
void menu_list_update(menu_list_t *list);
void menu_free_list(menu_list_t *list);
void menu_new_listentry(menu_list_t *list, const char *text, int id, void (*accept_func)(void));
void menu_listentry_val(menu_list_t *list, int key, const char *val);
void menu_listentry_val_int(menu_list_t *list, int key, int ival);
int menu_listentry_index(menu_list_t *list, int key);
int menu_list_selected_id(menu_list_t *list);
void menu_list_select_first(menu_list_t *list);
void menu_listentry_visible(menu_list_t *list, int id, int visible);
void menu_draw_list(menu_list_t *list);
void menu_list_input(menu_list_t *list, int type, int key);

#endif