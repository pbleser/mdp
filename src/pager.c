/*
 * Copyright (c) 2012 Bertrand Janin <b@janin.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *
 * The "pager" ensures password to not linger on screen. Our pager
 * is written using curses to make it fullscreen.
 */

#include <sys/ioctl.h>
#include <sys/param.h>

#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <wchar.h>
#include <err.h>
#include <signal.h>
#include <string.h>
#include <curses.h>

#include "array.h"
#include "mdp.h"
#include "curses.h"
#include "pager.h"
#include "config.h"
#include "results.h"
#include "keywords.h"


extern int	 window_width;
extern int	 window_height;
extern WINDOW	*screen;
extern struct wlist results;


/*
 * Display all the results.
 *
 * This function assumes curses is initialized.
 */
void
refresh_listing()
{
	int top_offset, left_offset, i;
	int len = results_visible_length();
	struct result *result;
	char line[MAX_LINE_SIZE];
	char refine_msg[] = "Too many results, please refine "
			    "your search.";

	if (len >= window_height || len >= RESULTS_MAX_LEN) {
		wmove(screen, window_height / 2,
				(window_width - strlen(refine_msg))/ 2);
		waddstr(screen, refine_msg);
		refresh();
		return;
	}

	top_offset = (window_height - len) / 2;
	left_offset = (window_width - get_widest_result()) / 2;

	/* Place the lines on screen. */
	for (i = 0; i < ARRAY_LENGTH(&results); i++) {
		result = ARRAY_ITEM(&results, i);

		if (result->status != RESULT_SHOW)
			continue;

		wcstombs(line, result->value, sizeof(line));
		wmove(screen, top_offset, left_offset);
		waddstr(screen, line);
		top_offset++;
	}

	refresh();
}


void
keyword_prompt(void)
{
	char kw[50] = "";

	curs_set(1);

	wmove(screen, window_height - 1, 0);
	waddstr(screen, "Keywords: ");

	refresh();
	echo();
	getnstr(kw, 50);

	keywords_load_from_char(kw);
}


/*
 * Take a finite amount of results and show them full-screen.
 *
 * If the number of results is greater than the available lines on screen,
 * display a prompt to refine the keywords.
 */
int
pager(enum pager_start_mode mode)
{
	init_curses();

	for (;;) {
		clear();

		if (mode == START_WITH_PROMPT) {
			mode = START_WITHOUT_PROMPT;
			keyword_prompt();
			filter_results();
			continue;
		}

		refresh_listing();

		/* Wait for any keystroke, a slash or a timeout. */
		if (getch() == '/') {
			keyword_prompt();
			filter_results();
			continue;
		}

		break;
	}

	shutdown_curses();

	return MODE_EXIT;
}

