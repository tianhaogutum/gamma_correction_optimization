#include "readppm.h"

// for all functions here, if fd is zero, then it's undefined behaviour
// status machine for magic number part, three functions for three possible status
static void magic_number_s0(FILE *fd);
static void magic_number_s1(FILE *fd);
static void magic_number_s2(FILE *fd);
// status machine for width, height and maxval part, two functions for two possible status
static struct digits_chain *getsize_s0(FILE *fd);                                                                // return the start of a digits chain
static void getsize_s1(FILE *fd, struct digits_chain *start);                                                    // append the following digits to digits chain
static size_t get_number_from_digits(struct digits_chain *start);                                                // get number and free all allocated space for digits chain
static void enter_and_exit_comment_with_errorfree(FILE *fd, struct digits_chain *start);                         // if program is exiting with error in a comment, then the allocated memory should be freed, and the file should be closed
static void free_from_start(struct digits_chain *start);                                                         // helper function to release a single linked list from head
static void exit_failure_with_errmessage_and_release(FILE *fd, struct digits_chain *start, const char *err_msg); // close input file and release allocated memory, if start is NULL, then release no memory

struct digits_chain
{
    char digit;
    struct digits_chain *next;
};

uint8_t *readppm(const char *input_file, size_t *width, size_t *height)
{
    FILE *fd = fopen(input_file, "r");
    if (!fd)
    {
        fprintf(stderr, "%s", "Cannot open your input file. Please check your input file.\n");
        exit(EXIT_FAILURE);
    }
    magic_number_s0(fd);
    magic_number_s1(fd);
    magic_number_s2(fd);
    struct digits_chain *start_of_width = getsize_s0(fd);
    getsize_s1(fd, start_of_width);
    *width = get_number_from_digits(start_of_width);
    struct digits_chain *start_of_height = getsize_s0(fd);
    getsize_s1(fd, start_of_height);
    *height = get_number_from_digits(start_of_height);
    struct digits_chain *start_of_maxval = getsize_s0(fd);
    getsize_s1(fd, start_of_maxval);
    size_t maxval = get_number_from_digits(start_of_maxval);
    if (maxval != 255)
    {
        fprintf(stderr, "This program only accept 24 bpp pictures, which means the maxval of your picture has to be 255. However, the maxval in the given picture is %lu.\n", maxval);
        fclose(fd);
        exit(EXIT_FAILURE);
    }
    uint8_t *value_of_pixels = malloc((*width) * (*height) * 3 + 1);
    if (!value_of_pixels)
    {
        exit_failure_with_errmessage_and_release(fd, NULL, "Can not allocate space for input pixels.\n");
    }
    size_t success_read = 2;
    for (size_t i = 0; i < (* height); ++i)
    {
        success_read = fread(value_of_pixels + i * (*width) * 3, (*width) * 3, 1, fd);
        if (!success_read)
        {
            fprintf(stderr, "%s", "Read pixel values of input file failed. Is your input file deprecated?\n");
            fclose(fd);
            free(value_of_pixels);
            exit(EXIT_FAILURE);
        }
    }
    printf("width is %lu\nheight is %lu\n", *width, *height);
    fclose(fd);
    return value_of_pixels;
}

static void magic_number_s0(FILE *fd)
{
    while (true)
    {
        switch (fgetc(fd))
        {
        case 'P':
            return;
        case '#':
            enter_and_exit_comment_with_errorfree(fd, NULL);
            break;
        default:
            exit_failure_with_errmessage_and_release(fd, NULL, "Magic number not starting with P. Please check your input file.\n");
        }
    }
}

static void magic_number_s1(FILE *fd)
{
    while (true)
    {
        switch (fgetc(fd))
        {
        case '6':
            return;
        case '#':
            enter_and_exit_comment_with_errorfree(fd, NULL);
            break;
        default:
            exit_failure_with_errmessage_and_release(fd, NULL, "Second digit of Magic number is not 6. Please check your input file.\n");
        }
    }
}

static void magic_number_s2(FILE *fd)
{
    int tmp = -1;
    while (true)
    {
        tmp = fgetc(fd);
        if (tmp == '#')
        {
            enter_and_exit_comment_with_errorfree(fd, NULL);
        }
        else if (isspace(tmp))
        {
            return;
        }
        else
        {
            exit_failure_with_errmessage_and_release(fd, NULL, "Magic number has more than two digits. Please check your input file.\n");
        }
    }
}

static void enter_and_exit_comment_with_errorfree(FILE *fd, struct digits_chain *start)
{
    int tmp = -1;
    while (true)
    {
        tmp = fgetc(fd);
        if (tmp == 10 || tmp == 13)
        {
            break;
        }
        if (tmp == EOF)
        {
            exit_failure_with_errmessage_and_release(fd, start, "It seems that your input file ends in a comment. This is weird. Please check your input file.\n");
        }
    }
}

static struct digits_chain *getsize_s0(FILE *fd)
{
    while (true)
    {
        int tmp = fgetc(fd);
        if (isspace(tmp))
        {
            continue;
        }
        else if (tmp == '#')
        {
            enter_and_exit_comment_with_errorfree(fd, NULL);
        }
        else if (tmp >= 48 && tmp <= 57)
        {
            struct digits_chain *start = malloc(sizeof(struct digits_chain));
            if (start == NULL)
            {
                exit_failure_with_errmessage_and_release(fd, NULL, "space allocation failed.\n");
            }
            start->digit = tmp;
            start->next = NULL;
            return start;
        }
        else
        {
            exit_failure_with_errmessage_and_release(fd, NULL, "The width or height or maxval info in your file is not starting with a digit. Please check your input file.\n");
        }
    }
}

static void getsize_s1(FILE *fd, struct digits_chain *start)
{
    struct digits_chain *current = start;
    while (true)
    {
        int tmp = fgetc(fd);
        if (isspace(tmp))
        {
            return;
        }
        else if (tmp == '#')
        {
            enter_and_exit_comment_with_errorfree(fd, start);
        }
        else if (tmp >= 48 && tmp <= 57)
        {
            current->next = malloc(sizeof(struct digits_chain));
            if (current->next == NULL)
            {
                exit_failure_with_errmessage_and_release(fd, start, "space allocation failed.\n");
            }
            current = current->next;
            current->digit = tmp;
            current->next = NULL;
        }
        else
        {
            exit_failure_with_errmessage_and_release(fd, start, "The width or height or maxval info in your file contains non digits. Please check your input file.\n");
        }
    }
}

static size_t get_number_from_digits(struct digits_chain *start)
{
    size_t ret = 0;
    struct digits_chain *tofree = NULL;
    while (start)
    {
        ret = ret * 10 + start->digit - 48;
        tofree = start;
        start = start->next;
        free(tofree);
    }
    return ret;
}

static void free_from_start(struct digits_chain *start)
{
    struct digits_chain *tofree = NULL;
    while (start)
    {
        tofree = start;
        start = start->next;
        free(tofree);
    }
}

static void exit_failure_with_errmessage_and_release(FILE *fd, struct digits_chain *start, const char *err_msg)
{
    fprintf(stderr, "%s", err_msg);
    fclose(fd);
    if (start)
    {
        free_from_start(start);
    }
    exit(EXIT_FAILURE);
}