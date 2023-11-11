#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include "mytbf.h"

static const int SIZE = 1024;   // buffer size
static const int CPS = 10;      // token release rate
static const int BURST = 100;   // token max

static volatile int token = 0;

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        fprintf(stdout, "Usage...");
        exit(1);
    }

    mytbf_t *tbf;

    tbf = mytbf_init(CPS, BURST);   // init token bucket
    if (tbf == NULL)
    {
        fprintf(stderr, "tbf init error");
        exit(1);
    }

    int sfd, dfd = 0;
    do
    {
        sfd = open(argv[1], O_RDONLY);
        if (sfd < 0)
        {
            if (errno == EINTR)     // if interrupted by signal, continue
                continue;
            fprintf(stderr, "%s\n", strerror(errno));
            exit(1);
        }
    }while(sfd < 0);

    char buf[SIZE];
    
    while(1)
    {
        int len, ret, pos = 0;
        int size = mytbf_fetchtoken(tbf, SIZE);     // try to get SIZE tokens; if tokens not enough, get all existing tokens; if no token, block

        if (size < 0){
            fprintf(stderr, "mytbf_fetchtoken()%s\n", strerror(-size));
            exit(1);
        }

        len = read(sfd, buf, size);
        while (len < 0)
        {
            if (errno == EINTR)
                continue;
            strerror(errno);
            break;
        }

        if (len == 0)
            break;

        if (size - len > 0)
        {
            mytbf_returntoken(tbf, size-len);
        }

        while(len > 0)
        {
            ret = write(dfd, buf+pos, len);
            while (ret < 0)
            {
                if (errno == EINTR)
                    continue;
                printf("%s\n", strerror(errno));
                exit(1);
            }

            pos += ret;
            len -= ret;
        }
    }

    close(sfd);
    mytbf_destroy(tbf);

    exit(0);
}