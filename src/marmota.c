/* {{{
	MIT LICENSE

	Copyright (c) 2020, Mihail Szabolcs

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the 'Software'), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
}}} */
#include <stdio.h>
#include <marmota.h>

#ifndef MRT_FALLBACK_SHELL
	#define MRT_FALLBACK_SHELL "/bin/sh"
#endif

#ifndef MRT_UNUSED
	#define MRT_UNUSED(x) (void)(x)
#endif

#define MRT_ISSET(x) \
	((x) != NULL && *(x) != '\0')

#define mrt_quit(x) \
	gtk_window_close(GTK_WINDOW((x)->win))

#define _mrt_log(s, ...) \
	fprintf(stderr, s "%c", __VA_ARGS__)

#define mrt_log(...) \
	_mrt_log(__VA_ARGS__, '\n')

#define _mrt_print_gerror(err, s, ...) \
	mrt_log(s ":%c%s", __VA_ARGS__, (err) != NULL ? (err)->message : "<unknown error>")

#define mrt_print_gerror(...) \
	_mrt_print_gerror(__VA_ARGS__, ' ')

static void mrt_init_font(const mrt_context_t *ctx);
static void mrt_toggle_fullscreen(mrt_context_t *ctx);
static void mrt_toggle_scrollbar(mrt_context_t *ctx);

static gchar *mrt_find_shell(const mrt_context_t *ctx);
static gchar *mrt_find_link(const mrt_context_t *ctx, GdkEvent *event);
static gboolean mrt_open_link(const mrt_context_t *ctx, const gchar *link);

static gboolean mrt_on_key_press(GtkWidget *widget, GdkEvent *event, gpointer data);
static gboolean mrt_on_button_press(GtkWidget *widget, GdkEvent *event, gpointer data);

static void mrt_on_window_destroy(GtkWidget *widget, gpointer data);
static void mrt_on_window_title_changed(VteTerminal *term, gpointer data);

static void mrt_on_spawn(VteTerminal *term, GPid pid, GError *err, gpointer data);
static void mrt_on_child_exited(VteTerminal *term, gint exit_code, gpointer data);
static gboolean mrt_on_draw(GtkWidget *widget, cairo_t *cr, gpointer data);

static void mrt_on_bell(VteTerminal *term, gpointer data);
static void mrt_on_hyperlink_changed(VteTerminal *term, gchar *url, GdkRectangle *bbox, gpointer data);

static void mrt_context_menu_on_copy(GtkWidget *widget, gpointer data);
static void mrt_context_menu_on_paste(GtkWidget *widget, gpointer data);
static void mrt_context_menu_on_fullscreen(GtkWidget *widget, gpointer data);
static void mrt_context_menu_on_scrollbar(GtkWidget *widget, gpointer data);
static void mrt_context_menu_on_close(GtkWidget *widget, gpointer data);

gboolean mrt_init(mrt_context_t *ctx)
{
	int i, tag;
	cairo_surface_t *surface;
	cairo_status_t status;
	GdkRGBA colors[MRT_MAX_COLORS];
	GdkRGBA color;
	GtkWidget *menu_item;
	GtkWidget *context_menu;
	VteRegex *url_regex;
	GError *err = NULL;

	ctx->link = NULL;
	if(ctx->allow_link)
	{
		if(!MRT_ISSET(ctx->link_handler))
			ctx->allow_link = FALSE;
		else if(!ctx->allow_hyperlink && !MRT_ISSET(ctx->link_regex))
			ctx->allow_link = FALSE;
	}

	ctx->win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT(ctx->win), "destroy", G_CALLBACK(mrt_on_window_destroy), ctx);

	gtk_window_set_title(GTK_WINDOW(ctx->win), ctx->title);

	if(MRT_ISSET(ctx->icon_name))
		gtk_window_set_icon_name(GTK_WINDOW(ctx->win), ctx->icon_name);

	if(ctx->borderless)
		gtk_window_set_decorated(GTK_WINDOW(ctx->win), FALSE);

	if(ctx->maximized)
		gtk_window_maximize(GTK_WINDOW(ctx->win));

	if(ctx->fullscreen)
	{
		ctx->fullscreen = FALSE;
		mrt_toggle_fullscreen(ctx);
	}

	if(ctx->centered)
		gtk_window_set_position(GTK_WINDOW(ctx->win), GTK_WIN_POS_CENTER_ALWAYS);

	if(ctx->allow_context_menu)
	{
		context_menu = gtk_menu_new();
		if(context_menu == NULL)
		{
			mrt_log("failed to create context menu");
			return FALSE;
		}
		ctx->context_menu = context_menu;

		menu_item = gtk_menu_item_new_with_label("Copy");
		gtk_menu_shell_append(GTK_MENU_SHELL(context_menu), menu_item);
		g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(mrt_context_menu_on_copy), ctx);

		menu_item = gtk_menu_item_new_with_label("Paste");
		gtk_menu_shell_append(GTK_MENU_SHELL(context_menu), menu_item);
		g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(mrt_context_menu_on_paste), ctx);

		if(ctx->allow_context_menu_fullscreen)
		{
			gtk_menu_shell_append(GTK_MENU_SHELL(context_menu), gtk_separator_menu_item_new());

			menu_item = gtk_check_menu_item_new_with_label("Fullscreen");
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), ctx->fullscreen);
			gtk_menu_shell_append(GTK_MENU_SHELL(context_menu), menu_item);
			g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(mrt_context_menu_on_fullscreen), ctx);

			ctx->fullscreen_menu_item = menu_item;
		}

		if(ctx->allow_context_menu_scrollbar)
		{
			gtk_menu_shell_append(GTK_MENU_SHELL(context_menu), gtk_separator_menu_item_new());

			menu_item = gtk_check_menu_item_new_with_label("Scrollbar");
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), ctx->show_scrollbar);
			gtk_menu_shell_append(GTK_MENU_SHELL(context_menu), menu_item);
			g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(mrt_context_menu_on_scrollbar), ctx);
		}

		if(ctx->allow_context_menu_close)
		{
			gtk_menu_shell_append(GTK_MENU_SHELL(context_menu), gtk_separator_menu_item_new());

			menu_item = gtk_menu_item_new_with_label("Close");
			gtk_menu_shell_append(GTK_MENU_SHELL(context_menu), menu_item);
			g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(mrt_context_menu_on_close), ctx);
		}

		gtk_widget_show_all(context_menu);
	}

	ctx->term = vte_terminal_new();

	if(ctx->allow_scrollbar)
	{
		ctx->scrollbar = gtk_scrollbar_new(
			GTK_ORIENTATION_VERTICAL,
			gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(ctx->term))
		);

		if(ctx->scrollbar != NULL)
		{
			GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

			gtk_box_pack_start(GTK_BOX(hbox), ctx->term, TRUE, TRUE, 0);
			gtk_box_pack_start(GTK_BOX(hbox), ctx->scrollbar, FALSE, FALSE, 0);

			gtk_container_add(GTK_CONTAINER(ctx->win), hbox);
		}
		else
		{
			ctx->allow_scrollbar = FALSE;
			ctx->show_scrollbar = FALSE;
			ctx->allow_context_menu_scrollbar = FALSE;
		}
	}
	else
	{
		gtk_container_add(GTK_CONTAINER(ctx->win), ctx->term);
	}

	mrt_init_font(ctx);

	gtk_widget_show_all(ctx->win);

	if(ctx->allow_scrollbar && !ctx->show_scrollbar)
	{
		ctx->show_scrollbar = TRUE;
		mrt_toggle_scrollbar(ctx);
	}

	vte_terminal_set_audible_bell(VTE_TERMINAL(ctx->term), FALSE);
	vte_terminal_set_word_char_exceptions(VTE_TERMINAL(ctx->term), ctx->word_char_exceptions);
	vte_terminal_set_bold_is_bright(VTE_TERMINAL(ctx->term), ctx->bold_is_bright);
	vte_terminal_set_cursor_blink_mode(VTE_TERMINAL(ctx->term), ctx->cursor_blink_mode);
	vte_terminal_set_cursor_shape(VTE_TERMINAL(ctx->term), ctx->cursor_shape);
	vte_terminal_set_mouse_autohide(VTE_TERMINAL(ctx->term), ctx->mouse_autohide);
	vte_terminal_set_scroll_on_output(VTE_TERMINAL(ctx->term), ctx->scroll_on_output);
	vte_terminal_set_scroll_on_keystroke(VTE_TERMINAL(ctx->term), ctx->scroll_on_keystroke);
	vte_terminal_set_scrollback_lines(VTE_TERMINAL(ctx->term), ctx->scrollback_lines);
	vte_terminal_set_allow_hyperlink(VTE_TERMINAL(ctx->term), ctx->allow_hyperlink);

	for(i = 0; i < MRT_MAX_COLORS; i++)
	{
		if(ctx->colors[i] == NULL)
			break;

		if(!gdk_rgba_parse(&colors[i], ctx->colors[i]))
		{
			mrt_log("invalid color '%s' at index %d", ctx->colors[i], i);
			break;
		}
	}
	vte_terminal_set_colors(VTE_TERMINAL(ctx->term), NULL, NULL, colors, i);

	if(ctx->foreground_color != NULL)
	{
		if(gdk_rgba_parse(&color, ctx->foreground_color))
			vte_terminal_set_color_foreground(VTE_TERMINAL(ctx->term), &color);
		else
			mrt_log("invalid foreground color '%s'", ctx->foreground_color);
	}

	if(ctx->background_color != NULL)
	{
		if(gdk_rgba_parse(&color, ctx->background_color))
			vte_terminal_set_color_background(VTE_TERMINAL(ctx->term), &color);
		else
			mrt_log("invalid background color '%s'", ctx->background_color);
	}

	if(ctx->bold_color != NULL)
	{
		if(gdk_rgba_parse(&color, ctx->bold_color))
			vte_terminal_set_color_bold(VTE_TERMINAL(ctx->term), &color);
		else
			mrt_log("invalid bold color '%s'", ctx->bold_color);
	}

	if(ctx->cursor_background_color != NULL)
	{
		if(gdk_rgba_parse(&color, ctx->cursor_background_color))
			vte_terminal_set_color_cursor(VTE_TERMINAL(ctx->term), &color);
		else
			mrt_log("invalid cursor background color '%s'", ctx->cursor_background_color);
	}

	if(ctx->cursor_foreground_color != NULL)
	{
		if(gdk_rgba_parse(&color, ctx->cursor_foreground_color))
			vte_terminal_set_color_cursor_foreground(VTE_TERMINAL(ctx->term), &color);
		else
			mrt_log("invalid cursor foreground color '%s'", ctx->cursor_foreground_color);
	}

	if(ctx->highlight_background_color != NULL)
	{
		if(gdk_rgba_parse(&color, ctx->highlight_background_color))
			vte_terminal_set_color_highlight(VTE_TERMINAL(ctx->term), &color);
		else
			mrt_log("invalid highlight background color '%s'", ctx->highlight_background_color);
	}

	if(ctx->highlight_foreground_color != NULL)
	{
		if(gdk_rgba_parse(&color, ctx->highlight_foreground_color))
			vte_terminal_set_color_highlight_foreground(VTE_TERMINAL(ctx->term), &color);
		else
			mrt_log("invalid hightlight foreground color '%s'", ctx->highlight_foreground_color);
	}

	if(ctx->allow_link && MRT_ISSET(ctx->link_regex))
	{
		url_regex = vte_regex_new_for_match(ctx->link_regex, strlen(ctx->link_regex), PCRE2_MULTILINE, &err);
		if(url_regex != NULL)
		{
			tag = vte_terminal_match_add_regex(VTE_TERMINAL(ctx->term), url_regex, 0);

			if(tag != -1 && MRT_ISSET(ctx->link_cursor_name))
				vte_terminal_match_set_cursor_name(VTE_TERMINAL(ctx->term), tag, ctx->link_cursor_name);

			vte_regex_unref(url_regex);
		}
		else
		{
			mrt_print_gerror(err, "invalid link regex '%s'", ctx->link_regex);
			g_clear_error(&err);
		}
	}

	g_signal_connect(G_OBJECT(ctx->term), "bell", G_CALLBACK(mrt_on_bell), ctx);
	g_signal_connect(G_OBJECT(ctx->term), "child-exited", G_CALLBACK(mrt_on_child_exited), ctx);

	if(ctx->background_image != NULL)
	{
		surface = cairo_image_surface_create_from_png(ctx->background_image);
		status = cairo_surface_status(surface);
		if(status == CAIRO_STATUS_SUCCESS)
		{
			ctx->background_image_surface = surface;

			vte_terminal_set_clear_background(VTE_TERMINAL(ctx->term), FALSE);
			g_signal_connect(G_OBJECT(ctx->term), "draw", G_CALLBACK(mrt_on_draw), ctx);
		}
		else
		{
			mrt_log("background image error: '%s'", cairo_status_to_string(status));
		}
	}

	if(ctx->allow_context_menu || ctx->allow_link)
		g_signal_connect(G_OBJECT(ctx->term), "button-press-event", G_CALLBACK(mrt_on_button_press), ctx);

	if(ctx->allow_fullscreen_toggle_shortcut || ctx->allow_copy_paste_shortcut || ctx->allow_hold_escape_shortcut)
		g_signal_connect(G_OBJECT(ctx->term), "key-press-event", G_CALLBACK(mrt_on_key_press), ctx);

	if(ctx->allow_link && ctx->allow_hyperlink)
		g_signal_connect(G_OBJECT(ctx->term), "hyperlink-hover-uri-changed", G_CALLBACK(mrt_on_hyperlink_changed), ctx);

	if(ctx->allow_change_title)
		g_signal_connect(G_OBJECT(ctx->term), "window-title-changed", G_CALLBACK(mrt_on_window_title_changed), ctx);

	gtk_widget_grab_focus(ctx->term);
	return TRUE;
}

void mrt_shutdown(mrt_context_t *ctx)
{
	g_free(ctx->link);
	ctx->link = NULL;

	if(ctx->background_image_surface != NULL)
	{
		cairo_surface_destroy(ctx->background_image_surface);
		ctx->background_image_surface = NULL;
	}
}

gboolean mrt_spawn(mrt_context_t *ctx)
{
	gchar *shell_argv[] = { NULL, NULL, NULL };
	gchar **argv;
	GSpawnFlags flags;

	ctx->exit_code = 0;

	if(ctx->spawn_argv != NULL)
	{
		argv = (gchar **) ctx->spawn_argv;
		flags = G_SPAWN_SEARCH_PATH;
	}
	else
	{
		shell_argv[0] = mrt_find_shell(ctx);

		if(ctx->login_shell)
			shell_argv[1] = g_strdup_printf("-%s", shell_argv[0]);
		else
			shell_argv[1] = g_strdup(shell_argv[0]);

		if(shell_argv[0] == NULL || shell_argv[1] == NULL)
		{
			ctx->exit_code = -1;
			mrt_log("could not find shell");
			return FALSE;
		}

		argv = shell_argv;
		flags = G_SPAWN_SEARCH_PATH | G_SPAWN_FILE_AND_ARGV_ZERO;
	}

	g_setenv(MRT_ENVIRONMENT_VARIABLE_NAME, ctx->background_image != NULL ? "1" : "0", FALSE);

	vte_terminal_spawn_async(
		VTE_TERMINAL(ctx->term),
		VTE_PTY_DEFAULT,
		NULL,
		argv,
		NULL,
		flags,
		NULL,
		NULL,
		NULL,
		-1,
		NULL,
		mrt_on_spawn,
		ctx
	);

	g_free(shell_argv[0]);
	g_free(shell_argv[1]);
	return TRUE;
}

static void mrt_init_font(const mrt_context_t *ctx)
{
	PangoFontDescription *font_desc;

	if(!MRT_ISSET(ctx->font))
		return;

	font_desc = pango_font_description_from_string(ctx->font);
	if(font_desc == NULL)
		return;

	vte_terminal_set_font(VTE_TERMINAL(ctx->term), font_desc);

	if(ctx->font_scale > 0.0)
		vte_terminal_set_font_scale(VTE_TERMINAL(ctx->term), ctx->font_scale);

	pango_font_description_free(font_desc);
}

static void mrt_toggle_fullscreen(mrt_context_t *ctx)
{
	ctx->fullscreen = !ctx->fullscreen;

	if(ctx->fullscreen)
		gtk_window_fullscreen(GTK_WINDOW(ctx->win));
	else
		gtk_window_unfullscreen(GTK_WINDOW(ctx->win));
}

static void mrt_toggle_scrollbar(mrt_context_t *ctx)
{
	ctx->show_scrollbar = !ctx->show_scrollbar;

	if(ctx->show_scrollbar)
		gtk_widget_show(ctx->scrollbar);
	else
		gtk_widget_hide(ctx->scrollbar);
}

static gchar *mrt_find_shell(const mrt_context_t *ctx)
{
	gchar *shell;

	if(MRT_ISSET(ctx->shell))
		return g_strdup(ctx->shell);

	shell = vte_get_user_shell();
	if(MRT_ISSET(shell))
		return shell;

	g_free(shell);

	shell = g_strdup(g_getenv("SHELL"));
	if(MRT_ISSET(shell))
		return shell;

	g_free(shell);

	return g_strdup(MRT_FALLBACK_SHELL);
}

static gchar *mrt_find_link(const mrt_context_t *ctx, GdkEvent *event)
{
	gchar *link;

	if((link = vte_terminal_hyperlink_check_event(VTE_TERMINAL(ctx->term), event)) != NULL)
		return link;

	if((link = vte_terminal_match_check_event(VTE_TERMINAL(ctx->term), event, NULL)) != NULL)
		return link;

	return NULL;
}

static gboolean mrt_open_link(const mrt_context_t *ctx, const gchar *link)
{
	const gchar *argv[] = {
		ctx->link_handler,
		link,
		NULL
	};
	GError *err = NULL;

	if(link == NULL)
		return FALSE;

	if(!g_spawn_async(NULL, (gchar **) argv, NULL, G_SPAWN_DEFAULT | G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &err))
	{
		mrt_print_gerror(err, "could not spawn link handler for '%s'", link);
		g_clear_error(&err);
		return FALSE;
	}

	return TRUE;
}

static gboolean mrt_on_key_press(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	GdkEventKey *kevent = (GdkEventKey *) event;
	mrt_context_t *ctx = (mrt_context_t *) data;

	MRT_UNUSED(widget);

	if(ctx->allow_hold_escape_shortcut && ctx->hold && ctx->has_exit_code)
	{
		switch(kevent->keyval)
		{
			case GDK_KEY_Escape:
				mrt_quit(ctx);
				return TRUE;
		}
	}

	if(ctx->allow_fullscreen_toggle_shortcut && (kevent->state & GDK_MOD1_MASK))
	{
		switch(kevent->keyval)
		{
			case GDK_KEY_Return:
			{
				if(ctx->fullscreen_menu_item != NULL)
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(ctx->fullscreen_menu_item), !ctx->fullscreen);
				else
					mrt_toggle_fullscreen(ctx);
			}
			return TRUE;
		}
	}

	if(ctx->allow_copy_paste_shortcut && (kevent->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)))
	{
		switch(kevent->keyval)
		{
			case GDK_KEY_C:
				vte_terminal_copy_clipboard_format(VTE_TERMINAL(ctx->term), VTE_FORMAT_TEXT);
				return TRUE;

			case GDK_KEY_V:
				vte_terminal_paste_clipboard(VTE_TERMINAL(ctx->term));
				return TRUE;
		}
	}

	return FALSE;
}

static gboolean mrt_on_button_press(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gchar *link;
	GdkEventButton *bevent = (GdkEventButton *) event;
	mrt_context_t *ctx = (mrt_context_t *) data;

	MRT_UNUSED(widget);

	if(event->type != GDK_BUTTON_PRESS)
		return FALSE;

	if(ctx->allow_context_menu && (bevent->button == GDK_BUTTON_SECONDARY))
	{
		g_free(ctx->link);
		ctx->link = mrt_find_link(ctx, event);

		gtk_menu_popup_at_pointer(GTK_MENU(ctx->context_menu), event);
		return TRUE;
	}

	if(ctx->allow_link && ((bevent->button == GDK_BUTTON_PRIMARY) && (bevent->state & GDK_CONTROL_MASK)))
	{
		if((link = mrt_find_link(ctx, event)) != NULL)
		{
			mrt_open_link(ctx, link);
			g_free(link);
			return TRUE;
		}
	}

	return FALSE;
}

static void mrt_on_window_destroy(GtkWidget *widget, gpointer data)
{
	mrt_context_t *ctx = (mrt_context_t *) data;

	MRT_UNUSED(widget);

	if(!ctx->has_exit_code)
		ctx->exit_code = EXIT_FAILURE;

	gtk_main_quit();
}

static void mrt_on_window_title_changed(VteTerminal *term, gpointer data)
{
	mrt_context_t *ctx = (mrt_context_t *) data;
	const gchar *title = vte_terminal_get_window_title(term);

	if(MRT_ISSET(title))
		gtk_window_set_title(GTK_WINDOW(ctx->win), title);
	else
		gtk_window_set_title(GTK_WINDOW(ctx->win), ctx->title);
}

static void mrt_on_spawn(VteTerminal *term, GPid pid, GError *err, gpointer data)
{
	mrt_context_t *ctx = (mrt_context_t *) data;

	MRT_UNUSED(term);

	if(pid != -1)
		return;

	mrt_print_gerror(err, "error on spawn");
	mrt_quit(ctx);
}

static gboolean mrt_on_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	double w, h, sx, sy;
	GdkPoint *p;
	cairo_surface_t *surface;
	mrt_context_t *ctx;

	ctx = (mrt_context_t *) data;

	surface = ctx->background_image_surface;

	cairo_save(cr);

	gdk_cairo_set_source_rgba(cr, &ctx->background_image_color);
	cairo_paint(cr);

	if(ctx->allow_background_image_scale)
	{
		p = &ctx->background_image_scale;
		if(p->x != 0 && p->y != 0)
			cairo_scale(cr, p->x, p->y);
	}
	else if(ctx->allow_background_image_autoscale)
	{
		w = cairo_image_surface_get_width(surface);
		h = cairo_image_surface_get_height(surface);

		if(w > 0 && h > 0)
		{
			sx = gtk_widget_get_allocated_width(widget) / w;
			sy = gtk_widget_get_allocated_height(widget) / h;

			if(sx != 0 && sy != 0)
				cairo_scale(cr, sx, sy);
		}
	}

	p = &ctx->background_image_position;
	cairo_set_source_surface(cr, surface, p->x, p->y);
	cairo_paint(cr);

	gdk_cairo_set_source_rgba(cr, &ctx->background_image_overlay_color);
	cairo_paint(cr);

	cairo_restore(cr);
	return FALSE;
}

static void mrt_on_child_exited(VteTerminal *term, gint exit_code, gpointer data)
{
	mrt_context_t *ctx = (mrt_context_t *) data;

	MRT_UNUSED(term);

	ctx->has_exit_code = TRUE;
	ctx->exit_code = exit_code;

	if(ctx->hold)
		return;

	mrt_quit(ctx);
}

static void mrt_on_bell(VteTerminal *term, gpointer data)
{
	mrt_context_t *ctx = (mrt_context_t *) data;

	MRT_UNUSED(term);

	gtk_window_set_urgency_hint(GTK_WINDOW(ctx->win), FALSE);
}

static void mrt_on_hyperlink_changed(VteTerminal *term, gchar *url, GdkRectangle *bbox, gpointer data)
{
	MRT_UNUSED(bbox);
	MRT_UNUSED(data);

	if(MRT_ISSET(url))
		gtk_widget_set_tooltip_text(GTK_WIDGET(term), url);
	else
		gtk_widget_set_has_tooltip(GTK_WIDGET(term), FALSE);
}

static void mrt_context_menu_on_copy(GtkWidget *widget, gpointer data)
{
	mrt_context_t *ctx = (mrt_context_t *) data;

	MRT_UNUSED(widget);

	if(vte_terminal_get_has_selection(VTE_TERMINAL(ctx->term)))
		vte_terminal_copy_clipboard_format(VTE_TERMINAL(ctx->term), VTE_FORMAT_TEXT);
	else if(ctx->link != NULL)
		gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), ctx->link, -1);
}

static void mrt_context_menu_on_paste(GtkWidget *widget, gpointer data)
{
	mrt_context_t *ctx = (mrt_context_t *) data;

	MRT_UNUSED(widget);

	vte_terminal_paste_clipboard(VTE_TERMINAL(ctx->term));
}

static void mrt_context_menu_on_fullscreen(GtkWidget *widget, gpointer data)
{
	MRT_UNUSED(widget);

	mrt_toggle_fullscreen((mrt_context_t *) data);
}

static void mrt_context_menu_on_scrollbar(GtkWidget *widget, gpointer data)
{
	MRT_UNUSED(widget);

	mrt_toggle_scrollbar((mrt_context_t *) data);
}

static void mrt_context_menu_on_close(GtkWidget *widget, gpointer data)
{
	MRT_UNUSED(widget);

	mrt_quit((mrt_context_t *) data);
}
