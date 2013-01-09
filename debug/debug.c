#include "debug.h"
#include "cpu.h"
#include "io/lcd.h"
#include "cpu/defines.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>

dbg_t dbg;

static cpu_t cpu_before, cpu_after;
static ram_t ram_before, ram_after;

#define RUN_FOREVER 0
#define RUN_TRACE 1
#define RUN_UNTIL_CURSOR_EQ 2
#define RUN_UNTIL_CURSOR_GE 3
#define RUN_UNTIL_CURSOR_SE 4
#define RUN_UNTIL_REGISTER 5
#define RUN_UNTIL_MEMORY 6
#define RUN_UNTIL_IO 7
#define RUN_UNTIL_SYMBOL 8
#define RUN_UNTIL_JUMP 9

#define MONITOR_NONE 0x01
#define MONITOR_REGISTER 0x02
#define MONITOR_MEMORY 0x04
#define MONITOR_IO 0x08
#define MONITOR_SYMBOL 0x10
#define MONITOR_INTERRUPT 0x10
#define MONITOR_OP 0x10

#define SYMBOL_DMA 0x01
#define SYMBOL_IRQ 0x02
#define SYMBOL_IEXEC 0x04
#define SYMBOL_MEMORY 0x08
#define SYMBOL_IO 0x10
#define SYMBOL_OP 0x20


typedef struct {
    cpu_t cpu;
} trace_node_t;

typedef struct {
    int size;
    trace_node_t **nodes;
} trace_t;

static trace_t trace = {0, NULL};


static void trace_update() {
    trace.nodes = realloc(trace.nodes, (++trace.size) * sizeof(*trace.nodes));
    trace.nodes[trace.size-1] = malloc(sizeof(**trace.nodes));
    trace.nodes[trace.size-1]->cpu = cpu;
}

static void dump_trace(int lvl) {
    unsigned int n;
    FILE *f;

    f = fopen("trace.txt", "w");
    for(n = 0; n < trace.size; n++) {
        fprintf(f, "PC=%.4X\n", trace.nodes[n]->cpu.pc.w);
    }
    fclose(f);
}

static void print_bits8(u8 v) {
    int b;
    for(b = 7; b >= 0; b--) {
        fprintf(stderr, "%i", (1<<b)&v?1:0);
    }
}

static void cpu_print_diff_b(u8 b, u8 a, const char *name) {
    if(b != a)
        fprintf(stderr, "    %s: %.2X=>%.2X\n", name, b, a);
}

static void cpu_print_diff_w(u16 b, u16 a, const char *name) {
    if(b != a)
        fprintf(stderr, "    %s: %.4X=>%.4X\n", name, b, a);
}

static void cpu_print_diff_flag(u8 b, u8 a, const char *name) {
    if(b != a) {
        fprintf(stderr, "    %s: ", name);
        print_bits8(b);fprintf(stderr, "=>");print_bits8(a);fprintf(stderr, "\n");
    }
}

static void dump_fb(FILE *f, u8 *fb) {
    unsigned int x, y;
    for(y = 0; y < 144; y++) {
        for(x = 0; x < 160; x++) {
            fprintf(f, "%i", fb[y*160 + x]);
        }
        fprintf(f, "\n");
    }
}


static void dump_fbs() {
    FILE *f = fopen("fbs.txt", "w");
    assert(f != NULL);

    fprintf(f, "Clean-FB:\n"); dump_fb(f, lcd.clean_fb);
    fprintf(f, "Working-FB:\n"); dump_fb(f, lcd.working_fb);

    fclose(f);
}

static void dump_oam(FILE *f) {
    unsigned int s, ss;
    for(s = 0, ss = 0; s < 40; s++, ss+=4) {
        fprintf(f, "  Sprite %i Y=%i X=%i Tile=%.2X Attributes=%.2X\n", s, (int)ram.oam[ss], (int)ram.oam[ss+1], (int)ram.oam[ss+2], (int)ram.oam[ss+3]);
    }
}

static void dump_vram(FILE *f) {
    unsigned int x, y;
    for(y = 0; y < 256; y++) {
        fprintf(f, "  ");
        for(x = 0; x < 32; x++) {
            fprintf(f, "%X ", ram.vbanks[0][y*32+x]);
        }
        fprintf(f, "\n");
    }
}

static void dump_video() {
    FILE *f = fopen("video.txt", "w");
    assert(f != NULL);

    fprintf(f, "OAM:\n"); dump_oam(f);
    fprintf(f, "VRAM:\n"); dump_vram(f);

    fclose(f);
}

void debug_init() {
    dbg.verbose = DBG_VLVL_NORMAL;
    dbg.mode = DBG_TRACE;
    dbg.cursor = 0;
    dbg.state_lvl = 0;
}

void debug_console() {
    char str[256];

    trace_update();

    if(dbg.mode == DBG_CURSOR_EQ || dbg.mode == DBG_CURSOR_GE) {
        if(dbg.mode == DBG_CURSOR_EQ && cpu.pc.w == dbg.cursor) {
            dbg.mode = DBG_TRACE;
        }
        else if(dbg.mode == DBG_CURSOR_GE && cpu.pc.w >= dbg.cursor) {
            dbg.mode = DBG_TRACE;
        }
        else {
            return;
        }
    }

    for(;;) {
        fprintf(stderr,"PC=%.4X: ", cpu.pc.w);
        assert(gets(str) != NULL);
        fflush(stdin);

        if(!isalnum(*str))
            break;

        if(str[0] == 'c') {
            if(str[1] == '=')
                dbg.mode = DBG_CURSOR_EQ;
            else
                dbg.mode = DBG_CURSOR_GE;
            dbg.cursor = strtol(&str[2], NULL, 16);
            break;
        }
        if(str[0] == 'v') {
            dbg.verbose = strtol(&str[2], NULL, 10);
            continue;
        }
        if(str[0] == 's') {
            if(str[1] == '=') {
                dbg.state_lvl = strtol(&str[2], NULL, 10);
            }
            else {
                debug_print_cpu_state();
            }
            continue;
        }
        if(str[0] == 'r') {
            u16 adr = strtol(&str[2], NULL, 16);
            fprintf(stderr, "%.2X\n", mem_readb(adr));
            continue;
        }
        if(str[0] == 'd') {
            switch(str[1]) {
                case 'v': dump_video(); break;
                case 't':
                    if(str[2] == '=')
                        dump_trace(strtol(&str[3], NULL, 10));
                    else
                        dump_trace(0);
                break;
                case'f':
                    dump_fbs();
                break;
            }

            continue;
        }
    }
}


void debug_print_cpu_state() {
    fprintf(stderr, "[");
    fprintf(stderr, "PC:%.4X AF:%.2X%.2X BC:%.2X%.2X DE:%.2X%.2X HL:%.2X%.2X SP:%.4X F:", (int)PC, (int)A, (int)F, (int)B, (int)C, (int)D, (int)E, (int)H, (int)L, (int)SP);
    int b;
    for(b = 7; b >= 4; b--) {
        fprintf(stderr, "%i", (1<<b)&F?1:0);
    }
    if(dbg.state_lvl > 0)
        fprintf(stderr, " \n LC:%.2X LS:%.2X LY:%.2X CC: %.8X]\n", lcd.c, lcd.stat, lcd.ly, cpu.cc);
    else
        fprintf(stderr, "]\n");
}

static void debug_cpu_before() {
    cpu_before = cpu;
}

static void debug_cpu_after() {
    cpu_after = cpu;
}

static void debug_cpu_print_diff() {
    if(cpu_after.pc.w - 1 == cpu_before.pc.w)
        cpu_before.pc = cpu_after.pc;

    cpu_before.cc = cpu_after.cc;
    if(memcmp(&cpu_before, &cpu_after, sizeof(cpu_t)) == 0)
        return;

    fprintf(stderr, "  CPU-Diff \n");
    cpu_print_diff_b(cpu_before.af.b[1], cpu_after.af.b[1], "A");
    cpu_print_diff_flag(cpu_before.af.b[0], cpu_after.af.b[0], "F");
    cpu_print_diff_b(cpu_before.bc.b[1], cpu_after.bc.b[1], "B");
    cpu_print_diff_b(cpu_before.bc.b[0], cpu_after.bc.b[0], "C");
    cpu_print_diff_b(cpu_before.de.b[1], cpu_after.de.b[1], "D");
    cpu_print_diff_b(cpu_before.de.b[0], cpu_after.de.b[0], "E");
    cpu_print_diff_b(cpu_before.hl.b[1], cpu_after.hl.b[1], "H");
    cpu_print_diff_b(cpu_before.hl.b[0], cpu_after.hl.b[0], "L");
    cpu_print_diff_w(cpu_before.pc.w, cpu_after.pc.w, "PC");
}

static void debug_ram_before() {
    ram_before = ram;
}

static void debug_ram_after() {
    ram_after = ram;
}


static void ram_print_i_diff() {
    unsigned int bank, byte;

    for(bank = 0; bank < 8; bank++) {
        for(byte = 0; byte < 0x1000; byte++) {
            if(ram_before.ibanks[bank][byte] != ram_after.ibanks[bank][byte]) {
                fprintf(stderr, "    IRAM[%i][%.4X] %.2X=>%.2X\n", bank, byte, ram_before.ibanks[bank][byte], ram_after.ibanks[bank][byte]);
            }
        }
    }
}

static void ram_print_h_diff() {
    unsigned int byte;

    for(byte = 0; byte < 0x80; byte++) {
        if(ram_before.hram[byte] != ram_after.hram[byte]) {
            fprintf(stderr, "    HRAM[%.2X] %.2X=>%.2X\n", byte, ram_before.hram[byte], ram_after.hram[byte]);
        }
    }
}

static void ram_print_v_diff() {
    unsigned int bank, byte;

    for(bank = 0; bank < 2; bank++) {
        for(byte = 0; byte < 0x2000; byte++) {
            if(ram_before.vbanks[bank][byte] != ram_after.vbanks[bank][byte]) {
                fprintf(stderr, "    VRAM[%i][%.4X] %.2X=>%.2X\n", bank, byte, ram_before.vbanks[bank][byte], ram_after.vbanks[bank][byte]);
            }
        }
    }
}

static void ram_print_oam_diff() {
    unsigned int byte;

    for(byte = 0; byte < 0xA0; byte++) {
        if(ram_before.oam[byte] != ram_after.oam[byte]) {
            fprintf(stderr, "    OAM[%.2X] %.2X=>%.2X\n", byte, ram_before.oam[byte], ram_after.oam[byte]);
        }
    }
}

static void debug_ram_print_diff() {
    if(memcmp(&ram_before, &ram_after, sizeof(ram_t)) == 0)
        return;

    fprintf(stderr, "  RAM-Diff \n");
    ram_print_i_diff();
    ram_print_h_diff();
    ram_print_v_diff();
    ram_print_oam_diff();
}

void debug_before() {
    debug_cpu_before();
    debug_ram_before();
}

void debug_after() {
    debug_cpu_after();
    debug_ram_after();
}

void debug_print_diff() {
    debug_cpu_print_diff();
    debug_ram_print_diff();
}

