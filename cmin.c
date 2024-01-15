#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <getopt.h>

int pipes[2];
int fd;

void child_proc() {

    close(pipes[0]) ;
    dup2(fd, 0);
    dup2(pipes[1], 2);

	execlp("./target", "./target", (char *)NULL);
    
	exit(0) ;
}


int main(int argc, char * argv[]) {

    int opt;
    // input_file: crash, str_err: SEGV, output_file: reduced
    char input_file[128], str_err[128], output_file[128];

    while ((opt = getopt(argc, argv, "i:m:o:")) != -1) {
        switch (opt) {
            case 'i':
                strcpy(input_file, optarg);
                printf("i: %s\n", input_file);
                break;
            case 'm':
                strcpy(str_err, optarg);
                printf("m: %s\n", str_err);
                break;
            case 'o':
                strcpy(output_file, optarg);
                printf("o: %s\n", output_file);
                break;
        }
    }

    if (pipe(pipes) != 0) {
		perror("Error") ;
		exit(EXIT_FAILURE) ;
	}

	// printf("%d %d\n", pipes[0], pipes[1]) ;

    pid_t child_pid ;
    int exit_code ;

    fd = open("hello.txt", O_RDONLY);

    child_pid = fork() ;

    if (child_pid == 0) {
		child_proc() ;
	}

    char buf[32] ;
	ssize_t s ;

    while ((s = read(pipes[0], buf, 31)) > 0) {
		buf[s + 1] = 0x0 ;
		printf(">%s\n", buf) ;
	}

    wait(&exit_code) ;


}