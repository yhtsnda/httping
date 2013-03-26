#include <ncurses.h>

extern char win_resize;

void init_ncurses(void);
void end_ncurses(void);
void fast_log(const char *fmt, ...);
void slow_log(const char *fmt, ...);
