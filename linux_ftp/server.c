#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "config.h"
#include <sys/stat.h>
#include <fcntl.h>

int get_cmd_type(char *cmd)
{
        if (!strcmp(cmd, "ls"))            return LS;
        if (!strcmp(cmd, "pwd"))           return PWD;
        if (!strcmp(cmd, "quit"))          return QUIT;

        if (strstr(cmd, "cd") != NULL)     return CD;
        if (strstr(cmd, "get") != NULL)    return GET;
        if (strstr(cmd, "put") != NULL)    return PUT;

        return 24;
}

char *getDesDir(char *cmd)
{
        char *p = NULL;
        p = strtok(cmd, " ");
        p = strtok(NULL, " ");

        return p;
}
void msg_handler(struct Msg msg, int fd)
{
        int ret;
        int file_fd;
        char dataBuf[1024] = {0};
        char *file = NULL;

        printf("cmd: %s\n", msg.cmd);
        ret = get_cmd_type(msg.cmd);

        switch(ret){
                case LS:
                case PWD:
                        msg.type = 0;
                        FILE *fp = popen(msg.cmd, "r");
                        memset(msg.cmd, 0, sizeof(msg.cmd));
                        fread(msg.cmd, sizeof(msg.cmd), 1, fp);
                        write(fd, &msg, sizeof(msg));
                        //fwrite(&msg, sizeof(msg), 1, fp); 这里不使用fwrite是因为要通过fd将msg内容发送到客户端才能实现
                        pclose(fp);

                        break;
                case CD:
                        msg.type = 1;
                        char *dir = getDesDir(msg.cmd);
                        printf("path_file_name: %s\n", dir);
                        chdir(dir); //这里不能调用system，因为system会重新开辟一个终端来进入目标路径，而服务端当前路径却是没有发生改变的

                        break;
                case GET:
                        file = getDesDir(msg.cmd);
                        printf("file: %s\n", file);

                        if (access(file, F_OK) == -1){
                                strcpy(msg.cmd, "No This File!");
                                write(fd, &msg, sizeof(msg));
                        }
                        else {
                                msg.type = DOFILE;

                                file_fd = open(file, O_RDWR);
                                int N_read = read(file_fd, dataBuf, sizeof(dataBuf));
                                //printf("N_read = %d\n", N_read);
                                //printf("strlen: %ld\n", strlen(dataBuf));
                                close(file_fd);

                                strcpy(msg.cmd, dataBuf);
                                int N_write = write(fd, &msg, sizeof(msg));
                                //printf("N_write = %d\n", N_write);
                        }
                        break;
                case PUT:
                        file = getDesDir(msg.cmd);
                        printf("file: %s\n", file);
                        file_fd = open(file, O_RDWR | O_CREAT, 0666); //注意使用O_CREAT，逗号后面一定要设置文件权限
                        int N_write = write(file_fd, msg.secondBuf, strlen(msg.secondBuf));
                        //printf("write %d bytes, buf: %s\n", N_write, msg.secondBuf);
                        close(file_fd);

                        break;
                case QUIT:
                        printf("Client Quit!\n");
                        exit(0);

        }
}
int main(int argc, char **argv)
{
        int s_fd;
        int c_fd;
        int clen;
        int n_read;
        struct Msg msg;

        struct sockaddr_in s_addr;
        struct sockaddr_in c_addr;
        memset(&s_addr, 0, sizeof(struct sockaddr_in));
        memset(&c_addr, 0, sizeof(struct sockaddr_in));

        if (argc != 3){
                printf("the param is not good!\n");
                exit(-1);
        }

        //socket(创建套接子)
        s_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (s_fd == -1){
                perror("socket error");
                exit(-1);
        }

        s_addr.sin_family = AF_INET;
        s_addr.sin_port = htons(atoi(argv[2]));
        inet_aton(argv[1], &s_addr.sin_addr);

        //bind(配置ip地址和端口号)
        bind(s_fd, (struct sockaddr *)&s_addr, sizeof(struct sockaddr_in));
        //listen(监听网络连接)
        listen(s_fd, 10);
        //accept(连接客户端)
        clen = sizeof(struct sockaddr_in);
        while(1){
                c_fd = accept(s_fd, (struct sockaddr *)&c_addr, &clen);
                if (c_fd == -1){
                        perror("accept error");
                }
                else {
                        printf("get connect: %s\n", inet_ntoa(c_addr.sin_addr));
                }

                if (fork() == 0){
                        while(1){
                                memset(msg.cmd, 0, sizeof(msg.cmd));
                                n_read = read(c_fd, &msg, sizeof(msg));
                                if (n_read == 0){
                                        printf("client out!\n");
                                        break;
                                }
                                else if (n_read > 0){
                                        msg_handler(msg, c_fd);
                                }

                        }
                }

        }

        close(s_fd);
        close(c_fd);

        return 0;
}




