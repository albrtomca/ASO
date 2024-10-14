// Alberto Meca Inoriza (3.1) y Ismael Moussaid Gómez (3.3)

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUF_SIZE_DEF 16
#define MAX_LINE_SIZE_DEF 32

#define BUF_MIN 1
#define BUF_MAX 8192
#define LINE_MIN 16
#define LINE_MAX 1024


int main(int argc, char **argv)
{
    pid_t pid; /* Usado en el proceso padre para guardar el PID del proceso hijo */
    int fd; //Descriptor de fichero
    int opt;
    int buf_size = BUF_SIZE_DEF, max_line_size = MAX_LINE_SIZE_DEF;

    char *buffer;
    
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
    
    //Guardamos memoria para el buffer
    if ((buffer = (char *) malloc(buf_size * sizeof(char))) == NULL)
    {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }


    ssize_t num_leidos;
    ssize_t offset = 0; //Controlar los saltos de línea y guardar la siguiente instrucción si se evalua en la iteración anterior

    //Si de una pasada leemos una instrucción, salto de línea y parte de la siguiente instrucción, esa segunda parte se guarda en el offset
    //La segunda condición sirve para que si de una pasada leemos la línea completa pero incluye varias instrucciones, también se traten 
    while((num_leidos = read(STDIN_FILENO, buffer + offset, buf_size - offset)) > 0 || offset != 0)
    {
        if(num_leidos == -1)
        {
            perror("read()");
            exit(EXIT_FAILURE);
        }
        
        //Para llevar un control y saber donde está el offset
        buffer[offset + num_leidos] = '\0'; //preguntar si eso esta bien

        //Trozeamos la entrada para obtener la primera instrucción el resto, que se encontrará después del salto de línea \n
        char *linea = strtok(buffer, "\n");
        char *resto = strtok(NULL, "");

        printf("Numero de bytes leidos: %ld\n", num_leidos);
        printf("Buffer: %s\n", buffer);

        //Poner el while o no, genera la misma salida por el print
        while (linea != NULL) 
        {
            printf("Linea: %s\n", linea);   
            linea = strtok(NULL, "\n"); 
        }
        //printf("Linea: %s\n", linea);

        //Comprobamos si hemos leido algo más a partir del \n
        //En caso afirmativo guardamos la longitud en el offset y lo movemos al principio del buffer
        //En caso contrario no guardamos nada
        if(resto != NULL)
        {
            offset = strlen(resto);
            memmove(buffer, resto, offset);
        }
        else
            offset = 0;
        
        

        printf("Resto: %s\n", resto);


        
    } 
    





    // ssize_t bytes_leidos;
    // //buffer[buf_size + 1]; // Espacio adicional para el terminador nulo
    // buffer[0] = '\0';          // Inicia el buffer vacío
    // size_t offset = 0;         // Inicio para la siguiente lectura

    // while ((bytes_leidos = read(STDIN_FILENO, buffer + offset, buf_size - offset)) > 0) {
    //     if (bytes_leidos == -1) {
    //         perror("read()");
    //         exit(EXIT_FAILURE);
    //     }

    //     // Agregar el terminador nulo al final de los datos leídos
    //     buffer[offset + bytes_leidos] = '\0';

    //     // Procesar el contenido del buffer
    //     char *linea = strtok(buffer, "\n");
    //     char *resto = strtok(NULL, "");

    //     while (linea != NULL) {
    //         printf("Linea: %s\n", linea);
    //         linea = strtok(NULL, "\n");
    //     }

    //     // Mover el resto al inicio del buffer para la siguiente iteración
    //     if (resto != NULL) {
    //         offset = strlen(resto);
    //         memmove(buffer, resto, offset);
    //     } else {
    //         offset = 0;  // No hay resto, comenzamos desde el inicio
    //     }

    //     printf("Numero de bytes leidos: %ld\n", bytes_leidos);
    //     printf("Buffer para la siguiente iteración: %s\n", buffer);
    // }

    // if (bytes_leidos == -1) {
    //     perror("read()");
    //     exit(EXIT_FAILURE);
    // }




    free(buffer);
    exit(EXIT_SUCCESS);
}
