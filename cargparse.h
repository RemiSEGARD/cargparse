#ifndef CARGPARSE_H
#define CARGPARSE_H

#include <stddef.h>
#include <stdbool.h>

#ifndef CARG_LOCATION
#  error "CARG_LOCATION is not defined"
#endif

typedef char *cstr;
typedef struct
{
    char **strs;
    size_t size;
    size_t capacity;
} cargparse_str_vector;

typedef cargparse_str_vector str_vec;

#define ARGUMENT(NAME, TYPE, DEFAULT_VALUE, ARGS, DESC) TYPE NAME;
#include CARG_LOCATION
#undef ARGUMENT

void cargparse_str_vector_add_str(cargparse_str_vector *vec, char *str);
char *cargparse_str_vector_get(cargparse_str_vector *vec, size_t index);

typedef struct
{
    const char *str;
    size_t size;
} cargparse_str_view;

int cargparse_str_view_eq(const cargparse_str_view *s1,
                          const cargparse_str_view *s2);
cargparse_str_view cargparse_str_view_from_cstr(const char *cstr);
cargparse_str_view cargparse_cstr_split(char **ptr, const char *delim);

typedef int(*cargparse_parse_type_arg_f)(char *, void *);
typedef struct cargparse_arg_map_item
{
    cargparse_str_view name;
    cargparse_parse_type_arg_f parse_function;
    void *data;
    int needs_value;
    struct cargparse_arg_map_item *next;
} cargparse_arg_map_item ;

cargparse_arg_map_item *
cargparse_arg_map_item_get(const cargparse_str_view *arg_name);

typedef struct
{
    cargparse_arg_map_item **items;
    size_t capacity;
} cargparse_arg_map;


typedef enum
{
    CARGPARSE_NO_ERROR,
    CARGPARSE_UNKNOWN_ARG,
    CARGPARSE_WRONG_VALUE_TYPE,
    CARGPARSE_PRINT_HELP,
} cargparse_error;

static inline int cargparse_streq(const char *s1, const char *s2);

int cargparse_register_arg(const cargparse_str_view *arg,
                           cargparse_parse_type_arg_f parse_function,
                           void *data);
void cargparse_print_help(cargparse_error error, const char *wrong_arg);

int cargparse_parse_str_vec_arg(char *arg, void *data);
int cargparse_parse_bool_arg(char *arg, void *data);
int cargparse_parse_cstr_arg(char *arg, void *data);
int cargparse_parse_int_arg(char *arg, void *data);

int cargparse_setup_args(char *usage_str);

char *cargparse_shift_args(int *argc, char **argv[]);
cargparse_str_view cargparse_get_arg_name(const char *str, int *is_equal_arg);
int cargparse_parse_argument(char *arg, int *argc, char **argv[]);
int cargparse_parse_args(int *argc, char **argv[]);

#ifndef CARGPARSE_IMPLEMENTATION

#else /* CARGPARSE_IMPLEMENTATION */

#undef ARGUMENT
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

#include <stdlib.h>
#include <string.h>

/**
 * @brief shorthand for strcmp(s1, s2) == 0
 */
static inline int cargparse_streq(const char *s1, const char *s2)
{
    return strcmp(s1, s2) == 0;
}

void cargparse_str_vector_add_str(cargparse_str_vector *vec, char *str)
{
    if (vec->capacity == 0 && vec->strs == NULL)
    {
        vec->strs = calloc(8, sizeof(char *));
        vec->capacity = 8;
    }
    if (vec->size == vec->capacity)
    {
        vec->strs = realloc(vec->strs, vec->capacity * 2 * sizeof(char *));
        vec->capacity *= 2;
    }
    vec->strs[vec->size++] = str;
}

char *cargparse_str_vector_get(cargparse_str_vector *vec, size_t index)
{
    return vec->strs[index];
}

int cargparse_str_view_eq(const cargparse_str_view *s1,
                          const cargparse_str_view *s2)
{
    if (s1->size != s2->size)
        return 0;
    return strncmp(s1->str, s2->str, s1->size) == 0;
}

cargparse_str_view cargparse_str_view_from_cstr(const char *cstr)
{
    cargparse_str_view res = { .str = cstr, .size = strlen(cstr) };
    return res;
}

cargparse_str_view cargparse_cstr_split(char **ptr, const char *delim)
{
    cargparse_str_view sv = { .str = NULL, .size = 0 };
    while (**ptr != '\0' && strchr(delim, **ptr) != NULL)
        *ptr += 1;
    if (**ptr == '\0')
        return sv;

    sv.str = *ptr;
    while (**ptr != '\0' && strchr(delim, **ptr) == NULL)
    {
        *ptr += 1;
        sv.size += 1;
    }
    return sv;
}

cargparse_arg_map cargparse_args_data = { 0 };

static size_t cargparse_hash_str_view(const cargparse_str_view *sv)
{
    size_t hash = 0;
    for (size_t i = 0; i < sv->size; i++)
        hash = (hash ^ sv->str[i]) + sv->str[i];
    return hash;
}

int cargparse_register_arg(const cargparse_str_view *arg,
                           cargparse_parse_type_arg_f parse_function,
                           void *data)
{
    cargparse_arg_map_item *item = calloc(1, sizeof(cargparse_arg_map_item));
    item->name = *arg;
    item->parse_function = parse_function;
    item->needs_value = parse_function != cargparse_parse_bool_arg;
    item->data = data;
    size_t h = cargparse_hash_str_view(arg) % cargparse_args_data.capacity;
    if (cargparse_args_data.items[h] == NULL)
    {
        cargparse_args_data.items[h] = item;
        return 0;
    }
    item->next = cargparse_args_data.items[h];
    cargparse_args_data.items[h] = item;
    return 0;
}

char *cargparse_usage_string = NULL;

int cargparse_setup_args(char *usage_str)
{
    cargparse_usage_string = usage_str;
    cargparse_args_data.items = calloc(10, sizeof(cargparse_arg_map_item));
    cargparse_args_data.capacity = 10;

#define ARGUMENT(NAME, TYPE, DEFAULT_VALUE, ARGS, DESC)                        \
    do {                                                                       \
        NAME = DEFAULT_VALUE;                                                  \
        char *str = ARGS;                                                      \
        cargparse_str_view sv = { 0 };                                         \
        while ((sv = cargparse_cstr_split(&str, "|")).str != NULL)             \
            cargparse_register_arg(&sv, cargparse_parse_##TYPE##_arg, &NAME);  \
    } while (0);
#include CARG_LOCATION
#undef ARGUMENT
    return 0;
}

static char *error_string[] =
{
    [CARGPARSE_UNKNOWN_ARG] = "Unknown argument",
    [CARGPARSE_WRONG_VALUE_TYPE] = "Wrong value type for"
};

void cargparse_print_help(cargparse_error error, const char *wrong_arg)
{
    static char *args_args[] = {
#define ARGUMENT(NAME, TYPE, DEFAULT_VALUE, ARGS, DESC) ARGS,
        "help",
#include CARG_LOCATION
#undef ARGUMENT
    };
    static char *args_types[] = {
#define ARGUMENT(NAME, TYPE, DEFAULT_VALUE, ARGS, DESC) #TYPE,
ARGUMENT(,,,,)
    ""
#include CARG_LOCATION
#undef ARGUMENT
    };
    static char *args_descs[] = {
#define ARGUMENT(NAME, TYPE, DEFAULT_VALUE, ARGS, DESC) DESC,
        "prints this message",
#include CARG_LOCATION
#undef ARGUMENT
    };

    if (wrong_arg != NULL && error != CARGPARSE_PRINT_HELP)
        printf("%s `%s'\n", error_string[error], wrong_arg);

    if (cargparse_usage_string != NULL)
        printf("USAGE: %s\n", cargparse_usage_string);

    puts("OPTIONS:");
    for (size_t i = 0; i < sizeof(args_args) / sizeof(*args_args); i++)
    {
        int printed = 0;
        cargparse_str_view sv = { 0 };
        char *arg = args_args[i];
        printed += printf(" ");
        while ((sv = cargparse_cstr_split(&arg, "|")).str != NULL)
        {
            if (printed > 1)
                printed += printf(",");
            if (sv.size == 1)
                printed += printf("-");
            else
                printed += printf("--");
            printed += fwrite(sv.str, sizeof(char), sv.size, stdout);
        }
        printf("%*s ", 30 - printed, "");
        if (args_types[i][0] != '\0')
            printf("(%s) ", args_types[i]);
        printf("%s\n", args_descs[i]);
    }

    exit(error == CARGPARSE_PRINT_HELP ? 0 : 1);
}

int cargparse_parse_str_vec_arg(char *arg, void *data)
{
    cargparse_str_vector vector = { 0 };
    char *str = strtok(arg, ",");
    cargparse_str_vector_add_str(&vector, str);
    while ((str = strtok(NULL, ",")) != NULL)
        cargparse_str_vector_add_str(&vector, str);
    cargparse_str_vector *dest = data;
    *dest = vector;
    return CARGPARSE_NO_ERROR;
}

int cargparse_parse_bool_arg(__attribute((unused))char *arg, void *data)
{
    bool *dest = data;
    *dest = !*dest;
    return CARGPARSE_NO_ERROR;
}

int cargparse_parse_cstr_arg(char *arg, void *data)
{
    const char **dest = data;
    *dest = arg;
    return CARGPARSE_NO_ERROR;
}

int cargparse_parse_int_arg(char *arg, void *data)
{
    int *dest = data;
    char *endptr = NULL;
    *dest = (int)strtol(arg, &endptr, 10);
    return *endptr == '\0' ? CARGPARSE_NO_ERROR : CARGPARSE_WRONG_VALUE_TYPE;
}

char *cargparse_shift_args(int *argc, char **argv[])
{
    if (*argc == 0)
        return NULL;
    char *res = (*argv)[0];
    *argc -= 1;
    *argv += 1;
    return res;
}

cargparse_arg_map_item *
cargparse_arg_map_item_get(const cargparse_str_view *arg_name) {
    size_t h = cargparse_hash_str_view(arg_name) % cargparse_args_data.capacity;
    if (cargparse_args_data.items[h] == NULL)
        return NULL;
    cargparse_arg_map_item *item = cargparse_args_data.items[h];
    while (item != NULL && !cargparse_str_view_eq(arg_name, &item->name))
        item = item->next;
    return item;
}

cargparse_str_view cargparse_get_arg_name(const char *str, int *is_equal_arg)
{
    cargparse_str_view res = { .str = str + 1, .size = 0};
    if (str[0] != '-')
        return res;
    if (str[1] == '-')
        res.str += 1;
    while (res.str[res.size] != '\0' && res.str[res.size] != '=')
        res.size += 1;

    *is_equal_arg = res.str[res.size] == '=';
    return res;
}

int cargparse_parse_small_arg(char *arg, int *argc, char **argv[])
{
    cargparse_str_view arg_name = { .str = arg + 1, .size = 1 };
    while (arg_name.str[0] != '\0')
    {
        cargparse_arg_map_item *arg_item =
                cargparse_arg_map_item_get(&arg_name);
        if (arg_item == NULL)
            return CARGPARSE_UNKNOWN_ARG;
        if (arg_item->needs_value)
            arg = cargparse_shift_args(argc, argv);
        cargparse_error error = arg_item->parse_function(arg, arg_item->data);
        if (error)
            return error;
        arg_name.str += 1;
    }
    return CARGPARSE_NO_ERROR;
}

int cargparse_parse_argument(char *arg, int *argc, char **argv[])
{
    if (arg[1] != '-')
        return cargparse_parse_small_arg(arg, argc, argv);
    int is_equal_arg = 0;
    cargparse_str_view arg_name =
        cargparse_get_arg_name(arg, &is_equal_arg);
    if (strncmp(arg_name.str, "help", arg_name.size) == 0)
        return CARGPARSE_PRINT_HELP;
    cargparse_arg_map_item *arg_item = cargparse_arg_map_item_get(&arg_name);
    if (arg_item == NULL)
        return CARGPARSE_UNKNOWN_ARG;

    if (!is_equal_arg && arg_item->needs_value)
        arg = cargparse_shift_args(argc, argv);
    else if (is_equal_arg)
        arg = strchr(arg, '=') + 1;

    return arg_item->parse_function(arg, arg_item->data);
}

int cargparse_parse_args(int *argc, char **argv[])
{
    char **argv_begin = *argv;
    int final_argc = 0;
    cargparse_shift_args(argc, argv);
    char *arg = NULL;
    while ((arg = cargparse_shift_args(argc, argv)) != NULL)
    {
        if (cargparse_streq(arg, "--"))
            break;
        if (arg[0] != '-')
        {
            argv_begin[final_argc++] = arg;
            continue;
        }

        cargparse_error error = cargparse_parse_argument(arg, argc, argv);
        if (error)
            cargparse_print_help(error, arg);
    }
    while ((arg = cargparse_shift_args(argc, argv)) != NULL)
        argv_begin[final_argc++] = arg;
    argv_begin[final_argc] = NULL;
    *argc = final_argc;
    *argv = argv_begin;
    return 0;
}

#endif /* ! CARGPARSE_IMPLEMENTATION */
#endif /* ! CARGPARSE_H */
