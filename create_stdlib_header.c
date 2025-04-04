#include <stdio.h>

int main(void)
{
    printf(
        "#ifndef STDLIB_HEADER__\n"
        "#define STDLIB_HEADER__\n"
        "\n"
        "/* this code was generated using the \"script\" create_stdlib_header.c */\n"
        "\n"
        "const char * const stdlib = {\"");


    int c;
    while ((c = getchar()) != EOF) {
        switch (c) {
            case '\n':
                putchar('\\');
                putchar('n');
                continue;
            case '\\': case '"': case '\'':
                putchar('\\');
                break;
        }
        putchar(c);
    }

    printf(
        "\"};\n"
        "\n"
        "#endif\n");

    return 0;
}
