/*
 * cli.h
 * Version: 0.1.0
 * License: MIT
 * Repository: https://github.com/iamkotovsky/cli
 * Description: Small single-header, X-macro based typed CLI parser for C.
 *
 * Define CLI_ARGS(REQUIRED, OPTIONAL) before including this header.
 * Define CLI_CUSTOM_ARGS before including this header to collect arguments
 * after a literal "--" into cli_t.argc and cli_t.argv.
 */
#ifndef CLI_H
#define CLI_H

#include "strview/strview.h"

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CLI_ARGS
#error "define CLI_ARGS(REQUIRED, OPTIONAL) before including cli.h"
#endif

#define CLI_TYPE_STRING strview_t
#define CLI_TYPE_INT int
#define CLI_TYPE_FLOAT float
#define CLI_TYPE_BOOL bool
#define CLI_TYPE(kind) CLI_TYPE_##kind

#define CLI_PARSE_STRING cli_parse_string
#define CLI_PARSE_INT cli_parse_int
#define CLI_PARSE_FLOAT cli_parse_float
#define CLI_PARSE_BOOL cli_parse_bool
#define CLI_PARSE(kind) CLI_PARSE_##kind

#define CLI_DEFAULT_STRING(value) {.STRING = value}
#define CLI_DEFAULT_INT(value) {.INT = value}
#define CLI_DEFAULT_FLOAT(value) {.FLOAT = value}
#define CLI_DEFAULT_BOOL(value) {.BOOL = value}
#define CLI_DEFAULT(kind, value) CLI_DEFAULT_##kind(value)

typedef struct {
#define CLI_REQUIRED_FIELD(kind, field, long_name, short_name, description)    \
    CLI_TYPE(kind) field;
#define CLI_OPTIONAL_FIELD(kind, field, long_name, short_name, default_value,  \
                           description)                                        \
    CLI_TYPE(kind) field;
    CLI_ARGS(CLI_REQUIRED_FIELD, CLI_OPTIONAL_FIELD)
#undef CLI_OPTIONAL_FIELD
#undef CLI_REQUIRED_FIELD

#ifdef CLI_CUSTOM_ARGS
    char **argv;
    int argc;
#endif
} cli_t;

typedef enum {
    CLI_KIND_STRING,
    CLI_KIND_INT,
    CLI_KIND_FLOAT,
    CLI_KIND_BOOL,
} cli_kind_t;

typedef enum {
#define CLI_REQUIRED_ID(kind, field, long_name, short_name, description)       \
    CLI_ID_##field,
#define CLI_OPTIONAL_ID(kind, field, long_name, short_name, default_value,     \
                        description)                                           \
    CLI_ID_##field,
    CLI_ARGS(CLI_REQUIRED_ID, CLI_OPTIONAL_ID)
#undef CLI_OPTIONAL_ID
#undef CLI_REQUIRED_ID
    CLI_ID_COUNT,
} cli_id_t;

typedef union {
    strview_t STRING;
    int INT;
    float FLOAT;
    bool BOOL;
} cli_arg_value_t;

typedef struct {
    cli_id_t id;
    cli_kind_t kind;
    strview_t long_name;
    strview_t description;
    char short_name;
    bool required;
    cli_arg_value_t default_value;
} cli_arg_t;

#define CLI_ARG_REQUIRED(arg_kind, field, long_literal, short_ch, desc_literal) \
    {.id = CLI_ID_##field,                                                     \
     .kind = CLI_KIND_##arg_kind,                                              \
     .long_name = STRVIEW_INIT(long_literal),                                  \
     .description = STRVIEW_INIT(desc_literal),                                \
     .short_name = short_ch,                                                   \
     .required = true,                                                         \
     .default_value = {.INT = 0}},
#define CLI_ARG_OPTIONAL(arg_kind, field, long_literal, short_ch, default_expr, \
                         desc_literal)                                         \
    {.id = CLI_ID_##field,                                                     \
     .kind = CLI_KIND_##arg_kind,                                              \
     .long_name = STRVIEW_INIT(long_literal),                                  \
     .description = STRVIEW_INIT(desc_literal),                                \
     .short_name = short_ch,                                                   \
     .required = false,                                                        \
     .default_value = CLI_DEFAULT(arg_kind, default_expr)},

static const cli_arg_t cli_args[] = {
    CLI_ARGS(CLI_ARG_REQUIRED, CLI_ARG_OPTIONAL)
};

#undef CLI_ARG_OPTIONAL
#undef CLI_ARG_REQUIRED

typedef struct {
    int argc;
    char **argv;
    int index;
    cli_t *cli;
    char *error;
    size_t error_size;
    bool seen[CLI_ID_COUNT];
} cli_parser_t;

static void cli_parser_error(cli_parser_t *parser, const char *message,
                             strview_t value) {
    if (parser->error == NULL || parser->error_size == 0) {
        return;
    }

    if (value.data == NULL) {
        snprintf(parser->error, parser->error_size, "%s", message);
        return;
    }

    snprintf(parser->error, parser->error_size, "%s: %.*s", message,
             (int)value.length, value.data);
}

static bool cli_find_long(strview_t name, const cli_arg_t **arg) {
    for (size_t i = 0; i < CLI_ID_COUNT; ++i) {
        if (strview_equals(cli_args[i].long_name, name)) {
            *arg = &cli_args[i];
            return true;
        }
    }

    return false;
}

static const char *cli_kind_name(cli_kind_t kind) {
    switch (kind) {
    case CLI_KIND_STRING:
        return "string";
    case CLI_KIND_INT:
        return "int";
    case CLI_KIND_FLOAT:
        return "float";
    case CLI_KIND_BOOL:
        return "bool";
    }

    return "value";
}

static inline bool cli_arg_has_short_name(const cli_arg_t *arg) {
    return arg->short_name != 0;
}

static bool cli_find_short(char name, const cli_arg_t **arg) {
    for (size_t i = 0; i < CLI_ID_COUNT; ++i) {
        if (cli_arg_has_short_name(&cli_args[i]) &&
            cli_args[i].short_name == name) {
            *arg = &cli_args[i];
            return true;
        }
    }

    return false;
}

static bool cli_to_cstr(cli_parser_t *parser, strview_t value, char *buffer,
                        size_t buffer_size) {
    if (value.length >= buffer_size) {
        cli_parser_error(parser, "value is too long", value);
        return false;
    }

    strview_to_cstr(buffer, buffer_size, value);
    return true;
}

static bool cli_parse_string(cli_parser_t *parser, strview_t value,
                             strview_t *out) {
    (void)parser;

    *out = value;
    return true;
}

static bool cli_parse_int(cli_parser_t *parser, strview_t value, int *out) {
    char buffer[64];
    char *end = NULL;
    long parsed;

    if (!cli_to_cstr(parser, value, buffer, sizeof(buffer))) {
        return false;
    }

    errno = 0;
    parsed = strtol(buffer, &end, 10);
    if (errno != 0 || *end != '\0' || parsed < INT_MIN || parsed > INT_MAX) {
        cli_parser_error(parser, "expected integer", value);
        return false;
    }

    *out = (int)parsed;
    return true;
}

static bool cli_parse_float(cli_parser_t *parser, strview_t value, float *out) {
    char buffer[64];
    char *end = NULL;
    float parsed;

    if (!cli_to_cstr(parser, value, buffer, sizeof(buffer))) {
        return false;
    }

    errno = 0;
    parsed = strtof(buffer, &end);
    if (errno != 0 || *end != '\0') {
        cli_parser_error(parser, "expected float", value);
        return false;
    }

    *out = parsed;
    return true;
}

static bool cli_parse_bool(cli_parser_t *parser, strview_t value, bool *out) {
    if (strview_equals(value, STRVIEW_LIT("true")) ||
        strview_equals(value, STRVIEW_LIT("1")) ||
        strview_equals(value, STRVIEW_LIT("yes"))) {
        *out = true;
        return true;
    }

    if (strview_equals(value, STRVIEW_LIT("false")) ||
        strview_equals(value, STRVIEW_LIT("0")) ||
        strview_equals(value, STRVIEW_LIT("no"))) {
        *out = false;
        return true;
    }

    cli_parser_error(parser, "expected boolean", value);
    return false;
}

static bool cli_arg_needs_value(const cli_arg_t *arg) {
    return arg->kind != CLI_KIND_BOOL;
}

static size_t cli_arg_usage_width(const cli_arg_t *arg) {
    size_t width = 8 + arg->long_name.length;

    if (cli_arg_needs_value(arg)) {
        width += strlen(cli_kind_name(arg->kind)) + 3;
    }

    return width;
}

static void cli_print_spaces(FILE *stream, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        fputc(' ', stream);
    }
}

static void cli_print_default_value(FILE *stream, const cli_arg_t *arg) {
    switch (arg->kind) {
    case CLI_KIND_STRING:
        if (arg->default_value.STRING.data != NULL) {
            fprintf(stream, "%.*s", (int)arg->default_value.STRING.length,
                    arg->default_value.STRING.data);
        }
        break;
    case CLI_KIND_INT:
        fprintf(stream, "%d", arg->default_value.INT);
        break;
    case CLI_KIND_FLOAT:
        fprintf(stream, "%g", arg->default_value.FLOAT);
        break;
    case CLI_KIND_BOOL:
        fprintf(stream, "%s", arg->default_value.BOOL ? "true" : "false");
        break;
    }
}

static void cli_init(cli_t *cli) {
    *cli = (cli_t){0};

#define CLI_REQUIRED_DEFAULT(kind, field, long_name, short_name, description)
#define CLI_OPTIONAL_DEFAULT(kind, field, long_name, short_name,               \
                             default_value, description)                       \
    cli->field = (default_value);
    CLI_ARGS(CLI_REQUIRED_DEFAULT, CLI_OPTIONAL_DEFAULT)
#undef CLI_OPTIONAL_DEFAULT
#undef CLI_REQUIRED_DEFAULT
}

static bool cli_set(cli_parser_t *parser, const cli_arg_t *arg,
                    strview_t value) {
    switch (arg->id) {
#define CLI_REQUIRED_SET(kind, field, long_name, short_name, description)      \
    case CLI_ID_##field:                                                       \
        return CLI_PARSE(kind)(parser, value, &parser->cli->field);
#define CLI_OPTIONAL_SET(kind, field, long_name, short_name, default_value,    \
                         description)                                          \
    case CLI_ID_##field:                                                       \
        return CLI_PARSE(kind)(parser, value, &parser->cli->field);
        CLI_ARGS(CLI_REQUIRED_SET, CLI_OPTIONAL_SET)
#undef CLI_OPTIONAL_SET
#undef CLI_REQUIRED_SET
    case CLI_ID_COUNT:
        break;
    }

    cli_parser_error(parser, "internal parser error", (strview_t){0});
    return false;
}

static bool cli_take_value(cli_parser_t *parser, const cli_arg_t *arg,
                           strview_t inline_value, strview_t *value) {
    if (!cli_arg_needs_value(arg)) {
        *value = inline_value.data == NULL ? STRVIEW_LIT("true") : inline_value;
        return true;
    }

    if (inline_value.data != NULL) {
        *value = inline_value;
        return true;
    }

    if (parser->index + 1 >= parser->argc) {
        cli_parser_error(parser, "missing value for option", arg->long_name);
        return false;
    }

    parser->index += 1;
    *value = strview_from_cstr(parser->argv[parser->index]);
    return true;
}

static bool cli_parse_next(cli_parser_t *parser) {
    const cli_arg_t *arg = NULL;
    strview_t inline_value = {0};
    strview_t token = strview_from_cstr(parser->argv[parser->index]);
    strview_t value = {0};

    if (strview_starts_with(token, STRVIEW_LIT("--"))) {
        strview_t name = strview_slice(token, 2, token.length - 2);
        size_t eq = strview_find_char(name, '=');

        if (eq != STRVIEW_NPOS) {
            inline_value = strview_slice(name, eq + 1, name.length - eq - 1);
            name = strview_slice(name, 0, eq);
        }

        if (!cli_find_long(name, &arg)) {
            cli_parser_error(parser, "unknown option", token);
            return false;
        }
    } else if (strview_starts_with(token, STRVIEW_LIT("-")) &&
               token.length == 2) {
        if (!cli_find_short(token.data[1], &arg)) {
            cli_parser_error(parser, "unknown option", token);
            return false;
        }
    } else {
        cli_parser_error(parser, "unexpected positional argument", token);
        return false;
    }

    if (!cli_take_value(parser, arg, inline_value, &value)) {
        return false;
    }

    if (!cli_set(parser, arg, value)) {
        return false;
    }

    parser->seen[arg->id] = true;
    return true;
}

static bool cli_check_required(cli_parser_t *parser) {
    for (size_t i = 0; i < CLI_ID_COUNT; ++i) {
        if (cli_args[i].required && !parser->seen[cli_args[i].id]) {
            cli_parser_error(parser, "missing required option",
                             cli_args[i].long_name);
            return false;
        }
    }

    return true;
}

static bool cli_parse(int argc, char **argv, cli_t *cli, char *error,
                      size_t error_size) {
    cli_parser_t parser = {
        .argc = argc,
        .argv = argv,
        .index = 1,
        .cli = cli,
        .error = error,
        .error_size = error_size,
        .seen = {0},
    };

    cli_init(cli);

    while (parser.index < parser.argc) {
#ifdef CLI_CUSTOM_ARGS
        strview_t token = strview_from_cstr(parser.argv[parser.index]);
        if (strview_equals(token, STRVIEW_LIT("--"))) {
            cli->argv = parser.argv + parser.index + 1;
            cli->argc = argc - parser.index - 1;
            break;
        }
#endif
        if (!cli_parse_next(&parser)) {
            return false;
        }
        parser.index += 1;
    }

    return cli_check_required(&parser);
}

static void cli_usage(FILE *stream, const char *program) {
    fprintf(stream, "usage: %s", program);

    for (size_t i = 0; i < CLI_ID_COUNT; ++i) {
        const cli_arg_t *arg = &cli_args[i];
        if (!arg->required) {
            continue;
        }
        fprintf(stream, " --%.*s <%s>", (int)arg->long_name.length,
                arg->long_name.data, cli_kind_name(arg->kind));
    }
#ifdef CLI_CUSTOM_ARGS
    fprintf(stream, " [-- args...]");
#endif

    size_t max_option_width = 0;
    for (size_t i = 0; i < CLI_ID_COUNT; ++i) {
        const cli_arg_t *arg = &cli_args[i];
        size_t width = cli_arg_usage_width(arg);
        if (width > max_option_width) {
            max_option_width = width;
        }
    }

    fprintf(stream, "\n\noptions:\n");
    for (size_t i = 0; i < CLI_ID_COUNT; ++i) {
        const cli_arg_t *arg = &cli_args[i];
        size_t width = cli_arg_usage_width(arg);

        if (cli_arg_has_short_name(arg)) {
            fprintf(stream, "  -%c, --%.*s", arg->short_name,
                    (int)arg->long_name.length, arg->long_name.data);
        } else {
            fprintf(stream, "      --%.*s", (int)arg->long_name.length,
                    arg->long_name.data);
        }

        if (cli_arg_needs_value(arg)) {
            fprintf(stream, " <%s>", cli_kind_name(arg->kind));
        }

        if (!strview_is_empty(arg->description) || !arg->required) {
            cli_print_spaces(stream, max_option_width - width + 2);
        }

        if (!strview_is_empty(arg->description)) {
            fprintf(stream, "%.*s", (int)arg->description.length,
                    arg->description.data);
        }

        if (!arg->required) {
            if (!strview_is_empty(arg->description)) {
                fprintf(stream, " ");
            }
            fprintf(stream, "(default: ");
            cli_print_default_value(stream, arg);
            fprintf(stream, ")");
        }

        fprintf(stream, "\n");
    }
}

#undef CLI_PARSE
#undef CLI_PARSE_BOOL
#undef CLI_PARSE_FLOAT
#undef CLI_PARSE_INT
#undef CLI_PARSE_STRING
#undef CLI_DEFAULT
#undef CLI_DEFAULT_BOOL
#undef CLI_DEFAULT_FLOAT
#undef CLI_DEFAULT_INT
#undef CLI_DEFAULT_STRING
#undef CLI_TYPE
#undef CLI_TYPE_BOOL
#undef CLI_TYPE_FLOAT
#undef CLI_TYPE_INT
#undef CLI_TYPE_STRING

#endif
