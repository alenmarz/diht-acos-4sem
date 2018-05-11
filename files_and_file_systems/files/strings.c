#include <stdio.h>

int is_normal(char symbol) {
    return symbol < 127 && symbol > 32 || symbol == ' ' || symbol == '\t';
}

void report_stream(char *filename) {
    FILE *file = filename ? fopen(filename, "rb") : stdin;
    if (file == NULL) {
        printf("File %s ", filename, " could not be opened\n");
        return;
    }

    char current_symbol;
    char buff[4] = "\0\0\0\0";
    int normal_symbols_count = 0;

    do {
        current_symbol = fgetc(file);

        if (is_normal(current_symbol)) {

            if (normal_symbols_count < 3) {
                buff[normal_symbols_count] = current_symbol;
            }

            if (normal_symbols_count == 3) {
                printf("%s", buff);
            }

            if (normal_symbols_count >= 3) {
                printf("%c", current_symbol);
            }

            normal_symbols_count++;
        } else {
            if (normal_symbols_count >= 4) {
                printf("\n");
            }
            normal_symbols_count = 0;
        }

    } while (!feof(file));

    fclose(file);
}

int main(int argc, char **argv) {
    if (argc == 1) report_stream(NULL);
    for (int i = 1; i < argc; i++) report_stream(argv[i]);

    return 0;
}
