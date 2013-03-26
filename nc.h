#include <ncurses.h>

extern char win_resize;

void init_ncurses(void);
void end_ncurses(void);
void fast_log(const char *fmt, ...);
void slow_log(const char *fmt, ...);
void my_beep(void);
void update_terminal(void);
void status_line(char *fmt, ...);
