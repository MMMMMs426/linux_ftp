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
        if (!strcmp("ls", cmd))            return LS;
        if (!strcmp("pwd", cmd))           return PWD;
        if (!strcmp("quit", cmd))          return QUIT;
        if (!strcmp("lls", cmd))           return LLS;

        //if (strstr(cmd, "cd") != NULL)     return CD;  //注意lcd的判断要放在cd前面，因为cd也是lcd的字串，会造成lcd、cd都会返回6
        if (strstr(cmd, "lcd") != NULL)    return LCD;
        if (strstr(cmd, "cd") != NULL)     return CD;
        if (strstr(cmd, "get") != NULL)    return GET;
        if (strstr(cmd, "put") != NULL)    return PUT;

        return -1;
}

char *getDesDir(char *cmd)
{
        char *p = NULL;
        p = strtok(cmd, " ");
        p = strtok(NULL, " ");

        return p;
}
int cmd_handler(struct Msg msg, int fd)
{
        int ret;
        int file_fd;
        char buf[32];
        char *file = NULL;
        char *des_dir = NULL;

        ret = get_cmd_type(msg.cmd);
        switch(ret){
                case LS:
                case CD:
                case PWD:
                        msg.type = 0;
                        write(fd, &msg, sizeof(msg));
                        break;
                case GET:
                        msg.type = 2;
                        write(fd, &msg, sizeof(msg));
                        break;
                case PUT:
                        strcpy(buf, msg.cmd);
                        file = getDesDir(buf); //这里定义了一个buf作为中间媒介，来获取文件名，因为strtok会破环msg.cmd的完整性，导致发送到服务端的msg.cmd不完整

                        if (access(file, F_OK) == -1){
                                printf("%s not exist!\n", file);
                        }
                        else {
                                file_fd = open(file, O_RDWR);
                                int N_read = read(file_fd, msg.secondBuf, sizeof(msg.secondBuf));
                                //printf("read %d bytes,  %s\n", N_read, msg.secondBuf);
                                write(fd, &msg, sizeof(msg));

                                close(file_fd);
                        }
                        break;
                case LLS:
                        system("ls");
                        break;
                case LCD:
                        des_dir = getDesDir(msg.cmd);
                        chdir(des_dir);
                        break;
                case QUIT:
                        strcpy(msg.cmd, "quit");
                        write(fd, &msg, sizeof(msg));

                        close(fd);
                        exit(0);
        }
        return ret;
}
void handler_server_message(struct Msg msg, int fd)
{
        int n_read;
        int new_fd;
        char *file = NULL;
        struct Msg msgget;

        n_read = read(fd, &msgget, sizeof(msgget));
        //printf("n_read = %d\n", n_read);
        //printf("msg.cmd: %s\n", msgget.cmd);

        if (n_read == 0){
                printf("the server is out so quit!\n");
                exit(-1);
        }
        else if (msgget.type == DOFILE){
                file = getDesDir(msg.cmd);
                new_fd = open(file, O_RDWR | O_CREAT, 0666); //记得是","后面加文件访问权限
                //printf("new_fd = %d\n", new_fd);
                int N_write = write(new_fd, msgget.cmd, strlen(msgget.cmd));
                //printf("write: %d bytes, buf: %s\n", N_write, msgget.cmd);
                close(new_fd);

                putchar('>');
                fflush(stdout);
        }
        else {
                printf("---------------------------------------------\n");
                printf("\n%s\n", msgget.cmd);
                printf("---------------------------------------------\n");

                putchar('>');
                fflush(stdout);
        }
}
int main(int argc, char **argv)
{
        int c_fd;
        int clt;
        int ret;
        int len;

        struct Msg msg;

        struct sockaddr_in c_addr;
        memset(&c_addr, 0, sizeof(struct sockaddr_in));

        if (argc != 3){
                printf("param is wrong!\n");
                exit(-1);
        }

        //socket(创建套接字)
        c_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (c_fd == -1){
                perror("socket error");
                exit(-1);
        }

        c_addr.sin_family = AF_INET;
        c_addr.sin_port = htons(atoi(argv[2]));
        inet_aton(argv[1], &c_addr.sin_addr);

        //connect(客户端连接)
        clt = connect(c_fd, (struct sockaddr *)&c_addr, sizeof(struct sockaddr_in));
        if (clt == -1){
                printf("connect error!\n");
                exit(-1);
        }
        else {
                printf("connect with server!\n");
                while(1){
                        memset(msg.cmd, '\0', sizeof(msg.cmd));
                        putchar('>');
                        fflush(stdout);
                        fgets(msg.cmd, 1024, stdin);  //fgets只能读取N-1个字符，包括最后的'\n'，读完结束后系统将自动在最后加'\0'
                        len = strlen(msg.cmd);
                        //printf("len = %d\n", len);

                        if (msg.cmd[len - 1] == '\n'){   //去掉换行符
                                msg.cmd[len - 1] = '\0';
                        }
                        //gets(msg.cmd);  //gets读完结束后系统自动会将'\n'置换成'\0'
                        //当输入字符个数不超过1024时，fgets()是可以跟gets()等价使用的，只不过会有去换行符的操作
                        //printf("cmd: %s\n", msg.cmd);

                        ret = cmd_handler(msg, c_fd);
                        //printf("ret = %d\n", ret);
                        if (ret > IFGO){
                                putchar('>');
                                fflush(stdout);
                                continue;
                        }
                        else if (ret == -1){
                                printf("Sorry command not found!\n");
                                putchar('>');
                                fflush(stdout);
                                continue;
                        }

                        handler_server_message(msg, c_fd);

                }
        }
        return 0;
}
                                             

