#include <stdarg.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#include <ncurses.h>

#include "gen.h"

void handler(int sig);

char win_resize = 0;
WINDOW *w_stats = NULL, *w_line1 = NULL, *w_slow = NULL, *w_line2 = NULL, *w_fast = NULL;
unsigned int max_x = 80, max_y = 25;

double *history = NULL;
unsigned int history_n = 0;

typedef enum { C_WHITE = 0, C_GREEN, C_YELLOW, C_BLUE, C_MAGENTA, C_CYAN, C_RED } color_t;

void determine_terminal_size(unsigned int *max_y, unsigned int *max_x)
{
        struct winsize size;

        *max_x = *max_y = 0;

        if (ioctl(1, TIOCGWINSZ, &size) == 0)
        {
                *max_y = size.ws_row;
                *max_x = size.ws_col;
        }

        if (!*max_x || !*max_y)
        {
                char *dummy = getenv("COLUMNS");
                if (dummy)
                        *max_x = atoi(dummy);
                else
                        *max_x = 80;

                dummy = getenv("LINES");
                if (dummy)
                        *max_x = atoi(dummy);
                else
                        *max_x = 24;
        }
}

void update_terminal(void)
{
        wnoutrefresh(w_stats);
        wnoutrefresh(w_slow);
        wnoutrefresh(w_fast);

        doupdate();
}

void create_windows(void)
{
	unsigned int nr = 0, logs_n = 0, slow_n = 0, fast_n = 0;
	double scale = 0.0;

	if (w_stats)
	{
		delwin(w_stats);
		delwin(w_line1);
		delwin(w_slow);
		delwin(w_line2);
		delwin(w_fast);
	}

	if (max_x > history_n)
	{
		history = (double *)realloc(history, sizeof(double) * max_x);

		memset(&history[history_n], 0x00, (max_x - history_n) * sizeof(double));

		history_n = max_x;
	}

	w_stats = newwin(6, max_x,  0, 0);
	scrollok(w_stats, false);

	w_line1 = newwin(1, max_x,  6, 0);
	scrollok(w_line1, false);
	wnoutrefresh(w_line1);

	logs_n = max_y - 8;
	scale = (double)logs_n / 16.0;
	slow_n = (int)(scale * 6);
	fast_n = logs_n - slow_n;

	w_slow  = newwin(slow_n, max_x, 7, 0);
	scrollok(w_slow, true);

	w_line2 = newwin(1, max_x, 7 + slow_n, 0);
	scrollok(w_line2, false);
	wnoutrefresh(w_line2);

	w_fast  = newwin(fast_n, max_x, 7 + slow_n + 1, 0);
	scrollok(w_fast, true);

	wattron(w_line1, A_REVERSE);
	wattron(w_line2, A_REVERSE);
	for(nr=0; nr<max_x; nr++)
	{
		wprintw(w_line1, " ");
		wprintw(w_line2, " ");
	}
	wattroff(w_line2, A_REVERSE);
	wattroff(w_line1, A_REVERSE);

        wnoutrefresh(w_line1);
        wnoutrefresh(w_line2);

	doupdate();

	signal(SIGWINCH, handler);
}

void recreate_terminal(void)
{
        determine_terminal_size(&max_y, &max_x);

        resizeterm(max_y, max_x);

        endwin();
        refresh(); /* <- as specified by ncurses faq, was: doupdate(); */

        create_windows();
}

void init_ncurses(void)
{
        initscr();
        start_color();
        keypad(stdscr, TRUE);
        intrflush(stdscr, FALSE);
        noecho();
        //nonl();
        refresh();
        nodelay(stdscr, FALSE);
        meta(stdscr, TRUE);     /* enable 8-bit input */
        idlok(stdscr, TRUE);    /* may give a little clunky screenredraw */
        idcok(stdscr, TRUE);    /* may give a little clunky screenredraw */
        leaveok(stdscr, FALSE);

        init_pair(C_WHITE, COLOR_WHITE, COLOR_BLACK);
        init_pair(C_CYAN, COLOR_CYAN, COLOR_BLACK);
        init_pair(C_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(C_BLUE, COLOR_BLUE, COLOR_BLACK);
        init_pair(C_YELLOW, COLOR_YELLOW, COLOR_BLACK);
        init_pair(C_GREEN, COLOR_GREEN, COLOR_BLACK);
        init_pair(C_RED, COLOR_RED, COLOR_BLACK);

        recreate_terminal();
}

void end_ncurses(void)
{
	if (w_stats)
	{
		delwin(w_stats);
		delwin(w_line1);
		delwin(w_slow);
		delwin(w_line2);
		delwin(w_fast);
	}

	endwin();
}

void fast_log(const char *fmt, ...)
{
        va_list ap;

        va_start(ap, fmt);
        vwprintw(w_fast, fmt, ap);
        va_end(ap);
}

void slow_log(const char *fmt, ...)
{
        va_list ap;

        va_start(ap, fmt);
        vwprintw(w_slow, fmt, ap);
        va_end(ap);
}

void my_beep(void)
{
	beep();
}

void status_line(char *fmt, ...)
{
        va_list ap;

	wattron(w_line1, A_REVERSE);

	wmove(w_line1, 0, 0);

        va_start(ap, fmt);
        vwprintw(w_line1, fmt, ap);
        va_end(ap);

	wattroff(w_line1, A_REVERSE);

	wnoutrefresh(w_line1);
}

void update_stats(stats_t *connect, stats_t *request, stats_t *total, int n_ok, int n_fail, const char *last_connect_str, const char *fp)
{
	werase(w_stats);

	if (n_ok)
	{
		mvwprintw(w_stats, 0, 0, "connect: %6.2f/%6.2f/%6.2f/%6.2f/%6.2f (cur/min/avg/max/sd)",
			connect -> cur, connect -> min, connect -> avg / (double)connect -> n, connect -> max, calc_sd(connect));
		mvwprintw(w_stats, 1, 0, "request: %6.2f/%6.2f/%6.2f/%6.2f/%6.2f",
			request -> cur, request -> min, request -> avg / (double)request -> n, request -> max, calc_sd(request));
		mvwprintw(w_stats, 2, 0, "total  : %6.2f/%6.2f/%6.2f/%6.2f/%6.2f",
			total   -> cur, total   -> min, total   -> avg / (double)total   -> n, total   -> max, calc_sd(total  ));

		mvwprintw(w_stats, 3, 0, "ok: %4d, fail: %4d", n_ok, n_fail);

		mvwprintw(w_stats, 4, 0, "http result code: %s, SSL fingerprint: %s", last_connect_str, fp ? fp : "n/a");
	}

	memmove(&history[1], &history[0], (history_n - 1) * sizeof(double));
	history[0]= total -> cur;

	// FIXME draw history graph

	wnoutrefresh(w_stats);
}
