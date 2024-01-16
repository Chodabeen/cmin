#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <getopt.h>

int input_pipes[2], output_pipes[2];
char str_err[128], output_file[128];


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

void run_target() {

    close(input_pipes[1]) ;
    close(output_pipes[0]) ;
    dup2(input_pipes[0], 0) ;
    dup2(output_pipes[1], 2) ;

	execlp("./target", "./target", (char *)NULL);
    
	exit(0) ;
    
}

// if crash exists return 1, otherwise 0
int check_crashing() {
    char * buf = (char*) malloc(sizeof(char) * 512);
    char * buf_start = buf;
    FILE * stream;

    if((stream = tmpfile()) == NULL) {
        perror( "Could not open new temporary file\n" );
        exit(EXIT_FAILURE);
    }

    // save the sentence of output_pipes[0] to tmpfile
    size_t read_cnt, write_all = 0;
    while ((read_cnt = read(output_pipes[0], buf, 512))) {
        size_t written_acc, written = 0 ;

        for (written_acc = 0 ; written_acc < read_cnt; written_acc += written) {
            written = fwrite(buf, 1, read_cnt - written_acc, stream);
            buf += written ;
        }
        write_all += written_acc;
    }
    // printf("write_all : %zu\n", write_all);

    rewind(stream);
    
    // get a line from tmpfile
    char *line = NULL;
    size_t len = 0;
    while ((read_cnt = getline(&line, &len, stream)) != -1) {
        line[read_cnt] = '\0';
        // check line has str_err
        if (strstr(line, str_err) != NULL) {
            // printf("The substring is present in the sentence.\n");
            fclose (stream);
            return 1;
        } 
    }
    
    free (buf_start);
    fclose (stream);
    return 0;
}

char * reduce(char * t) {
    pid_t child_pid ;
    int exit_code ;
    int i;
    char head[4096], tail[4096], mid[4096];

    fprintf(stderr, "\nREDUCE\nt: %s\n", t);
   
    int s = strlen(t) - 1;
    while (s > 0) {

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

            strncpy(head, t, i - 1 + 1);
            strncpy(tail, t + i + s, strlen(t) - 1 - i - s + 1);
            head[i - 1 + 1] = '\0';
            tail[strlen(t) - 1 - i - s + 1] = '\0';
            strcat(head, tail);

            fprintf(stderr, "i: %d\n", i);
            fprintf(stderr, "head+tail: %s\n", head);

            // run target using head+tail
            if ((child_pid = fork()) == 0) {
            	run_target();
            } else {    
                // parent
                close (input_pipes[0]);
                close (output_pipes[1]);
                // write head+tail to input_pipes[1]
                write_bytes(input_pipes[1], head, strlen(head));
                close (input_pipes[1]);
                wait(&exit_code) ;
            }
            // check head+tail satisfies crash
            if (check_crashing() == 1) {
                fprintf(stderr, "head+tail satisfies crash!!\n");
                close (output_pipes[0]);
                return reduce(head);    // current head has head+tail
            }
            close (output_pipes[0]);
        }

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

            fprintf(stderr, "i: %d\n", i);
            fprintf(stderr, "mid: %s\n", mid);

            strncpy(mid, t + i, s);
            mid[s] = '\0';

            // run target using mid
            if ((child_pid = fork()) == 0) {
            	run_target();
            } else {    
                // parent
                close (input_pipes[0]);
                close (output_pipes[1]);
                // write mid to input_pipes[1]
                write_bytes(input_pipes[1], mid, strlen(mid));
                close (input_pipes[1]);
                wait(&exit_code) ;
            }
            // check mid satisfies crash
            if (check_crashing() == 1) {
                fprintf(stderr, "mid satisfies crash!!\n");
                close (output_pipes[0]);
                return reduce(mid);    // current head has head+tail
            }
            close (output_pipes[0]);
        }

        s -= 1;
    }
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

    // check option
    if (!i_flag || !m_flag || !o_flag) {
        fprintf(stderr, "Error: Missing required options\n");
        fprintf(stderr, "Usage: %s -i input_file -m str_err -o output_file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // read input_file and save them to buf
    char buf[4097];
    // int input_fd = open(input_file, O_RDONLY);
    int input_fd = open("hello.txt", O_RDONLY);
    read_bytes(input_fd, buf, 4096);
    fprintf(stderr, "main: %s\n", buf);

    close (input_fd);

    // open 
    int output_fd = open(output_file, O_CREAT | O_WRONLY, 0644);
    write_bytes(output_fd, minimize(buf), strlen(minimize(buf)));
    close(output_fd);


    
}