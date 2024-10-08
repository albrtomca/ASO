// Alberto Meca Inoriza (3.1) y Ismael Moussaid Gómez (3.3)

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUF_SIZE_DEF 16
#define MAX_LINE_SIZE_DEF 32

#define BUF_MIN 1
#define BUF_MAX 8192
#define LINE_MIN 16
#define LINE_MAX 1024

int main(int argc, char **argv)
{
    int opt;
    int buf_size = BUF_SIZE_DEF, max_line_size = MAX_LINE_SIZE_DEF;
    
    optind = 1;
    while ((opt = getopt(argc, argv, "b:l:h")) != -1)
    {
        switch (opt)
        {
        case 'b':
            buf_size = atoi(optarg);
            break;
        case 'l':
            max_line_size = atoi(optarg);
            break;
        case 'h':
            printf("Uso: %s [-b BUF_SIZE] [-l MAX_LINE_SIZE] \n" \
            "Lee de la entrada estándar una secuencia de líneas conteniendo órdenes para ser ejecutadas y \n" \
            "lanza los procesos necesarios para ejecutar cada línea, esperando a su terminación para ejecutar la siguiente. \n" \
            "-b BUF_SIZE \t Tamaño del buffer de entrada 1 <= BUF_SIZE <= 8192 \n" \
            "-l MAX_LINE_SIZE \t Tamaño máximo de línea 16 <= MAX_LINE_SIZE <= 1024\n", argv[0]);
            exit(EXIT_SUCCESS);
        default:
            fprintf(stderr, "Uso: %s [-b BUF_SIZE] [-l MAX_LINE_SIZE] \n" \
            "Lee de la entrada estándar una secuencia de líneas conteniendo órdenes para ser ejecutadas y \n" \
            "lanza los procesos necesarios para ejecutar cada línea, esperando a su terminación para ejecutar la siguiente. \n" \
            "-b BUF_SIZE \t Tamaño del buffer de entrada 1 <= BUF_SIZE <= 8192 \n" \
            "-l MAX_LINE_SIZE \t Tamaño máximo de línea 16 <= MAX_LINE_SIZE <= 1024\n", argv[0]);
            exit(EXIT_FAILURE);
            
        }
    }

    
    if(buf_size < BUF_MIN || buf_size > BUF_MAX)
    {
        fprintf(stderr, "Error: El tamaño de buffer tiene que estar entre 1 y 8192.\n");
    }

    if(max_line_size < LINE_MIN || max_line_size > LINE_MAX)
    {
        fprintf(stderr, "Error: El tamaño de línea tiene que estar entre 16 y 1024.\n");
    }


    // printf("flag: %d, n: %d, s: \"%s\"", flag, n, s);
    // for (int i = optind; i < argc; i++)
    //     printf(", param%d: \"%s\"", i - optind, argv[i]);
    // printf("\n");

    return EXIT_SUCCESS;
}
