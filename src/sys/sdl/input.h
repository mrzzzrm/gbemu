#ifndef SYS_INPUT_H
#define SYS_INPUT_H

typedef struct {
    struct {
        int up, down, left, right;
        int a, b, start, select;

        int save, load;
        int menu;
    } keys;
} input_t;

extern input_t input;

void input_init();
void input_event(int type, int key);

#endif // SYS_INPUT_H