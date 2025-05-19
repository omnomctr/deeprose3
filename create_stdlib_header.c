#include <stdio.h>

int main(void)
{
    printf(
        "#ifndef STDLIB_HEADER__\n"
        "#define STDLIB_HEADER__\n"
        "\n"
        "/* this code was generated using the \"script\" create_stdlib_header.c */\n"
        "/* look at programs/stdlib.deeprose for original program */\n"
        "\n"
        "const char stdlib[] = {");


    int c;
    while ((c = getchar()) != EOF) {
        if (c == ';') {
            while ((c = getchar()) != '\n' && c != EOF) {}
            continue;
        }
        printf("%d, ", (char)c);
    }

    printf(
        "};\n"
        "\n"
        "#endif\n");

    return 0;
}
