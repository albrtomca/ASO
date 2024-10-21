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
#include <stdbool.h>

#define BUF_SIZE_DEF 16
#define MAX_LINE_SIZE_DEF 32

#define BUF_MIN 1
#define BUF_MAX 8192
#define LINE_MIN 16
#define LINE_MAX 1024

typedef enum
{
    NADA,
    REDIR_IZQ,
    REDIR_DCHA,
    REDIR2,
    TUBERIA
} operador_enum;

void ejecutar_comando_sin_operador(char *comando)
{
    char *ptrToken;
    char *saveptr;
    char **argum;

    if ((argum = malloc(strlen(comando) * sizeof(char))) == NULL)
    {
        perror("malloc(argum)");
        exit(EXIT_FAILURE);
    }

    int i = 0;

    ptrToken = strtok_r(comando, " ", &saveptr);
    while (ptrToken != NULL)
    {
        argum[i] = ptrToken;
        i++;
        ptrToken = strtok_r(NULL, " ", &saveptr);
    }

    argum[i] = NULL;

    execvp(argum[0], argum);
    perror("exec()");
    free(argum);
    exit(EXIT_FAILURE);
}

void redireccion_izq(char *lado_izq, char *lado_dcho)
{
    int fd; // Descriptor de fichero
    switch (fork())
    {
    case -1: // error
        perror("fork()");
        exit(EXIT_FAILURE);
        break;
    case 0: // ejecucion proceso hijo
        if (close(STDIN_FILENO) == -1)
        {
            perror("close(STDOUT_FILENO)");
            exit(EXIT_FAILURE);
        }

        if ((fd = open(lado_dcho, O_RDONLY | O_CREAT, S_IRWXU)) == -1)
        {
            perror("open(fd_entrada)");
            exit(EXIT_FAILURE);
        }

        ejecutar_comando_sin_operador(lado_izq);

        break;
    }
}

void redireccion_dcha_o_doble(char *lado_izq, char *lado_dcho, bool doble)
{
    int fd; // Descriptor de fichero
    switch (fork())
    {
    case -1: // error
        perror("fork()");
        exit(EXIT_FAILURE);
        break;
    case 0: // ejecucion proceso hijo
        if (close(STDOUT_FILENO) == -1)
        {
            perror("close(STDOUT_FILENO)");
            exit(EXIT_FAILURE);
        }
        if (doble)
        {
            if ((fd = open(lado_dcho, O_WRONLY | O_CREAT | O_APPEND, S_IRWXU)) == -1)
            {
                perror("open(fd_doble)");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if ((fd = open(lado_dcho, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU)) == -1)
            {
                perror("open(fd_simple)");
                exit(EXIT_FAILURE);
            }
        }

        ejecutar_comando_sin_operador(lado_izq);

        break;

    default: // ejecucion proceso padre (espera al hijo)
        if (wait(NULL) == -1)
        {
            perror("wait()");
            exit(EXIT_FAILURE);
        }
        break;
    }
}

void tuberia(char *lado_izq, char *lado_dcho)
{
    int pipefds[2];

    if (pipe(pipefds) == -1)
    {
        perror("pipe()");
        exit(EXIT_FAILURE);
    }

    switch (fork())
    {
    case -1:
        perror("fork()");
        exit(EXIT_FAILURE);
        break;
    case 0: // hijo izquierdo
        if (close(pipefds[0]) == -1) // cerramos el descriptor de lectura
        {
            perror("close()");
            exit(EXIT_FAILURE);
        }

        if (dup2(pipefds[1], STDOUT_FILENO) == -1) // redirigimos al extremo de escritura del pipe
        {
            perror("dup2()");
            exit(EXIT_FAILURE);
        }

        if (close(pipefds[1])) // cerramos el duplicado
        {
            perror("close()");
            exit(EXIT_FAILURE);
        }

        ejecutar_comando_sin_operador(lado_izq);

        break;
    default:
        break;
    }

    switch (fork())
    {
    case -1:
        perror("fork()");
        exit(EXIT_FAILURE);
        break;
    case 0: // hijo derecho
        if (close(pipefds[1]) == -1) // cerramos el extremo de escritura
        {
            perror("close()");
            exit(EXIT_FAILURE);
        }

        if (dup2(pipefds[0], STDIN_FILENO) == -1) // redirigimos al extremo de lectura del pipe
        {
            perror("dup2()");
            exit(EXIT_FAILURE);
        }

        if (close(pipefds[0]) == -1) // cerramos el duplicado
        {
            perror("close(4)");
            exit(EXIT_FAILURE);
        }

        ejecutar_comando_sin_operador(lado_dcho);

        break;
    default: 
        break;
    }
}

void procesar_linea(char *linea)
{
    // printf("line:%s\n",linea);
    char *lado_izq, *operador, *lado_dcho = NULL;
    operador_enum enum_op = NADA;
    bool doble;

    if (strstr(linea, ">>") != NULL)
    {
        operador = ">>";
        doble = true;
        enum_op = REDIR2;
    }
    else if (strchr(linea, '>') != NULL)
    {
        operador = ">";
        doble = false;
        enum_op = REDIR_DCHA;
    }
    else if (strchr(linea, '<') != NULL)
    {
        operador = "<";
        enum_op = REDIR_IZQ;
    }
    else if (strchr(linea, '|') != NULL)
    {
        operador = "|";
        enum_op = TUBERIA;
    }

    if (operador != NULL)
    {
        lado_izq = strtok(linea, operador);
        lado_dcho = strtok(NULL, operador);
        lado_dcho = strtok(lado_dcho, " ");
    }

    // printf("Lado izq: %s\n", lado_izq);

    switch (enum_op)
    {
    case REDIR2:
        redireccion_dcha_o_doble(lado_izq, lado_dcho, doble);
        break;
    case REDIR_DCHA:
        redireccion_dcha_o_doble(lado_izq, lado_dcho, doble);
        break;
    case REDIR_IZQ:
        redireccion_izq(lado_izq, lado_dcho);
        break;
    case TUBERIA:
        tuberia(lado_izq, lado_dcho);
        break;
    default:
        ejecutar_comando_sin_operador(lado_izq);
        break;
    }
}

int main(int argc, char **argv)
{
    pid_t pid; /* Usado en el proceso padre para guardar el PID del proceso hijo */
    int opt;
    int buf_size = BUF_SIZE_DEF, max_line_size = MAX_LINE_SIZE_DEF;

    char *buffer;
    char *linea;

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
            printf("Uso: %s [-b BUF_SIZE] [-l MAX_LINE_SIZE] \n"
                   "Lee de la entrada estándar una secuencia de líneas conteniendo órdenes para ser ejecutadas y \n"
                   "lanza los procesos necesarios para ejecutar cada línea, esperando a su terminación para ejecutar la siguiente. \n"
                   "-b BUF_SIZE \t Tamaño del buffer de entrada 1 <= BUF_SIZE <= 8192 \n"
                   "-l MAX_LINE_SIZE \t Tamaño máximo de línea 16 <= MAX_LINE_SIZE <= 1024\n",
                   argv[0]);
            exit(EXIT_SUCCESS);
        default:
            fprintf(stderr, "Uso: %s [-b BUF_SIZE] [-l MAX_LINE_SIZE] \n"
                            "Lee de la entrada estándar una secuencia de líneas conteniendo órdenes para ser ejecutadas y \n"
                            "lanza los procesos necesarios para ejecutar cada línea, esperando a su terminación para ejecutar la siguiente. \n"
                            "-b BUF_SIZE \t Tamaño del buffer de entrada 1 <= BUF_SIZE <= 8192 \n"
                            "-l MAX_LINE_SIZE \t Tamaño máximo de línea 16 <= MAX_LINE_SIZE <= 1024\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (buf_size < BUF_MIN || buf_size > BUF_MAX)
    {
        fprintf(stderr, "Error: El tamaño de buffer tiene que estar entre 1 y 8192.\n");
    }

    if (max_line_size < LINE_MIN || max_line_size > LINE_MAX)
    {
        fprintf(stderr, "Error: El tamaño de línea tiene que estar entre 16 y 1024.\n");
    }

    // Guardamos memoria para el buffer
    if ((buffer = (char *)malloc(buf_size * sizeof(char))) == NULL)
    {
        perror("malloc(buffer)");
        exit(EXIT_FAILURE);
    }

    if ((linea = (char *)malloc(max_line_size * sizeof(char))) == NULL)
    {
        perror("malloc(linea)");
        exit(EXIT_FAILURE);
    }

    ssize_t num_leidos;
    int indice_linea = 0;
    int num_linea_error = 1;
    bool controlador_error = false;
    char *resto;

    // Leemos de la entrada estandar
    while ((num_leidos = read(STDIN_FILENO, buffer, buf_size)) > 0)
    {
        // De los datos que vamos leyendo
        for (int i = 0; i < num_leidos; i++)
        {
            // Comprobamos que no haya alcanzado el maximo de línea permitido
            if (indice_linea == max_line_size)
            {
                // De ser así, imprimimos un mensaje de error y hacemos tratamiento de datos
                fprintf(stderr, "Error, línea %d demasiado larga: %s \n", num_linea_error, linea);
                num_linea_error++;
                // memset(linea, 0, max_line_size);
                indice_linea = 0;
                controlador_error = true;
            }

            // Si no ha alcanzado el límite, añadimos el dato leido a un buffer de datos leidos
            // printf("Contenido del buffer de linea: %s\n", linea);
            linea[indice_linea] = buffer[i];
            if (buffer[i] == '\n')
            {
                char *lineaBuf = strtok(linea, "\n");
                char *restoBuf = strtok(NULL, "");
                // printf("Contenido Antes de ejecucion: %s\n", lineaBuf);

                // Aqui se trata el resto de la linea cuando ha pasado por el condicionante de error
                if (controlador_error == true)
                {
                    controlador_error = false;
                }
                else
                { // Si no ha pasado por el error, ejecutamos la linea
                    procesar_linea(lineaBuf);
                    num_linea_error++;
                }
                // Reseteamos valores
                indice_linea = 0;
                // memset(linea, 0, max_line_size);
            }
            else
            {
                indice_linea++;
            }
        }
    }

    free(buffer);
    free(linea);
    exit(EXIT_SUCCESS);
}
