#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#define SIZE 4096
#define SERVFIFO "./fifo.serv"

int main(int argc, char** argv, char** envp) {
    int fdin, fdout, fh, rd = -1, notend, req = 0;
    char* CLFIFO, * filename, DEAD[] = "stop";
    pid_t chpid;
    //enough arguments
    if (argc != 1) {
        printf("Error, incorrect number of arguments\n");
        return 0;
    }

    void* buf = NULL;
    if ((buf = malloc(SIZE)) == NULL) {
        fprintf(stderr, "ERROR\n");
        return -1;
    }
    umask(0);

    // open FIFO for input
    if (mkfifo(SERVFIFO, 0600) < 0) {
        perror("create fifo fail");
        return errno;
    }

    if ((CLFIFO = malloc(SIZE)) == NULL) {
        fprintf(stderr, "ERROR\n");
        return -1;
    }
    if ((filename = malloc(SIZE)) == NULL) {
        fprintf(stderr, "ERROR\n");
        return -1;
    }

    // read client FIFO and filename
    do {
        if ((fdin = open(SERVFIFO, O_RDONLY)) < 0) {
            perror("open server fifo fail");
            return errno;
        }
        if ((rd = read(fdin, buf, SIZE)) < 0) {
            perror("FIFO read fail");
            return errno;
        }
        close(fdin);
        sscanf(buf, "%s %s", CLFIFO, filename);
        //shutdown
        if (strcmp(filename, DEAD) == 0)
            break;

        if ((chpid = fork()) < 0) {
            perror("fork fail");
            return errno;
        }

        //fork
        if (chpid == 0) {
            if ((fdout = open(CLFIFO, O_WRONLY)) < 0) {
                perror("open client FIFO fail");
                return errno;
            }
            if ((fh = open(filename, O_RDONLY)) < 0) {
                perror("file open fail");
                if (write(fdout, "SERVER ERROR", strlen("SERVER ERROR") + 1) < 0) {
                    perror("FIFO write fail");
                    return errno;
                }
                return errno;
            }

            // send file info
            while (1) {
                if ((rd = read(fh, buf, SIZE)) < 0) {
                    perror("file read fail");
                    if (write(fdout, "SERVFIFO ERROR", strlen("SERVFIFO ERROR") + 1) < 0) {
                        perror("FIFO write fail");
                        return errno;
                    }
                    return errno;
                }
                if (rd == 0)
                    break;
                if (write(fdout, buf, rd) < 0) {
                    perror("FIFO write fail");
                    if (write(fdout, "SERVFIFO ERROR", strlen("SERVFIFO ERROR") + 1) < 0) {
                        perror("FIFO write fail");
                        return errno;
                    }
                    return errno;
                }
            };
            close(fdout);
            close(fh);

            printf("Process %d done, file: %s\n", req + 1, filename);
            fflush(stdout);
            free(CLFIFO);
            free(filename);
            free(buf);
            return 0;
        }
        notend = strcmp(filename, DEAD);
        req++;
    } while (notend != 0);

    printf("Total %d requests\n", req);
    free(CLFIFO);
    free(filename);
    unlink(SERVFIFO);
    free(buf);

    return 0;
}