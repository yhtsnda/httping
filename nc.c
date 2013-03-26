#include <stdarg.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#include <ncurses.h>

void handler(int sig);

char win_resize = 0;
WINDOW *w_stats = NULL, *w_line1 = NULL, *w_slow = NULL, *w_line2 = NULL, *w_fast = NULL;
unsigned int max_x = 80, max_y = 25;

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
        wmove(w_stats, 4, max_x - 1);

        wnoutrefresh(w_stats);
        wnoutrefresh(w_slow);
        wnoutrefresh(w_fast);

        doupdate();
}

void create_windows(void)
{
	if (w_stats)
	{
		delwin(w_stats);
		delwin(w_line1);
		delwin(w_slow);
		delwin(w_line2);
		delwin(w_fast);
	}

	w_stats = newwin(5, 80,  0, 0);
	scrollok(w_stats, false);

	w_line1 = newwin(1, 80,  5, 0);
	scrollok(w_line1, false);
	wnoutrefresh(w_line1);

	w_slow  = newwin(8, 80,  6, 0);
	scrollok(w_slow, true);

	w_line2 = newwin(1, 80, 14, 0);
	scrollok(w_line2, false);
	wnoutrefresh(w_line2);

	w_fast  = newwin(9, 80, 15, 0);
	scrollok(w_fast, true);

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
        nonl();
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
}

void slow_log(const char *fmt, ...)
{
}
