#include "debug.h"
#include "cpu.h"
#include "io/lcd.h"
#include "cpu/defines.h"

dbg_t dbg;

static cpu_t cpu_before, cpu_after;
static ram_t ram_before, ram_after;

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

void debug_init() {
    dbg.verbose = DBG_VLVL_NORMAL;
    dbg.mode = DBG_TRACE;
    dbg.cursor = 0;
}



void debug_console() {
    char str[256];

    if(dbg.mode == DBG_CURSOR) {
        if(cpu.pc.w == dbg.cursor) {
            dbg.mode = DBG_TRACE;
            dbg.verbose = DBG_VLVL_NORMAL;
            return;
        }
        else {
            return;
        }
    }


    gets(str);
    fflush(stdin);

    if(str[0] == 'c') {
        dbg.mode = DBG_CURSOR;
        dbg.cursor = strtol(&str[2], NULL, 16);
        dbg.verbose = DBG_VLVL_MIN;
    }
    if(str[0] == 'v') {
        dbg.verbose = strtol(&str[2], NULL, 10);
        debug_console();
        return;
    }
}


void debug_print_cpu_state() {
    fprintf(stderr, "[");
    fprintf(stderr, "PC:%.4X AF:%.2X%.2X BC:%.2X%.2X DE:%.2X%.2X HL:%.2X%.2X SP:%.4X F:", (int)PC, (int)A, (int)F, (int)B, (int)C, (int)D, (int)E, (int)H, (int)L, (int)SP);
    int b;
    for(b = 7; b >= 4; b--) {
        fprintf(stderr, "%i", (1<<b)&F?1:0);
    }
    fprintf(stderr, " \n LC:%.2X LS:%.2X LY:%.2X CC: %.8X]\n", lcd.c, lcd.stat, lcd.ly, cpu.cc);
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


void debug_cpu_before() {
    cpu_before = cpu;
}

void debug_cpu_after() {
    cpu_after = cpu;
}

void debug_cpu_print_diff() {
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

void debug_ram_before() {
    ram_before = ram;
}

void debug_ram_after() {
    ram_after = ram;
}


static void ram_print_i_diff() {
    unsigned int bank, byte;

    for(bank = 0; bank < 8; bank++) {
        for(byte = 0; byte < 0x1000; byte++) {
            if(ram_before.ibanks[bank][byte] != ram_after.ibanks[bank][byte]) {
                fprintf(stderr, "    IRAM[%i][%.4X] %.2X=>%.2X", bank, byte, ram_before.ibanks[bank][byte], ram_after.ibanks[bank][byte]);
            }
        }
    }
}

static void ram_print_h_diff() {
    unsigned int byte;

    for(byte = 0; byte < 0x80; byte++) {
        if(ram_before.hram[byte] != ram_after.hram[byte]) {
            fprintf(stderr, "    HRAM[%.2X] %.2X=>%.2X", byte, ram_before.hram[byte], ram_after.hram[byte]);
        }
    }
}

static void ram_print_v_diff() {
    unsigned int bank, byte;

    for(bank = 0; bank < 2; bank++) {
        for(byte = 0; byte < 0x2000; byte++) {
            if(ram_before.vbanks[bank][byte] != ram_after.vbanks[bank][byte]) {
                fprintf(stderr, "    VRAM[%i][%.4X] %.2X=>%.2X", bank, byte, ram_before.vbanks[bank][byte], ram_after.vbanks[bank][byte]);
            }
        }
    }
}

static void ram_print_oam_diff() {
    unsigned int byte;

    for(byte = 0; byte < 0xA0; byte++) {
        if(ram_before.oam[byte] != ram_after.oam[byte]) {
            fprintf(stderr, "    OAM[%.2X] %.2X=>%.2X", byte, ram_before.oam[byte], ram_after.oam[byte]);
        }
    }
}


void debug_ram_print_diff() {
    if(memcmp(&ram_before, &ram_after, sizeof(ram_t)) == 0)
        return;

    fprintf(stderr, "  RAM-Diff \n");
    ram_print_i_diff();
    ram_print_h_diff();
    ram_print_v_diff();
    ram_print_oam_diff();
}

