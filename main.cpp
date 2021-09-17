#include <cstdlib>
#include <ctype.h>
#include <stdio.h>

static inline bool is_letter(char symbol) {
    return !isspace(symbol) && !ispunct(symbol);
}

int compare_lexicographically(char* str1, char* str2, int direction) {
    while (*str1 != '\0' && *str2 != '\0')
        if (*str1 == *str2) {
            str1 += direction;
            str2 += direction;
        } else if (!is_letter(*str1)) {
            str1 += direction;
        } else if (!is_letter(*str2)) {
            str2 += direction;
        } else break;

    return str1[0] > str2[0];
}

struct line {
    char* beg;
    char* end;
};

int compare_lines_forward(const void* line1_void_ptr, const void* line2_void_ptr) {

    const line* line1 = (const line*) line1_void_ptr;
    const line* line2 = (const line*) line2_void_ptr;

    return compare_lexicographically(line1->beg, line2->beg, +1);
}

int compare_lines_backward(const void* line1_void_ptr, const void* line2_void_ptr) {

    const line* line1 = (const line*) line1_void_ptr;
    const line* line2 = (const line*) line2_void_ptr;

    return compare_lexicographically(line1->end, line2->end, -1);
}

char* read_file(const char* file_name, long* file_size) {
    FILE* file = fopen(file_name, "r");

    fseek(file, 0, SEEK_END);
    *file_size = ftell(file);
    rewind(file);

    setvbuf(stdout, NULL, _IONBF, 0);

    char *buffer = (char*) malloc((size_t) *file_size + 2);
    *file_size = (long) fread(buffer, 1, (size_t) *file_size, file);

    fclose(file);
    file = NULL;

    return buffer;
}

int count_new_lines(const char* buffer, const long file_size) {
    int new_line_count = 1;
    for (int i = 0; i <= file_size; ++ i) {
        char symbol = buffer[i];

        if (symbol == '\n')
            ++ new_line_count;
    }

    return new_line_count;
}

line* split_in_lines_with_terminator(char* buffer, const long file_size, int* number_of_lines) {
    int new_line_count = count_new_lines(buffer, file_size);

    line* lines = (line*) malloc((size_t) new_line_count * sizeof(line));
    int line_index = 0;

    lines[line_index].beg = buffer;
    for (int i = 0; i <= file_size; ++ i) {
        char symbol = buffer[i];

        if (symbol == '\n') {
            // Replace \n with string end (null-terminator)
            buffer[i] = '\0';

            if (i > 0)
                // Do not start with newline
                lines[line_index].end = buffer + (i - 1);
            else 
                lines[line_index].end = buffer + i;

            if (i < file_size - 1)
                // Do not end with newline
                lines[++ line_index].beg = buffer + (i + 1);
        }
    }

    *number_of_lines = line_index;
    return lines;
}

void fprint_lines(FILE* output, const line* lines, const int number_of_lines) {
    for (int i = 0; i < number_of_lines; ++ i) {
        line current = lines[i];

        fputs(current.beg, output);
        fputs("\n", output);
    }
}

void fsort_and_print_lines(FILE *output, line *lines, const int number_of_lines,
                           __compar_fn_t compare) {

    qsort(lines, (size_t) number_of_lines, sizeof(line), compare);
  fprint_lines(output, lines, number_of_lines);
}

void concatenate_separated_lines(char* buffer, const long file_size) {
    for (int i = 0; i <= file_size; ++ i)
        if (buffer[i] == '\0')
            buffer[i] = '\n';
}

int main(void) {
    long file_size = 0;
    char* buffer = read_file("onegin.txt", &file_size);

    int number_of_lines = 0;
    line* lines = split_in_lines_with_terminator(buffer, file_size, &number_of_lines);

    FILE* output = fopen("onegin-out.txt", "w");

    fsort_and_print_lines(output, lines, number_of_lines, compare_lines_forward);

    fsort_and_print_lines(output, lines, number_of_lines, compare_lines_backward);

    concatenate_separated_lines(buffer, file_size);
    fputs(buffer, output);

    // Free everything
    fclose(output);
    output = NULL;

    free(lines);
    lines = NULL;

    free(buffer);
    lines = NULL;
}
