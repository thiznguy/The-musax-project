#ifndef MUSAX_GUI_APPS_H
#define MUSAX_GUI_APPS_H

#include <musax/gui/wm.h>

/* The terminal window shows the console output of the muSIC shell and passes keys to it. */
void terminal_attach_output(void);
void terminal_detach_output(void);
struct window *terminal_open(void);

struct window *settings_open(void);

#endif
