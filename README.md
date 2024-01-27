# CArgParse

C header library to handle CLI argument parsing

## How to use

- Create a file (`cargparse.h.in`) and define your arguments using the ARGUMENT macro.
- Define a `CARG_LOCATION` string macro equal to the location of the `cargparse.h.in` file (absolute paths are easier the manage).
- In your `main` function call the following functions:
```
cargparse_setup_args();
cargparse_parse_args(int *argc, char **argv[]);
```
- You can then access the values inside the arguments using the variables declared by the ARGUMENT macro
