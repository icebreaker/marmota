/* {{{
	MIT LICENSE

	Copyright (c) 2020-2024, Mihail Szabolcs

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
#ifndef MRT_MARMOTA_H
#define MRT_MARMOTA_H

#include <gtk/gtk.h>
#include <vte/vte.h>

#ifndef PCRE2_CODE_UNIT_WIDTH
	#define PCRE2_CODE_UNIT_WIDTH 8
#endif
#include <pcre2.h>

#include "pl_mpeg.h"

#ifndef MRT_VERSION
	#define MRT_VERSION "1.0.9"
#endif

#ifndef MRT_UNUSED
	#define MRT_UNUSED(x) (void)(x)
#endif

#ifndef MRT_CLAMP
	#define MRT_CLAMP(x, min, max) (((x) < min) ? min : (((x) > max) ? max : (x)))
#endif

#ifndef MRT_ENVIRONMENT_VARIABLE_NAME
	#define MRT_ENVIRONMENT_VARIABLE_NAME "MARMOTA"
#endif

#ifndef MRT_MAX_COLORS
	#define MRT_MAX_COLORS 16
#endif

#ifndef MRT_FALLBACK_SHELL
	#define MRT_FALLBACK_SHELL "/bin/sh"
#endif

#ifndef MRT_FONT_SCALE_MIN
	#define MRT_FONT_SCALE_MIN 0.25
#endif

#ifndef MRT_FONT_SCALE_MAX
	#define MRT_FONT_SCALE_MAX 4.0
#endif

#ifndef MRT_VIDEO_DECODE_MAX_FPS
	#define MRT_VIDEO_DECODE_MAX_FPS 1.0 / 30.0
#endif

#ifndef MRT_VIDEO_SEEK_TO_AMOUNT
	#define MRT_VIDEO_SEEK_TO_AMOUNT 3
#endif

#ifndef MRT_CONTROL_SHIFT_MASK
	#define MRT_CONTROL_SHIFT_MASK (GDK_CONTROL_MASK | GDK_SHIFT_MASK)
#endif

typedef struct
{
	gboolean hold;
	gboolean allow_change_title;
	gboolean login_shell;
	gboolean fullscreen;
	gboolean borderless;
	gboolean maximized;
	gboolean centered;
	gboolean mouse_autohide;
	gboolean bold_is_bright;
	gboolean scroll_on_output;
	gboolean scroll_on_keystroke;
	gboolean show_scrollbar;
	gboolean allow_scrollbar;
	gboolean allow_hyperlink;
	gboolean allow_link;
	gboolean allow_context_menu;
	gboolean allow_context_menu_fullscreen;
	gboolean allow_context_menu_close;
	gboolean allow_context_menu_scrollbar;
	gboolean allow_context_menu_font_scale;
	gboolean allow_fullscreen_toggle_shortcut;
	gboolean allow_copy_paste_shortcut;
	gboolean allow_hold_escape_shortcut;
	gboolean allow_font_scale_shortcut;
	gboolean allow_background_video_seek_shortcut;
	gboolean allow_background_image_scale;
	gboolean allow_background_image_autoscale;
	gdouble font_scale;
	gdouble font_scale_increment;
	gdouble font_scale_current;
	guint scrollback_lines;
	VteCursorBlinkMode cursor_blink_mode;
	VteCursorShape cursor_shape;
	GdkRGBA background_image_color;
	GdkRGBA background_image_overlay_color;
	GdkPoint background_image_position;
	GdkPoint background_image_scale;
	const gchar *shell;
	const gchar *icon_name;
	const gchar *word_char_exceptions;
	const gchar *title;
	const gchar *font;
	const gchar *link_handler;
	const gchar *link_regex;
	const gchar *link_cursor_name;
	const gchar *bold_color;
	const gchar *cursor_background_color;
	const gchar *cursor_foreground_color;
	const gchar *highlight_background_color;
	const gchar *highlight_foreground_color;
	const gchar *foreground_color;
	const gchar *background_color;
	const gchar *colors[MRT_MAX_COLORS];
	const gchar *background_image;
	const gchar **spawn_argv;
	gchar *link;
	cairo_surface_t *background_image_surface;
	GtkWidget *term;
	GtkWidget *win;
	GtkWidget *context_menu;
	GtkWidget *fullscreen_menu_item;
	GtkWidget *scrollbar;
	gboolean has_exit_code;
	gint exit_code;
	plm_t *plm;
	guchar *background_video_buffer;
	guint background_video_decode_timer_id;
	gint64 background_video_decode_start_time;
	gint background_video_decode_seek_to;
} mrt_context_t;

gboolean mrt_init(mrt_context_t *ctx);
void mrt_shutdown(mrt_context_t *ctx);
gboolean mrt_spawn(mrt_context_t *ctx);

#endif
