#ifdef __MVS__
#define _XOPEN_SOURCE 600
#define _OPEN_SYS_FILE_EXT 1
#endif

#include <fcntl.h>
#include <features.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void zos_convert_query(int fd) {
    struct f_cnvrt cvtreq = {QUERYCVT, 0, 0};
    if (fcntl(fd, F_CONTROL_CVT, &cvtreq) < 0) {
        perror("Error getting conversion data.\n");
        close(fd);
        exit(1);
    }
    printf("Terminal %s %d %d %d \n", ttyname(fd), fd, cvtreq.pccsid, cvtreq.fccsid);
}

void zos_convert_set(int fd) {
    struct f_cnvrt cvtreq = {SETCVTON, 0, 1047};
    if (fcntl(fd, F_CONTROL_CVT, &cvtreq) < 0) {
        perror("Error setting conversion data.\n");
        close(fd);
        exit(1);
    }
}

void open_pty_tty(int* master, int* slave) {
    char* slave_name;
    int rc;

    *master = posix_openpt(O_RDWR | O_NOCTTY);
    if (*master == -1) {
        perror("Error opening a terminal.\n");
        close(*master);
        exit(1);
    }

#ifdef __MVS__
    printf("master\n");
    zos_convert_query(*master);
    zos_convert_set(*master);
    zos_convert_query(*master);
#endif

    if ((slave_name = ptsname(*master)) == NULL) {
        perror("ptsname() error");
        close(*master);
        exit(1);
    }
    if (grantpt(*master) == -1) {
        perror("grantpt() error");
        close(*master);
        exit(1);
    }
    if (unlockpt(*master) == -1) {
        perror("unlockpt() error");
        close(*master);
        exit(1);
    }

    if ((*slave = open(slave_name, O_RDWR | O_NOCTTY)) == -1) {
        perror("open()");
        close(*master);
        exit(1);
    }

#ifdef __MVS__
    printf("slave\n");
    zos_convert_query(*slave);
    zos_convert_set(*slave);
    zos_convert_query(*slave);
#endif
}

void print_bytes(char bytes[], long cnt) {
    printf("%zd: %s\n", cnt, bytes);
    for (long i = 0; i < cnt; i++) printf("%x ", (int)bytes[i]);
    printf("\n--------------------------------\n");
}

int main() {
    char str_in[10];
    int p, t;
    char str[] = "0129\b3\n";
    ssize_t bytes_out, bytes_in;
    char buffer[100];
    char termid[1025];

#ifdef __MVS__
    printf("MVS\n");
#endif

    open_pty_tty(&p, &t);

    if ((bytes_out = write(p, str, strlen(str))) == -1) {
        perror("write()");
        close(p);
        close(t);
        return 1;
    }
    printf("Pty write bytes ");
    print_bytes(str, bytes_out);

    if ((bytes_in = read(t, buffer, 100)) == -1) {
        perror("read()");
        close(p);
        close(t);
        return 1;
    }
    printf("Pty read result: ");
    print_bytes(buffer, bytes_in);

    if ((bytes_in = read(p, buffer, 100)) == -1) {
        perror("read()");
        close(p);
        close(t);
        return 1;
    }
    printf("Tty read result: ");
    print_bytes(buffer, bytes_in);

    close(p);
    close(t);
    return 0;
}
