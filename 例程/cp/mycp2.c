#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFSIZE 1024

int main(int argc, char **argv)
{
    int sfd, dfd;
    char buf[BUFSIZE];
    int len, ret, pos;

    if (argc < 3)
    {
        fprintf(stderr, "Usage:%s <src_file> <dest_file>\n", argv[0]);
        exit(1);
    }

    if ((sfd = open(argv[1], O_RDONLY)) < 0)
    {
        perror("open()");
        exit(1);
    }
    if ((dfd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0600)) < 0)
    {
        close(sfd);
        perror("open()");
        exit(1);
    }

    while (1)
    {
        if ((len = read(sfd, buf, BUFSIZE)) < 0)
        {
            perror("read()");
            break;
        }
        if (len == 0)
            break;

        pos = 0;
        while (len > 0)
        {
            if ((ret = write(dfd, buf + pos, len)) < 0)
            {
                perror("write()");
                exit(1);
            }
            pos += ret;
            len -= ret;
        }
    }

    close(dfd);
    close(sfd);

    exit(0);
}