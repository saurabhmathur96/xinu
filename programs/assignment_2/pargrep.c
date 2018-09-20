#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include "ring_buffer.h"

#define USAGE "Usage:\n"\
              "$> pargrep [-t] word [file]\n"\
              "Searches for <word> in provided <file> or standard input.\n\n"\
              "Pargrep options:\n"\
              "-t          number of threads\n"\
              "word        string to be searched in the file\n"\
              "file        file to lookup or search\n"\

struct search_line_args
{
    ring_buffer_t *line_queue;
    char *word;    
};

struct line_queue_item
{
    char *line;
    long long int line_number;
};

pthread_mutex_t line_number_lock;
long long int line_number = -1;

int get_line_number()
{
    long long int n;
    pthread_mutex_lock(&line_number_lock);
    n = line_number;
    pthread_mutex_unlock(&line_number_lock);
    return  n;
}

void *search_line(void *args)
{
    struct search_line_args *params = args;
    char *word = params->word;
    ring_buffer_t *line_queue = params->line_queue;

    struct line_queue_item *current_item;
    for(current_item = ring_buffer_remove(line_queue);
        current_item != NULL;
        current_item = ring_buffer_remove(line_queue))
    {
        // if current_item == NULL, then all lines processed
        // => exit

        char * position = strstr(current_item->line, word);
        while(get_line_number()+1 != current_item->line_number)
            sched_yield();
        if (position)
        {
            printf("%s", current_item->line);
        }
        pthread_mutex_lock(&line_number_lock);
        line_number++;
        pthread_mutex_unlock(&line_number_lock);
    }

    return NULL;
}


long get_n_threads(char *arg)
{
    char *end;
    long n_threads = strtol(arg, &end, 10);
    if (end == arg)
    {
        return -1; // Failure
    }
    else
    {
        return n_threads;
    }
}

int main(int argc, char *argv[])
{
    FILE *infile_handle = stdin;
    char *word;
    long n_threads = 3;
    if (argc < 2)
    {
        printf("Too few arguments.\n");
        printf(USAGE);
        return EXIT_FAILURE;
    }
    switch(argc)
    {
        case 2:
            if (0 == strcmp("-t", argv[1]))
            {
                printf("Too few arguments.\n");
                printf(USAGE);
                return EXIT_FAILURE;
            }
            word = argv[1];
            break;
        case 3:
            word = argv[1];
            infile_handle = fopen(argv[2], "r");
            break;
        case 4:
            if (0 == strcmp("-t", argv[1]))
            {
                n_threads = get_n_threads(argv[2]);
                word = argv[3];
            } 
            else if (0 == strcmp("-t", argv[2]))
            {
                word = argv[1];
                n_threads = get_n_threads(argv[3]);
            }
            else
            {
                printf("Incorrect argument format.\n");
            }
            break;

        case 5:
            if (0 == strcmp("-t", argv[1]))
            {
                n_threads = get_n_threads(argv[2]);
                word = argv[3];
                infile_handle = fopen(argv[4], "r");
            } 
            else if (0 == strcmp("-t", argv[2]))
            {
                word = argv[1];
                n_threads = get_n_threads(argv[3]);
                infile_handle = fopen(argv[4], "r");
            }
            else if (0 == strcmp("-t", argv[3]))
            {
                word = argv[1];
                infile_handle = fopen(argv[2], "r");
                n_threads = get_n_threads(argv[4]);
            }
            else
            {
                printf("Incorrect argument format.\n");
                printf(USAGE);
            }
            break;

        default:
            printf("Too many arguments.\n");
            printf(USAGE);
            return EXIT_FAILURE;
            
    }

    if (errno != 0 || n_threads < 1)
    {
        printf("Invalid arguments.\n");
        printf(USAGE);
        return EXIT_FAILURE;
    }
    pthread_t *threads = malloc(n_threads * sizeof(*threads));
    if (!threads)
    {
        fprintf(stderr, "Error allocating memory\n");
        return EXIT_FAILURE;
    }

    ring_buffer_t line_queue;
    ring_buffer_initialize(&line_queue);

    
    struct search_line_args args = {
        .line_queue = &line_queue,
        .word = word
    };

    int i;
    for (i=0; i<n_threads; i++)
    {
        pthread_create(&threads[i], NULL, *search_line, &args);
    }

    size_t n_read, size = 0;
    char *current_line = NULL;
    long long int current_line_number = 0;
    while ((n_read = getline(&current_line, &size, infile_handle)) != -1)
    {   // n_read+1 = no. of characters + 1 null terminator
        char *line = malloc((n_read+1) * sizeof(*line));
        if (!line)
        {
            // Error !
            printf("Unable to allocate memory.\n");
        }
        strncpy(line, current_line, n_read+1);

        struct line_queue_item *item = malloc(sizeof(*item));
        item->line = line;
        item->line_number = current_line_number++;
        ring_buffer_insert(&line_queue, item);
        
    }
    if (infile_handle != stdin)
    {
        fclose(infile_handle);
    }
    
    if (current_line)
    {
        free(current_line);
    }



    for (i=0; i<n_threads; i++)
    {
        ring_buffer_insert(&line_queue, NULL);
    }
    
    for (i=0; i<n_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    ring_buffer_destroy(&line_queue);

    return EXIT_SUCCESS;
}
