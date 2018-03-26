#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Definitions block **********************************************************/

#define TRUE  1
#define true  1
#define FALSE 0
#define false 0
#define WORD_LIST_SIZE_MAX 10000000

typedef int boolean;

typedef struct {
    size_t allocated_size;
    size_t word_size;
    char *word_content;
} word_t;



/* Global varias block ********************************************************/

FILE *read_stream;

unsigned long ampersand_count        = 0;
unsigned long double_ampersand_count = 0;
unsigned long pipe_count             = 0;
unsigned long double_pipe_count      = 0;
unsigned long semicolon_count        = 0;



/* Functions block ************************************************************/

void bad_input()
{
    printf("Unprocessable input found (probably unvoiced quotes)\n");
    exit(EXIT_FAILURE);
}

void out_of_memory()
{
    printf("Can't allocate additional memory (probably input is too long)\n");
    exit(EXIT_FAILURE);
}

void allocate_word(word_t *word)
{
    word -> allocated_size = 16;
    word -> word_content = (char *)calloc(word -> allocated_size, sizeof(char));
    if (word -> word_content == NULL) out_of_memory();
}

void add_char_to_word(char char_to_add, word_t *word)
{
    if (word -> word_size == word -> allocated_size) {
        size_t realloc_size = 2 * word -> allocated_size;
        word -> word_content =
          (char *)realloc(word -> word_content, realloc_size);
        if (word -> word_content == NULL) out_of_memory();
        word -> allocated_size = realloc_size;
    }
    word -> word_content[word -> word_size] = char_to_add;
    (word -> word_size)++;
}

boolean check_if_char_is_separator_and_increment_counts(char current_char)
{
    if (current_char == EOF) return TRUE;
    if (isspace(current_char)) return TRUE;

    if (current_char == '&') {
        char next_char = getc(read_stream);
        if (next_char == '&')
            double_ampersand_count++;
        else {
            ampersand_count++;
            ungetc(next_char, read_stream);
        }
        return TRUE;
    }

    if (current_char == '|') {
        char next_char = getc(read_stream);
        if (next_char == '|')
            double_pipe_count++;
        else {
            pipe_count++;
            ungetc(next_char, read_stream);
        }
        return TRUE;
    }

    if (current_char == ';') {
        semicolon_count++;
        return TRUE;
    }

    return FALSE;
}

boolean toggle_qotation(char current_char, char *quotation_flag)
{
    if (current_char != '"' && current_char != '\'') return FALSE;
    if (*quotation_flag) {
        if (*quotation_flag != current_char) return FALSE;
        *quotation_flag = 0;
        return TRUE;
    } else {
        *quotation_flag = current_char;
        return TRUE;
    }
}

boolean should_break_word_reading(char current_char, char quotation_flag)
{
    if (quotation_flag && current_char == EOF) bad_input();
    if (quotation_flag) return FALSE;
    return check_if_char_is_separator_and_increment_counts(current_char);
}

void read_word(word_t *word)
{
    char quotation_flag = 0;
    char current_char;
    while (TRUE) {
        current_char = getc(read_stream);
        if (toggle_qotation(current_char, &quotation_flag)) continue;
        if (should_break_word_reading(current_char, quotation_flag)) break;
        add_char_to_word(current_char, word);
    }

    add_char_to_word('\0', word);
}

word_t *word_list;
size_t word_list_size = WORD_LIST_SIZE_MAX;

void make_word_list()
{
    word_list = (word_t *)calloc(word_list_size, sizeof(word_t));
    if (word_list == NULL) out_of_memory();
    size_t current_word_index = 0;

    char current_char;
    while (TRUE) {
        current_char = getc(read_stream);
        if (current_char == EOF) break;
        if (check_if_char_is_separator_and_increment_counts(current_char))
          continue;
        ungetc(current_char, read_stream);
        if (current_word_index == word_list_size) out_of_memory();
        allocate_word(word_list + current_word_index);
        read_word(word_list + current_word_index);
        current_word_index++;
    }

    for (unsigned int index = 0; index < ampersand_count; index++) {
        if (current_word_index == word_list_size) out_of_memory();
        allocate_word(word_list + current_word_index);
        sprintf(word_list[current_word_index].word_content, "&");
        current_word_index++;
    }
    for (unsigned int index = 0; index < double_ampersand_count; index++) {
        if (current_word_index == word_list_size) out_of_memory();
        allocate_word(word_list + current_word_index);
        sprintf(word_list[current_word_index].word_content, "&&");
        current_word_index++;
    }
    for (unsigned int index = 0; index < pipe_count; index++) {
        if (current_word_index == word_list_size) out_of_memory();
        allocate_word(word_list + current_word_index);
        sprintf(word_list[current_word_index].word_content, "|");
        current_word_index++;
    }
    for (unsigned int index = 0; index < double_pipe_count; index++) {
        if (current_word_index == word_list_size) out_of_memory();
        allocate_word(word_list + current_word_index);
        sprintf(word_list[current_word_index].word_content, "||");
        current_word_index++;
    }
    for (unsigned int index = 0; index < semicolon_count; index++) {
        if (current_word_index == word_list_size) out_of_memory();
        allocate_word(word_list + current_word_index);
        sprintf(word_list[current_word_index].word_content, ";");
        current_word_index++;
    }

    word_list_size = current_word_index;
}

int word_cmp(const void *word1, const void *word2)
{
    return strcmp(
        ((word_t *)word1) -> word_content,
        ((word_t *)word2) -> word_content
    );
}

int main(void)
{
    read_stream = stdin;
    make_word_list();
    qsort(word_list, word_list_size, sizeof(word_t), word_cmp);
    for (size_t index = 0; index < word_list_size; index++)
        printf("\"%s\"\n", word_list[index].word_content);
    return 0;
}
