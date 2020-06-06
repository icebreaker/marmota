marmota
=======
marmota is an _opinionated_ and _minimalist_ terminal emulator based on [VTE][1].

![marmota logo by T. Michael Keesey](marmota.png)

Getting Started
---------------
There are no precompiled binaries, therefore you'll have to compile marmota yourself.

This isn't terribly difficult, since there are only a handful of dependencies,
namely: [GTK+ 3.0+][2] and [VTE 0.91+][1].

Once you installed the _development_ versions (with headers) of these the dependencies,
it becomes possible to compile marmota by typing typing `make` in a terminal.

```bash
$ make
```

This will result in a `build/config.h` header file a and `build/marmota` executable.

The `build/config.h` header file contains the default compile time configuration.

In most cases, you will want to modify `buld/config.h` and then recompile marmota
by typing `make` again in a terminal.

```bash
$ vim build/config.h
...
<edit configuration>
...
$ make
```

Once marmota has been compiled, it can be _installed_ in `/usr/local` or
another prefix of your choice by typing `make install` in a terminal.

```bash
$ make install

or

$ make install PREFIX=/opt
```

By default, marmota allows you to customize _the first 16_ colors via the
`.colors` array in `build/config.h`.

If more colors are desired or needed, then marmota needs to be compiled as such.

```bash
$ make MAX_COLORS=24
```

In the example above, we reserved 24 colors instead of 16.

Any color entries, not specified in `build/config.h`, will default to reasonable
hard-coded values.

### Command Line Arguments
When no command line arguments are given, marmota will attempt to detect the current
user's preferred SHELL or fallback to `/bin/sh`.

Whether or not this is a _login shell_ is determined by the `.login_shell`
(defaults to `true`) option in `build/config.h`.

To execute another command, instead of the SHELL, the `-e` command
line argument can be used.

```bash
$ marmota -e tmux
```

or

```bash
$ marmota -e /bin/sh -l -c tmux
```

Any arguments after `-e` are considered to be part of the command to execute.

In addition to `-e`, there's an additional argument, called `-hold` that can be
leveraged to prevent marmota from closing after the SHELL or the given command has
exited.

This can be useful, when using marmota as a _terminal_ from within some IDE, and
it's desirable to see the output after the SHELL or the the given command has
exited.

```bash
$ marmota -hold -e ls -lah
```

If the `.allow_hold_escape_shortcut` configuration option has been set to `true`
(default to `true`), then it is possible to close marmota by pressing escape
after the given command has exited in `-hold` mode.

Contribute
----------
* Fork the project.
* Make your feature addition or bug fix.
* Do **not** bump the version number.
* Create a pull request. Bonus points for topic branches.

License
-------
**marmota** is provided **as-is** under the **MIT** license.
For more information see LICENSE.

[Marmota monax (Linnaeus, 1758)][3] by T. Michael Keesey is licensed under CC0 1.0.

[1]: https://gitlab.gnome.org/GNOME/vte
[2]: https://www.gtk.org/
[3]: http://phylopic.org/image/eee50efb-40dc-47d0-b2cb-52a14a5e0e51/
