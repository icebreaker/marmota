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
#include <stdbool.h>

#include <marmota.h>

#define TRANSLATE_EXIT_CODE(x) \
	(x)->exit_code == 0 ? EXIT_SUCCESS : EXIT_FAILURE

static bool parse_args(mrt_context_t *ctx, const int argc, const char *argv[]);
static void show_help(const char *name, const char *arg);

int main(int argc, char *argv[])
{
	mrt_context_t ctx = {
#include <config.h>
	};

	gtk_init(&argc, &argv);

	if(!parse_args(&ctx, argc, (const char **) argv))
		return TRANSLATE_EXIT_CODE(&ctx);

	if(!mrt_init(&ctx))
		return TRANSLATE_EXIT_CODE(&ctx);

	if(!mrt_spawn(&ctx))
		return TRANSLATE_EXIT_CODE(&ctx);

	gtk_main();

	mrt_shutdown(&ctx);
	return TRANSLATE_EXIT_CODE(&ctx);
}

static bool parse_args(mrt_context_t *ctx, const int argc, const char *argv[])
{
	int i;
	const char *arg;
	const char *name = argv[0];

	for(i = 1; i < argc; i++)
	{
		arg = argv[i];

		if(!strcmp(arg, "-h") || !strcmp(arg, "--help"))
		{
			show_help(name, NULL);
			return FALSE;
		}
		else if(!strcmp(arg, "-v") || !strcmp(arg, "--version"))
		{
			fprintf(stdout, "%s v%s\n", name, MRT_VERSION);
			return FALSE;
		}
		else if(!strcmp(arg, "-hold"))
		{
			ctx->hold = TRUE;
			ctx->maximized = FALSE;
			ctx->borderless = FALSE;
			ctx->fullscreen = FALSE;
		}
		else if(!strcmp(arg, "-e") && i < argc - 1)
		{
			ctx->spawn_argv = &argv[++i];
			break;
		}
		else if((!strcmp(arg, "-f") || !strcmp(arg, "--font")) && i < argc - 1)
		{
			ctx->font = (const char *) argv[++i];
		}
		else if((!strcmp(arg, "-i") || !strcmp(arg, "--icon")) && i < argc - 1)
		{
			ctx->icon_name = (const char *) argv[++i];
		}
		else if(!strcmp(arg, "-background") && i < argc - 1)
		{
			ctx->background_image = (const char *) argv[++i];
		}
		else if(!strcmp(arg, "-background-opacity") && i < argc - 1)
		{
			ctx->background_image_overlay_color.alpha = g_ascii_strtod(argv[++i], NULL);
		}
		else if(!strcmp(arg, "-maximized"))
		{
			ctx->maximized = TRUE;
		}
		else if(!strcmp(arg, "-borderless"))
		{
			ctx->borderless = TRUE;
		}
		else if(!strcmp(arg, "-fullscreen"))
		{
			ctx->fullscreen = TRUE;
		}
		else
		{
			ctx->exit_code = -1;
			show_help(name, arg);
			return FALSE;
		}
	}

	if(ctx->title == NULL)
		ctx->title = name;

	return TRUE;
}

static void show_help(const char *name, const char *arg)
{
	if(arg != NULL)
	{
		fprintf(
			stderr,
			"invalid or unknown argument '%s'\n"
			"try '%s --help' for more information\n",
			arg,
			name
		);
		return;
	}

	fprintf(
		stderr,
		"%s v%s\n\n"
		"usage: %s [arguments]\n\n"
		"arguments:\n"
		"\t-e [arguments]\t- command to execute\n"
		"\t-hold\t\t- hold window after exit\n"
		"\t-maximized\t- force window to be maximized\n"
		"\t-borderless\t- force window to be borderless\n"
		"\t-fullscreen\t- force window to be fullscreen\n"
		"\t-background\t- set background image (i.e: 'terminal.png')\n"
		"\t-f, --font\t- set font (i.e: 'IBM Plex Mono weight=650 19')\n"
		"\t-i, --icon\t- set icon (i.e: 'launchpad')\n"
		"\t-h, --help\t- show this help\n"
		"\t-v, --version\t- display version\n",
		name,
		MRT_VERSION,
		name
	);
}
