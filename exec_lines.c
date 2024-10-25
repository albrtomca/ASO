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
#include <errno.h>

#define BUF_SIZE_DEF 16
#define MAX_LINE_SIZE_DEF 32

#define BUF_MIN 1
#define BUF_MAX 8192
#define LINE_MIN 16
#define LINE_MAX 1024

// Definimos un enumerado para los distintos operadores que puede tener un comando.
typedef enum
{
    NADA,
    REDIR_IZQ,
    REDIR_DCHA,
    REDIR2,
    TUBERIA
} operador_enum;

// En esta función hacemos el tratamiento de los errores que se pueden cometer en el momento de la ejecución de la linea
void analisis_errores_hijo(pid_t pid, int num_linea)
{
    int status;

    if (waitpid(pid, &status, 0) == -1)
    {
        perror("waitpid()");
        exit(EXIT_FAILURE);
    }

    if (WIFEXITED(status))
    {
        // Obtenemos el codigo del estado del hijo
        int codigo_salida = WEXITSTATUS(status);
        if (codigo_salida != 0) // En el caso de ser distinto de 0, significaría que ha fallado en la ejecución
        {
            fprintf(stderr, "Error al ejecutar la línea %d. Terminación normal con el código %d.\n", num_linea, codigo_salida);
            exit(WEXITSTATUS(status));
        }
    }
    else if (WIFSIGNALED(status)) // Captura del error si es cometido por una interrupción de señal.
    {
        fprintf(stderr, "Error al ejecutar la línea %d. Terminación anormal por señal %d.\n", num_linea, WTERMSIG(status));
        exit(WTERMSIG(status));
    }
}

// Función para ejecutar el comando
void ejecutar_comando_sin_operador(char *comando)
{
    char *ptrToken;
    char *saveptr;
    char **argum;
    int fd;

    // Creamos un buffer para almacenar el comando
    if ((argum = malloc(strlen(comando) * sizeof(char))) == NULL)
    {
        perror("malloc(argum)");
        exit(EXIT_FAILURE);
    }

    // Redirigimos la salida de error a /dev/null para que no salga por pantalla
    if ((fd = open("/dev/null", O_WRONLY, S_IRWXU)) == -1)
    {
        perror("open(/dev/null)");
        exit(EXIT_FAILURE);
    }

    if (dup2(fd, STDERR_FILENO) == -1)
    {
        perror("dup2(stderr)");
        exit(EXIT_FAILURE);
    }

    int i = 0;

    // Sacamos los argumentos del comando para poder ejecutarlos con execvp
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
    free(argum); // Liberamos la memoria.
    exit(EXIT_FAILURE);
}

// Función de procesamiento de línea cuando el operador es la redirección a la izquierda (<).
void redireccion_izq(char *lado_izq, char *lado_dcho, int num_linea)
{
    int status;
    pid_t pid;

    int fd; // Descriptor de fichero
    switch (pid = fork())
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

    default:
        analisis_errores_hijo(pid, num_linea);
        break;
    }
}

// Función de procesamiento de línea cuando el operador es la redirección doble o simple a la derecha (>> o >).
void redireccion_dcha_o_doble(char *lado_izq, char *lado_dcho, bool doble, int num_linea)
{
    pid_t pid;
    int fd; // Descriptor de fichero
    switch (pid = fork())
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
        if (doble) // Parámetro que indica si la redirección es doble o no.
        {
            if ((fd = open(lado_dcho, O_WRONLY | O_CREAT | O_APPEND, S_IRWXU)) == -1) // La redirección doble usa O_APPEND para escribir al final del fichero

            {
                perror("open(fd_doble)");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if ((fd = open(lado_dcho, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU)) == -1) // La redirección simple usa O_TRUNC para limpiar el fichero indicado
            {
                perror("open(fd_simple)");
                exit(EXIT_FAILURE);
            }
        }

        ejecutar_comando_sin_operador(lado_izq);
        break;

    default:
        analisis_errores_hijo(pid, num_linea);
        break;
    }
}

// Función de procesamiento de línea cuando el operador es una tubería (|).
void tuberia(char *lado_izq, char *lado_dcho, int num_linea)
{
    pid_t pid_izq;
    pid_t pid_dcho;

    int pipefds[2];

    if (pipe(pipefds) == -1)
    {
        perror("pipe()");
        exit(EXIT_FAILURE);
    }

    switch (pid_izq = fork())
    {
    case -1: // Caso de que fork() falle.
        perror("fork()");
        exit(EXIT_FAILURE);
        break;
    case 0:                          // hijo izquierdo
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

    switch (pid_dcho = fork())
    {
    case -1: // Caso de que fork() falle.
        perror("fork()");
        exit(EXIT_FAILURE);
        break;
    case 0:                          // hijo derecho
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
            perror("close()");
            exit(EXIT_FAILURE);
        }

        ejecutar_comando_sin_operador(lado_dcho);
        break;

    default:
        break;
    }

    // Cerramos extremos del padre
    if (close(pipefds[0]) == -1)
    {
        perror("close(pipefds[0])");
        exit(EXIT_FAILURE);
    }
    if (close(pipefds[1]) == -1)
    {
        perror("close(pipefds[1])");
        exit(EXIT_FAILURE);
    }

    // Analizamos por si falla alguno de los dos hijos para que señale el error.
    analisis_errores_hijo(pid_izq, num_linea);
    analisis_errores_hijo(pid_dcho, num_linea);
}

// Función de procesamiento de línea cuando no hay operadores.
void sin_operadores(char *lado_izq, int num_linea)
{
    pid_t pid;

    switch (pid = fork())
    {
    case -1: // error
        perror("fork()");
        exit(EXIT_FAILURE);
        break;
    case 0: // ejecucion proceso hijo
        ejecutar_comando_sin_operador(lado_izq);
        break;
    default:
        analisis_errores_hijo(pid, num_linea);
        break;
    }
}

// Función que analiza el operador que se encuentra en la lína a ejecutar
void procesar_linea(char *linea, int num_linea)
{
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
        lado_izq = strtok(linea, operador); // Separamos la parte izquierda de la tuberia de la parte derecha para su ejecución.
        lado_dcho = strtok(NULL, operador);
    }

    if (operador != NULL && enum_op != TUBERIA)
    {
        lado_izq = strtok(linea, operador); // Separamos la parte izquierda del operador de la parte derecha
        lado_dcho = strtok(NULL, operador);
        lado_dcho = strtok(lado_dcho, " "); // Volvemos a separar la derecha ya que aparece un espacio al principio, que nos dificulta su posterior ejecución.
    }

    // Tratamiento de casos en función del operador.
    switch (enum_op)
    {
    case REDIR2:
        redireccion_dcha_o_doble(lado_izq, lado_dcho, doble, num_linea);
        break;
    case REDIR_DCHA:
        redireccion_dcha_o_doble(lado_izq, lado_dcho, doble, num_linea);
        break;
    case REDIR_IZQ:
        redireccion_izq(lado_izq, lado_dcho, num_linea);
        break;
    case TUBERIA:
        tuberia(lado_izq, lado_dcho, num_linea);
        break;
    default:
        sin_operadores(lado_izq, num_linea);
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

    // Captura de las variables por la entrada estandar para la ejecución del ./exec_lines
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

    // Error si el tamaño del buffer pasado por entrada no se encuentra entre los valores predefinidos
    if (buf_size < BUF_MIN || buf_size > BUF_MAX)
    {
        fprintf(stderr, "Error: El tamaño de buffer tiene que estar entre 1 y 8192.\n");
        exit(EXIT_FAILURE);
    }

    // Error si el tamaño de línea pasado por entrada no se encuentra entre los valores predefinidos
    if (max_line_size < LINE_MIN || max_line_size > LINE_MAX)
    {
        fprintf(stderr, "Error: El tamaño de línea tiene que estar entre 16 y 1024.\n");
        exit(EXIT_FAILURE);
    }

    // Guardamos memoria para el buffer
    if ((buffer = (char *)malloc(buf_size * sizeof(char))) == NULL)
    {
        perror("malloc(buffer)");
        exit(EXIT_FAILURE);
    }
    // Guardamos memoria para el buffer de la linea que usaremos para ir guardando los datos que vayamos leyendo del buffer.
    if ((linea = (char *)malloc(max_line_size * sizeof(char))) == NULL)
    {
        perror("malloc(linea)");
        exit(EXIT_FAILURE);
    }

    ssize_t num_leidos;
    int indice_linea = 0;
    int num_linea = 1;

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
                fprintf(stderr, "Error, línea %d demasiado larga: \"%s...\" \n", num_linea, linea);
                exit(EXIT_FAILURE);
            }

            // Si no ha alcanzado el límite, añadimos el dato leido a un buffer de datos leidos
            linea[indice_linea] = buffer[i];
            if (buffer[i] == '\n')
            {
                char *lineaBuf = strtok(linea, "\n");

                procesar_linea(lineaBuf, num_linea);
                num_linea++;

                // Reseteamos el indice del buffer la linea.
                indice_linea = 0;
            }
            else
            {
                indice_linea++;
            }
        }
    }

    // Liberamos la memoria dinamica de ambos buffers
    free(buffer);
    free(linea);
    exit(EXIT_SUCCESS);
}
