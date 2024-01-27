#include <stdlib.h>
#define CBUILD_IMPLEMENTATION
#include "cbuild.h"

int main(int argc, char *argv[])
{
    CBUILD_REBUILD_YOURSELF(argc, argv);

    static cbuild_target example = CBUILD_TARGET("example",
            CBUILD_MAKE_FILE_SOURCE("example.c"),
            );

    cbuild_str_builder sb = { 0 };
    char *pwd = getenv("PWD");
    cbuild_str_builder_append_cstr(&sb, "-DCARG_LOCATION=\"");
    cbuild_str_builder_append_cstr(&sb, pwd);
    cbuild_str_builder_append_cstr(&sb, "/example_args.h.in\"");
    char *macro_path_to_cargs = cbuild_str_builder_to_cstr(&sb);

    cbuild_str_vector_add_strs(&example.command, "cc", "-Wall", "-Wextra", "-g", macro_path_to_cargs);

    if (cbuild_build_target(&example, NULL))
    {
        cbuild_log(CBUILD_ERROR, "Could not build example");
        return 1;
    }

    cbuild_command exec = { 0 };
    cbuild_command_add_args(&exec, "./example", "first", "-a", "2", "second",
                            "-b", "lala", "third", "forth");
    return cbuild_command_exec_sync(&exec);
}
