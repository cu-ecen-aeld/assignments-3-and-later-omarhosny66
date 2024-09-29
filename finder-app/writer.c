#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>

int main(int argc, char **argv)
{
    FILE *fptr = NULL;

    openlog("", LOG_CONS, LOG_USER);


    if (argc >= 1) {
        fptr = fopen(argv[1],"r");
    } else {
        syslog(LOG_ERR, "Error: Insufficient arguments");
    }

    if(fptr == NULL)
    {
        // printf("Error!");   
        syslog(LOG_ERR, "Error: %s", strerror(errno));
        exit(1);             
    }

    if (argc >= 2) {
        fprintf(fptr,"%s", argv[2]);
        syslog(LOG_DEBUG, "Writing %s to %s", argv[2], argv[1]);
    } else {
        syslog(LOG_ERR, "Error: Insufficient arguments");
    }

    closelog();
    fclose(fptr);

return 0;
}