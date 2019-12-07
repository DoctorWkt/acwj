#include <stdio.h>
#include <errno.h>
#include <string.h>
char *filename= "fred";
int main() {
    fprintf(stdout, "Unable to open %s: %s\n", filename, strerror(errno));
    return(0);
}
