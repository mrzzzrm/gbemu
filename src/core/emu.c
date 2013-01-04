#include "emu.h"
#include "cpu/defines.h"
#include "io/lcd.h"
#include "debug.h"
#include "loader.h"



static void emu_reset() {
    mem_reset();
    cpu_reset();
    lcd_reset();
}

static void emu_step() {
    cpu_step();
    lcd_step();
}

void emu_init() {

}

void emu_close() {

}

bool emu_load(u8 *data, size_t size) {
    if(!load_rom(data, size)) {
        err_set(ERR_ROM_CORRUPT);
        return false;
    }
    emu_reset();

    return true;
}

bool emu_run() {
    debug_init();
    printf("Starting emulation\n");

    for(;;) {
        debug_console();
        if(dbg.verbose) {
            debug_print_cpu_state();
            if(dbg.verbose >= DBG_VLVL_NORMAL) fprintf(stderr, "{\n");
        }

        debug_before();
        emu_step();
        debug_after();

        if(dbg.verbose) {
            if(dbg.verbose >= DBG_VLVL_NORMAL) debug_print_diff();
            if(dbg.verbose >= DBG_VLVL_NORMAL) fprintf(stderr, "}\n");
        }
    }
    return true;
}

