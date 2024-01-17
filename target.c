#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <getopt.h>

int main() {
    char buf[32] ;
	ssize_t s, all = 0;

	while ((s = read(0, buf, 31)) > 0) {
		buf[s + 1] = 0x0 ;
		// write(stderr, buf, 32);
		fprintf(stderr, "%s", buf) ;
		// all += s;
	}
	// fprintf(stdout, "size: %ld\n", all);
}