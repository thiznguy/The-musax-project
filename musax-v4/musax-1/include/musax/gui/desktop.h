#ifndef MUSAX_GUI_DESKTOP_H
#define MUSAX_GUI_DESKTOP_H

#include <stddef.h>

#define DESKTOP_OK 0
#define DESKTOP_ERR_NO_FONT (-1)
#define DESKTOP_ERR_NO_DISPLAY (-2)
#define DESKTOP_ERR_NO_MEMORY (-3)

#define DESKTOP_WIDTH 1024
#define DESKTOP_HEIGHT 768
#define DESKTOP_TOP_BAR_HEIGHT 28
#define DESKTOP_THEME_KEY "desktop.background"

/* Runs the desktop until the user exits it, then restores the text console. Returns DESKTOP_OK or a negative error. */
int desktop_run(void);
const char *desktop_error_message(int error);

/* For applications: ask for the screen to be redrawn, and read or change the background theme. */
void desktop_request_redraw(void);
size_t desktop_theme(void);
void desktop_set_theme(size_t index);

#endif
