#include <ncurses.h>

extern char win_resize;

void init_ncurses_ui(double graph_limit_in);
void end_ncurses(void);
void fast_log(const char *fmt, ...);
void slow_log(const char *fmt, ...);
void my_beep(void);
void update_terminal(void);
void status_line(char *fmt, ...);
void update_stats(stats_t *resolve, stats_t *connect, stats_t *request, stats_t *total, int n_ok, int n_fail, const char *last_connect_str, const char *fp, char use_tfo);
