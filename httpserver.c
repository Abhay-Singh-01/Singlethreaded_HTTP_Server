#include <stdio.h>
#include <stdlib.h>
#include "helper_funcs.h"
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <regex.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <errno.h>

//Buffer Size
#define BUFF 4096

//Valid Request REgular Expression
// #define REQexpr "([a-zA-Z].{1,8}) /([a-zA-Z0-9.-].{2,64}) (HTTP/[0-9].[0-9]).{8}\r\n(([a-zA-Z0-9.-].{1,128}): ([a-zA-Z0-9._-].{1,128})\r\n)*\r\n"
#define REQexpr                                                                                    \
    "([a-zA-Z]{1,8}) /([a-zA-Z0-9.-]{2,64}) (HTTP/[0-9].[0-9])(\r\n)(([a-zA-Z0-9.-]{1,128}): "     \
    "([a-zA-Z0-9./*_-]{1,128})\r\n)*(\r\n)*"

//Status Code Messages
#define Pok       "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nOK\n"
#define crea      "HTTP/1.1 201 Created\r\nContent-Length: 8\r\n\r\nCreated\n"
#define badreq    "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n"
#define forbidden "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n"
#define NotFound  "HTTP/1.1 404 Not Found\r\nContent-Length: 10\r\n\r\nNot Found\n"
#define ServErr                                                                                    \
    "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 22\r\n\r\nInternal Server Error\n"
#define notimp "HTTP/1.1 501 Not Implemented\r\nContent-Length: 16\r\n\r\nNot Implemented\n"
#define notsup                                                                                     \
    "HTTP/1.1 505 Version Not Supported\r\nContent-Length: 22\r\n\r\nVersion Not Supported\n"

struct Request {
    char *command;
    char filepath[BUFF];
    char *version;
    char Keys[BUFF];
    char Values[BUFF];
    int term_start;
};

int bad_request(int socket) {
    char buff[BUFF];
    strcpy(buff, badreq);
    int len = strlen(badreq);
    int written = write(socket, buff, len);
    if (written != -1) {
        return 0;
    } else {
        return 1;
    }
}

int notimplemented(int socket) {
    char buff[BUFF];
    strcpy(buff, notimp);
    int len = strlen(notimp);
    int written = write(socket, buff, len);
    if (written != -1) {
        return 0;
    } else {
        return 1;
    }
}

int notsupportedver(int socket) {
    char buff[BUFF];
    strcpy(buff, notsup);
    int len = strlen(notsup);
    int written = write(socket, buff, len);
    if (written != -1) {
        return 0;
    } else {
        return 1;
    }
}

int notfound(int socket) {
    char buff[BUFF];
    strcpy(buff, NotFound);
    int len = strlen(NotFound);
    int written = write(socket, buff, len);
    if (written != -1) {
        return 0;
    } else {
        return 1;
    }
}

int forbid(int socket) {
    char buff[BUFF];
    strcpy(buff, forbidden);
    int len = strlen(forbidden);
    int written = write(socket, buff, len);
    if (written != -1) {
        return 0;
    } else {
        return 1;
    }
}

int Serv_err(int socket) {
    char buff[BUFF];
    strcpy(buff, ServErr);
    int len = strlen(ServErr);
    int written = write(socket, buff, len);
    if (written != -1) {
        return 0;
    } else {
        return 1;
    }
}

int Put_OK(int socket) {
    char buff[BUFF];
    strcpy(buff, Pok);
    int len = strlen(Pok);
    int written = write(socket, buff, len);
    if (written != -1) {
        return 0;
    } else {
        return 1;
    }
}

int created(int socket) {
    char buff[BUFF];
    strcpy(buff, crea);
    int len = strlen(crea);
    int written = write(socket, buff, len);
    if (written != -1) {
        return 0;
    } else {
        return 1;
    }
}

int main(int argc, char **argv) {

    if (argc != 2) {
        fprintf(stderr, "Invalid Arguments\n");
        exit(1);
    }

    int port = atoi(argv[1]);

    if (port == 0 || port < 1 || port > 65535) {
        fprintf(stderr, "Invalid Port\n");
        exit(1);
    }

    Listener_Socket sock;

    int listener = listener_init(&sock, port);

    if (listener != 0) {
        fprintf(stderr, "Invalid Port\n");
        exit(0);
    }

    char buf[BUFF + 1];

    regex_t re;

    int compilereg = regcomp(&re, REQexpr, REG_EXTENDED);

    if (compilereg != 0) {
        fprintf(stderr, "Failed to compile Regex\n");
        exit(1);
    }

    // bool error = false;

    while (1) {
        int accepted = listener_accept(&sock);

        if (accepted > 0) {

            //read a buffer amount of bytes (Request should be in here)---------------------------------------------------------------------------------------------------------------
            int bread = read(accepted, buf, BUFF);

            //if read was unsuccessfull, print an error---------------------------------------------------------------------------------------------------
            if (bread == -1) {
                Serv_err(accepted);
                close(accepted);
                continue;
            }

            //null - terminate the buffer------------------------------------------------------------------------------------------------------------------
            buf[bread] = '\0';

            //run the regex on the buffer string to see if it is a valid request or not----------------------------------------------------------------
            int valid = regexec(&re, buf, 0, NULL, 0);

            //if the request is invalid print a bad request error 400 (Regex find no match)
            if (valid == REG_NOMATCH) {
                bad_request(accepted);
                printf("%s\n", "thing1");
                close(accepted);
                continue;
            }

            //initialize struct to store request data------------------------------------------------------------------------------------------------
            struct Request REQ;

            // index where the first \r in "\r\n\r\n" occurs start of body would be term_start + 4)--------------------------------------------------
            char *s;
            s = strstr(buf, "\r\n\r\n");
            REQ.term_start = s - buf;

            //Get the command and check if it is GET(1) or PUT(2), if not print an error code--------------------------------------------------------
            REQ.command = strtok(buf, " ");

            int type = 0;
            if (strcmp(REQ.command, "GET") == 0) {
                type = 1;
            } else if (strcmp(REQ.command, "PUT") == 0) {
                type = 2;
            } else {
                notimplemented(accepted);
                close(accepted);
                continue;
            }

            //get the filepath, get rid of forward slash to just get file name ------------------------------------------------------------
            char *file = strtok(NULL, " ");
            strcpy(REQ.filepath, file + 1);

            //get the version, if the version is not 1.1 or has too many digits print unsupported ver or bad req---------------------------
            REQ.version = strtok(NULL, "\r\n");
            if (strlen(REQ.version) != 8) {
                bad_request(accepted);
                close(accepted);
                continue;
            }

            if (strcmp(REQ.version, "HTTP/1.1") != 0) {
                notsupportedver(accepted);
                close(accepted);
                continue;
            }

            //start - aka after Status Line and 1st /r/n ----------------------------------------------------------------------------------

            int start = strlen(REQ.command) + strlen(REQ.filepath) + strlen(REQ.version) + 5;

            // header parsing algorithm ---------------------------------------------------------------------------------------------------
            bool clengthexists = false;
            char current_string[BUFF + 1] = "";
            char clength[BUFF] = "";
            int num_string = 0;

            for (int i = start; i < REQ.term_start; i++) {
                if (buf[i] != ' ' && buf[i] != '\r' && buf[i] != '\n') {
                    current_string[num_string] = buf[i];
                    num_string += 1;
                } else {
                    current_string[num_string] = '\0';
                    if (strcmp(current_string, "Content-Length:") == 0) {
                        clengthexists = true;
                    } else if (clengthexists == true && strlen(clength) == 0) {
                        strcpy(clength, current_string);
                    }
                    memset(current_string, 0, sizeof(current_string));
                    num_string = 0;
                }
            }

            if (strlen(current_string) > 0 && clengthexists == true && strlen(clength) == 0) {
                strcpy(clength, current_string);
            }

            // printf("Command: %s Filepath: %s\n Version: %s Start: %d\n Termination Start: %d Clength: %s\n Bytes_read %d\n", REQ.command, REQ.filepath, REQ.version, start, REQ.term_start, clength, bread);

            //Doing a Get Request----------------------------------------------------------------------------------------------------------------
            struct stat st;

            if (type == 1) {
                int getpath = open(REQ.filepath, O_RDONLY, 0666);
                if (getpath == -1) {
                    if (errno == ENOENT) {
                        notfound(accepted);
                        close(accepted);
                        continue;
                    } else if (errno == EACCES) {
                        forbid(accepted);
                        close(accepted);
                        continue;
                    } else {
                        Serv_err(accepted);
                        close(accepted);
                        continue;
                    }
                }
                fstat(getpath, &st);
                int siz = st.st_size;
                // printf("%d\n", siz);
                if (S_ISDIR(st.st_mode)) {
                    forbid(accepted);
                    close(getpath);
                    close(accepted);
                    continue;
                }
                // printf("%s\n", "good till here1");
                char beg[BUFF] = "HTTP/1.1 200 OK\r\nContent-Length: ";
                write(accepted, beg, strlen(beg));
                // printf("%s\n", "good till here2");
                char size[BUFF + 1];
                sprintf(size, "%d", siz);
                write(accepted, size, strlen(size));
                // printf("%s\n", "good till here3");
                char end[BUFF] = "\r\n\r\n";
                write(accepted, end, strlen(end));
                // printf("%s\n", "good till here4");

                int passing = pass_bytes(getpath, accepted, siz);
                printf("%d\n", passing);
                if (passing == -1) {
                    Serv_err(accepted);
                    close(getpath);
                    close(accepted);
                    continue;
                }
                close(getpath);
            }

            //Doing a PUT Request ------------------------------------------------------------------------------------------------------------------------------------------------

            int file_status = 0;
            if (type == 2) {
                int putpath;
                if (clengthexists == false) {
                    bad_request(accepted);
                    close(accepted);
                    continue;
                }
                if (access(REQ.filepath, F_OK) == -1) {
                    putpath = open(REQ.filepath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                    if (putpath == -1) {
                        Serv_err(accepted);
                        close(accepted);
                        continue;
                    }
                    file_status = 1;
                } else {
                    putpath = open(REQ.filepath, O_WRONLY | O_TRUNC, 0666);
                    if (putpath == -1) {
                        Serv_err(accepted);
                        close(accepted);
                        continue;
                    }
                    file_status = 2;
                }

                int leftovers
                    = write_all(putpath, buf + REQ.term_start + 4, bread - (REQ.term_start + 4));
                if (leftovers == -1) {
                    Serv_err(accepted);
                    close(putpath);
                    close(accepted);
                    continue;
                }

                int rest = atoi(clength) - leftovers;
                int passing = pass_bytes(accepted, putpath, rest);
                if (passing == -1) {
                    Serv_err(accepted);
                    close(putpath);
                    close(accepted);
                    continue;
                }

                if (file_status == 1) {
                    created(accepted);
                    close(putpath);
                    close(accepted);
                    continue;
                } else {
                    Put_OK(accepted);
                    close(putpath);
                    close(accepted);
                    continue;
                }

                close(putpath);
            }

            // printf("Command: %s Filepath: %s\n Version: %s Start: %d\n Termination Start: %d Clength: %s\n Bytes_read %d\n", REQ.command, REQ.filepath, REQ.version, start, REQ.term_start, clength, bread);

            // printf("Command: %s Filepath: %s\n Version: %s Start: %d\n Termination Start: %d Clength: %s\n Bytes_read %d\n", REQ.command, REQ.filepath, REQ.version, start, REQ.term_start, clength, bread);
            memset(current_string, 0, sizeof(current_string));
            memset(clength, 0, sizeof(clength));
        }

        close(accepted);
    }

    return 0;
}

//Old Get Algorithm---------------------------------------------------------------------------------------------------------------

// if (type == 1 && clengthexists == false && bread == REQ.term_start + 4) {

//     // printf("%s\n", "we in");
//     int getpath = open(REQ.filepath, O_RDONLY, 0666);
//     if (getpath == -1) {
//         if (errno == ENOENT) {
//             notfound(accepted);
//             close(accepted);
//             continue;
//         } else if (errno == EACCES) {
//             Serv_err(accepted);
//             close(accepted);
//             continue;
//         } else {
//             Serv_err(accepted);
//             close(accepted);
//             continue;
//         }
//     }
//     fstat(getpath, &st);

//     int siz = st.st_size;
//     // printf("%s\n", "we in2");
//     if (S_ISDIR(st.st_mode)) {
//         forbid(accepted);
//         close(accepted);
//         continue;
//     }

//     char beg[BUFF] = "HTTP/1.1 200 OK\r\nContent-Length: ";
//     write(accepted, beg, strlen(beg));
//     char size[siz + 1];
//     sprintf(size, "%d", siz);
//     write(accepted, size, strlen(size));
//     char end[BUFF] = "\r\n\r\n";
//     write(accepted, end, strlen(end));

//     while (1) {
//         bytes_read = read(getpath, beg, BUFF);
//         // printf("%d\n", bytes_read);
//         if (bytes_read == 0) {
//             break;
//         }
//         if (bytes_read == -1) {
//             Serv_err(accepted);
//             close(getpath);
//             close(accepted);
//             continue;
//         }
//         int bytes_written = 0;
//         while (bytes_written < bytes_read) {
//             int written
//                 = write(accepted, beg + bytes_written, bytes_read - bytes_written);
//             if (written == -1) {
//                 Serv_err(accepted);
//                 close(getpath);
//                 close(accepted);
//                 continue;
//             }
//             bytes_written += written;
//         }
//         // printf("%d\n", bytes_written);
//     }
// } else if (type == 1 && clengthexists == true) {
//     bad_request(accepted);
//     // printf("%s\n", "thing2");
//     close(accepted);
//     continue;
// } else if (type == 1 && bread > REQ.term_start + 4) {
//     bad_request(accepted);
//     // printf("%s\n", "thing3");
//     close(accepted);
//     continue;
// }

//Old Put Algorithm ---------------------------------------------------------------------------------------------------------------------------
//Doing a Put Request
// if (type == 2 && clengthexists == false) {
//     // printf("%s\n", "here1");
//     bad_request(accepted);
//     close(accepted);
//     continue;
// } else if (type == 2 && clengthexists == true && bread == REQ.term_start + 4) {
//     // printf("%s\n", "here2");
//     bad_request(accepted);
//     close(accepted);
//     continue;
// } else if (type == 2 && clengthexists == true && bread > REQ.term_start+4) {
//     int putpath = open(REQ.filepath, O_WRONLY | O_TRUNC, 0666);
//     int finding = 0;
//     if (putpath == -1) {
//         if (errno == EACCES) {
//             forbid(accepted);
//             close(accepted);
//             continue;
//         } else {
//             putpath = open(REQ.filepath, O_WRONLY | O_CREAT| O_TRUNC, 0666);
//             if (putpath == -1) {
//                 Serv_err(accepted);
//                 close(accepted);
//                 continue;
//             }
//             finding = 2;
//         }
//     } else {
//         finding = 1;
//     }

//     int leftover = atoi(clength);
//     int written = write_all(putpath, buf + REQ.term_start + 4, leftover);

//     leftover = leftover - written;

//     if (bread < BUFF) {
//         close(putpath);
//         if (finding == 1) {
//             Put_OK(accepted);
//             // memset(current_string, 0, sizeof(current_string));
//             // memset(clength, 0, sizeof(clength));
//             close(accepted);
//             continue;
//         } else {
//             created(accepted);
//             // memset(current_string, 0, sizeof(current_string));
//             // memset(clength, 0, sizeof(clength));
//             close(accepted);
//             continue;
//         }
//     } else {
//         while (1) {
//             int bytes_read = read(accepted, buf, sizeof(buf));
//             if (bytes_read == 0) {
//                 break;
//             }
//             if (bytes_read == -1) {
//                 Serv_err(accepted);
//                 close(putpath);
//                 close(accepted);
//                 continue;
//             }
//             int bytes_written = 0;
//             while (bytes_written < leftover) {
//                 int written
//                     = write(putpath, buf + bytes_written, leftover - bytes_written);
//                 if (written == -1) {
//                     Serv_err(accepted);
//                     close(putpath);
//                     close(accepted);
//                     continue;
//                 }
//                 bytes_written += written;
//             }
//             if (bytes_written == leftover) {
//                 close(putpath);
//                 if (finding == 0) {
//                     Put_OK(accepted);
//                     // memset(current_string, 0, sizeof(current_string));
//                     // memset(clength, 0, sizeof(clength));
//                     close(accepted);
//                     continue;
//                 } else {
//                     created(accepted);
//                     // memset(current_string, 0, sizeof(current_string));
//                     // memset(clength, 0, sizeof(clength));
//                     close(accepted);
//                     continue;
//                 }
//             }
//         }
//     }
// }
