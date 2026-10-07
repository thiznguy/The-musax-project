#ifndef MUSAX_SHELL_MUSIC_H
#define MUSAX_SHELL_MUSIC_H

/* Runs the shell on the text console; never returns. */
__attribute__((noreturn)) void music_run(void);

/* The same shell driven one key at a time, for hosts such as the desktop terminal: start prints the first prompt,
   key handles one key code from musax/keys.h and runs the command when Enter arrives. */
void music_session_start(void);
void music_session_key(int key);

#endif
