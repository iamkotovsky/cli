# cli

Small single-header, X-macro based typed CLI parser for C.

`cli.h` generates a concrete `cli_t` struct, parser, and usage output from one
argument schema. It is intended for small C tools that want typed options
without writing repetitive parsing code.

## Features

- Single-header CLI parser
- X-macro argument schema as the source of truth
- Generated typed `cli_t` result struct
- Required and optional flags
- Default values for optional flags
- Generated usage text showing optional defaults
- `string`, `int`, `float`, and `bool` argument kinds
- Long and short options
- `--name value` and `--name=value`
- Generated usage text with aligned descriptions
- Optional passthrough arguments after `--`

## Integration

`cli.h` depends on `strview.h`.

Clone with submodules:

```sh
git clone --recurse-submodules https://github.com/iamkotovsky/cli.git
```

Or initialize the dependency after cloning:

```sh
git submodule update --init --recursive
```

In one translation unit:

```c
#define STRVIEW_IMPLEMENTATION

#define CLI_ARGS(REQUIRED, OPTIONAL)                                           \
    REQUIRED(STRING, name, "name", 'n', "Name to print")                      \
    REQUIRED(STRING, task, "task", 't', "Task to run")                        \
    OPTIONAL(INT, repeat, "repeat", 'r', 1, "Repeat count")                   \
    OPTIONAL(BOOL, verbose, "verbose", 'v', false, "Enable verbose output")

#include "cli/cli.h"
```

Then parse:

```c
int main(int argc, char **argv) {
    cli_t cli;
    char error[256];

    if (!cli_parse(argc, argv, &cli, error, sizeof(error))) {
        fprintf(stderr, "error: %s\n\n", error);
        cli_usage(stderr, argv[0]);
        return 1;
    }

    printf("name: %.*s\n", (int)cli.name.length, cli.name.data);
    printf("task: %.*s\n", (int)cli.task.length, cli.task.data);
    printf("repeat: %d\n", cli.repeat);
    printf("verbose: %s\n", cli.verbose ? "true" : "false");

    return 0;
}
```

## Schema Contract

Define `CLI_ARGS(REQUIRED, OPTIONAL)` before including `cli.h`.

```c
REQUIRED(kind, field, long_name, short_name, description)
OPTIONAL(kind, field, long_name, short_name, default_value, description)
```

Use `0` as `short_name` to omit the short option:

```c
OPTIONAL(BOOL, verbose, "verbose", 0, false, "Enable verbose output")
```

Supported kinds:

- `STRING` -> `strview_t`
- `INT` -> `int`
- `FLOAT` -> `float`
- `BOOL` -> `bool`

Boolean options do not require a value when used as flags:

```sh
tool --verbose
tool -v
```

They can also be set explicitly:

```sh
tool --verbose=false
```

## Passthrough Arguments

Define `CLI_CUSTOM_ARGS` before including `cli.h` to collect arguments after a
literal `--`.

```c
#define CLI_CUSTOM_ARGS
#define CLI_ARGS(REQUIRED, OPTIONAL) /* ... */
#include "cli/cli.h"
```

Then `cli_t` also contains:

```c
char **argv;
int argc;
```

Example:

```sh
tool --name danil --task build -- file.c -Wall
```

Everything after `--` is stored in `cli.argv` and `cli.argc`.

Generated usage shows optional defaults at the end of each option description:

```text
options:
  -n, --name <string>    Name to print
  -r, --repeat <int>     Repeat count (default: 1)
      --verbose          Enable verbose output (default: false)
```

## API

- `cli_t`
- `cli_parse`
- `cli_usage`

## License

MIT.
