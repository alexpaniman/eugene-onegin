#include <cstdlib>
#include <ctype.h>
#include <stdio.h>
#include <locale.h>
#include <cwchar>
#include <cwctype>
#include <sys/stat.h>

int compare_lexicographically(const wchar_t *const str1,
                              const wchar_t *const str2,
                              int direction) {

    int ind1 = 0, ind2 = 0;

    while (ind1 >= 0 && ind2 >= 0) {
        wchar_t ch1 = str1[ind1], ch2 = str2[ind2];

        if (ch1 == L'\0' || ch2 == L'\0')
            break;

        if (towlower(ch1) == towlower(ch2)) {
            ind1 += direction;
            ind2 += direction;
        }
        else if (!iswalpha(ch1))
            ind1 += direction;
        else if (!iswalpha(ch2))
            ind2 += direction;
        else break;
    }

    wchar_t downcased1 = towlower(str1[ind1]),
            downcased2 = towlower(str2[ind2]);

    if (downcased1 == downcased2)
        return isupper(str1[ind1]);

    return downcased1 > downcased2;
}

struct line {
    const wchar_t* beg;
    const wchar_t* end;
};

int compare_lines_forward(const void* const line1_void_ptr,
                          const void* const line2_void_ptr) {

    const line* line1 = (const line*) line1_void_ptr;
    const line* line2 = (const line*) line2_void_ptr;

    return compare_lexicographically(line1->beg, line2->beg, +1 /* Go forward */);
}

int compare_lines_backward(const void* const line1_void_ptr,
                           const void* const line2_void_ptr) {

    const line* const line1 = (const line*) line1_void_ptr;
    const line* const line2 = (const line*) line2_void_ptr;

    return compare_lexicographically(line1->end, line2->end, -1 /* Go backward */);
}

// Return file size in bytes
ssize_t get_file_size(FILE* const file) {
    int fd = fileno(file);

    if (fd == -1) {
        perror("Error getting file descriptor");
        return -1;
    }

    struct stat file_stats;
    int fstat_return_code = fstat(fd, &file_stats);

    if (fstat_return_code == -1) {
        perror("Error getting file size");
        return -1;
    }

    return (ssize_t) file_stats.st_size;
}

wchar_t* read_file(const char* const file_name) {
    FILE* input_file = fopen(file_name, "r");

    if (input_file == NULL) {
        perror("Error opening file");
        return NULL;
    }

    const size_t size_bytes = get_file_size(input_file);

    // Allocates wchar_t for every byte in the file, it's likely more
    // space than required, but it's always enough.

    wchar_t* buffer = (wchar_t*)
        calloc(sizeof *buffer, size_bytes + 1 /* for '\0's */);

    if (buffer == NULL) {
        perror("Error allocating memory to store symbols");
        return NULL;
    }

    wchar_t* eof = buffer;
    while (fgetws_unlocked(eof, (int) size_bytes + 1, input_file) != NULL) {

        /* ┌─  Previous Line  ─┐┌────  Advance EOF Here  ────┐
         * ↓                   ↓↑ ←─────  From Here          ↓
         * ...CCCCCCCCCCCCCCCCCNCCCCCCCCCCCCCCCCCCCCCCCCCCCCN0
         *     [line chars]    ↑        [line chars]        ↑↑
         * ... ←─────────────  LF   ────────────────────────┘│
         * ... ←─────────────  EOF  ─────────────────────────┘ */

        while (*eof != L'\0')
            ++ eof;
    }

    fclose(input_file), input_file = NULL;
    return buffer;
}

int count_new_lines(const wchar_t* buffer) {
    int new_line_count = 1;

    wchar_t symbol = L'\0';
    while ((symbol = *buffer++) != L'\0')
        if (symbol == '\n')
            ++ new_line_count;

    return new_line_count;
}

line* split_in_lines_with_terminator(wchar_t* const buffer, size_t* const number_of_lines) {
    int new_line_count = count_new_lines(buffer);

    line* lines = (line*) calloc(new_line_count, sizeof(line));
    if (lines == NULL) {
        perror("Error allocating memory to store lines");
        return NULL;
    }

    size_t line_index = 0;

    lines[line_index].beg = buffer;
    for (int i = 0; buffer[i] != L'\0'; ++ i) {
        wchar_t symbol = buffer[i];

        if (symbol == '\n') {
            // Replace \n with string end (null-terminator)
            buffer[i] = '\0';

            if (i > 0)
                lines[line_index].end = buffer + (i - 1);
            else 
                // If first line is empty, close it
                lines[line_index].end = buffer;

            // Skip LF in the end
            if (buffer[i + 1] != L'\0')
                lines[++ line_index].beg = buffer + (i + 1);
        }
    }

    *number_of_lines = line_index;
    return lines;
}

void fprint_lines(FILE* output, const line* lines, const size_t number_of_lines) {
    for (size_t i = 0; i < number_of_lines; ++ i) {
        line current = lines[i];

        fputws(current.beg, output);
        fputws(L"\n", output);
    }
}

void fsort_and_print_lines(FILE *output, line *lines, const size_t number_of_lines,
                           __compar_fn_t compare) {

    qsort(lines, number_of_lines, sizeof(line), compare);
    fprint_lines(output, lines, number_of_lines);
}

void concatenate_separated_lines(wchar_t* buffer, const size_t number_of_lines) {
    for (size_t i = 0, line_num = 1; line_num < number_of_lines; ++ i)
        if (buffer[i] == '\0') {
            buffer[i] = '\n';
            ++ line_num;
        }
}

int main(void) {
    setlocale(LC_ALL, "");

    wchar_t* buffer = read_file("onegin.txt");

    size_t number_of_lines = 0;
    line* lines = split_in_lines_with_terminator(buffer, &number_of_lines);

    FILE* output = fopen("onegin-out.txt", "w");

    fsort_and_print_lines(output, lines, number_of_lines, compare_lines_forward);

    fsort_and_print_lines(output, lines, number_of_lines, compare_lines_backward);

    concatenate_separated_lines(buffer, number_of_lines);
    fputws(buffer, output);

    // Close output file
    fclose(output), output = NULL;

    // Free everything
    free(lines) , lines  = NULL;
    free(buffer), buffer = NULL;
}
