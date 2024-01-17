#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <getopt.h>

int input_pipes[2], output_pipes[2];
char str_err[128], output_file[128], target_pg[128], options[64];
pid_t child_pid ;
struct itimerval t ;

// if it was succeed return 0, otherwise return 1
int read_bytes(int fd, void * buf, size_t len) {
    char * p = buf ;
    size_t acc = 0 ;

    while (acc < len) {
        size_t _read ;
        _read = read(fd, p, len - acc) ;
        if (_read == 0) {
            return 1 ;
        }
        // fprintf(stderr, "read: %ld\n", _read);
        p += _read ;
        acc += _read ;
    }
    return 0 ;
}

// if it was succeed return 0, otherwise return 1
int write_bytes(int fd, void * buf, size_t len) {
    char * p = buf ;
    size_t acc = 0 ;

    while (acc < len) {
        size_t written ;
        written = write(fd, p, len - acc) ;
        if (written == 0) {
            return 1 ;
        }
        p += written ;
        acc += written ;
    }
    return 0 ;
}

void handler(int sig) {
    fprintf(stderr, "i will kill your child\nTime out\n");
	kill(child_pid, SIGTERM); 
    exit(EXIT_FAILURE);
}

void run_target() {
    close(input_pipes[1]) ;
    close(output_pipes[0]) ;
    dup2(input_pipes[0], 0) ;
    dup2(output_pipes[1], 2) ;
    close (STDOUT_FILENO);

    execlp(target_pg, target_pg, options, (char *)NULL);
    
	exit(0) ;
}

// if crash exists return 1, otherwise 0
int check_crashing() {
    char buf[4097];
    read_bytes(output_pipes[0], buf, 4097);
    
    if(strstr(buf,str_err) != NULL){
        return 1;
    }
    else{
        return 0;
    }
}

char * reduce(char * t) {
    int exit_code ;
    int i;

    // fprintf(stderr, "REDUCE: %s", t);
    
    int s = strlen(t) - 1;
    while (s > 0) {
        fprintf(stderr, "s: %d\n", s);
        
        char * head = (char*) malloc(sizeof(char) * 4097);
        char * tail = (char*) malloc(sizeof(char) * 4097);

        for (i = 0; i <= strlen(t) - s; i++) {
            // fprintf(stderr, "head+tail i:%d\n", i);
            // make pipes
            if (pipe(input_pipes) != 0) {
                perror("Error") ;
                exit(EXIT_FAILURE) ;
            }
            if (pipe(output_pipes) != 0) {
                perror("Error") ;
                exit(EXIT_FAILURE) ;
            }

            strncpy(head, t, i - 1 + 1);
            strncpy(tail, t + i + s, strlen(t) - 1 - i - s + 1);
            head[i - 1 + 1] = '\0';
            tail[strlen(t) - 1 - i - s + 1] = '\0';
            strcat(head, tail);

            // fprintf(stderr, "head: %s", head);
            // fprintf(stderr, "tail: %slen: %ld", tail, strlen(tail));

            // fprintf(stderr, "i: %d\n", i);
            // fprintf(stderr, "t: %s\n", t);
            // fprintf(stderr, "head+tail: %s", head);

            // run target using head+tail
            if ((child_pid = fork()) == 0) {
            	run_target();
            } else {    
                // parent
                close (input_pipes[0]);
                close (output_pipes[1]);
                // start timer
                alarm(3);
                // write head+tail to input_pipes[1]
                write_bytes(input_pipes[1], head, strlen(head));
                close (input_pipes[1]);
                waitpid(child_pid, &exit_code, 0) ;
                // quit alarm
                alarm(0);
                if (WIFSIGNALED(exit_code) && WTERMSIG(exit_code) == SIGKILL) {
                    printf("Parent: Child process was killed by SIGKILL.\n");
                    exit(EXIT_FAILURE);
                }
            }
            // check head+tail satisfies crash
            if (check_crashing() == 1) {
                // fprintf(stderr, "head+tail satisfies crash!!\n");
                close (output_pipes[0]);
                return reduce(head);    // current head has head+tail
            }
            close (output_pipes[0]);
        }

        free(head);
        free(tail);
        char * mid = (char*) malloc(sizeof(char) * 4097);

        for (i = 0; i <= strlen(t) - s; i++) {
            // make pipes
            if (pipe(input_pipes) != 0) {
                perror("Error") ;
                exit(EXIT_FAILURE) ;
            }
            if (pipe(output_pipes) != 0) {
                perror("Error") ;
                exit(EXIT_FAILURE) ;
            }

            strncpy(mid, t + i, s);
            mid[s] = '\0';

            // fprintf(stderr, "i: %d\n", i);
            // fprintf(stderr, "mid: %s\n", mid);
            // fprintf(stderr, "len: %ld\n", strlen(mid));

            // run target using mid
            if ((child_pid = fork()) == 0) {
            	run_target();
            } else {    
                // parent
                close (input_pipes[0]);
                close (output_pipes[1]);
                // start timer
                alarm(3);
                // write mid to input_pipes[1]
                write_bytes(input_pipes[1], &mid, strlen(mid));
                // exit(EXIT_FAILURE);
                close (input_pipes[1]);
                waitpid(child_pid, &exit_code, 0) ;
                // quit alarm
                alarm(0);
                if (WIFSIGNALED(exit_code) && WTERMSIG(exit_code) == SIGKILL) {
                    printf("Parent: Child process was killed by SIGKILL.\n");
                    exit(EXIT_FAILURE);
                }
            }
            // check mid satisfies crash
            if (check_crashing() == 1) {
                // fprintf(stderr, "mid satisfies crash!!\n");
                close (output_pipes[0]);
                return reduce(mid);    // current head has head+tail
            }
            close (output_pipes[0]);
        }
        free(mid);
        s -= 1;
    }
    fprintf(stderr, "end\n");
    return t;
}

char * minimize(char * t) {
    return reduce(t);
}

int main(int argc, char * argv[]) {

    int opt;
    int i_flag = 0, m_flag = 0, o_flag = 0;
    // input_file: crash, str_err: SEGV, output_file: reduced
    char input_file[128];

    while ((opt = getopt(argc, argv, "i:m:o:")) != -1) {
        switch (opt) {
            case 'i':
                strcpy(input_file, optarg);
                i_flag = 1;
                printf("i: %s\n", input_file);
                break;
            case 'm':
                strcpy(str_err, optarg);
                m_flag = 1;
                printf("m: %s\n", str_err);
                break;
            case 'o':
                strcpy(output_file, optarg);
                o_flag = 1;
                printf("o: %s\n", output_file);
                break;
        }
    }

    strcpy(target_pg, argv[7]);
    if (argc == 9) {
        strcpy(options, argv[8]);
    }

    // check option
    if (!i_flag || !m_flag || !o_flag) {
        fprintf(stderr, "Error: Missing required options\n");
        fprintf(stderr, "Usage: %s -i input_file -m str_err -o output_file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    signal(SIGALRM, handler) ;

    // read input_file and save them to buf
    char buf[4097];
    int input_fd = open(input_file, O_RDONLY);
    read_bytes(input_fd, buf, 4096);
    // fprintf(stderr, "main: %s\n", buf);
    // fprintf(stderr, "size: %ld\n", strlen(buf));

    close (input_fd);

    // open output_file and write reduced sentence
    int output_fd = open(output_file, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    fprintf(stderr, "reduced: %s\n", minimize(buf));
    // write_bytes(output_fd, minimize(buf), strlen(minimize(buf)));
    close(output_fd);
}