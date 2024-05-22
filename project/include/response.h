#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "parse.h"
#include <netinet/in.h>
#include <netinet/ip.h>
#include <fcntl.h>

#define MAX_MESSAGE_LENGTH 4096
#define GET_SUCCESS 1
#define GET_FAILURE 0
#define URL_MAX_SIZE 256
#define O_RDONLY 00
#define S_ISREG 0100000
#define S_IRUSR 00400
#define BUF_SIZE 4096

char RESPONSE_400[4096] = "HTTP/1.1 400 Bad request\r\n\r\n";
char RESPONSE_404[4096] = "HTTP/1.1 404 Not Found\r\n\r\n";
char RESPONSE_501[4096] = "HTTP/1.1 501 Not Implemented\r\n\r\n";
char RESPONSE_505[4096] = "HTTP/1.1 505 HTTP Version not supported\r\n\r\n";
char RESPONSE_200[4096] = "HTTP/1.1 200 OK\r\n\r\n";
char http_version_now[50] = "HTTP/1.1";
char root_path[50] = "./static_site";
char file_path[50] = "/index.html";


// char* convertTimestampToDate(time_t timestamp) {
//     // 定义一个结构体用于存放格式化的日期和时间
//     static char formattedDate[80]; // 静态分配以避免返回局部变量的地址
//     struct tm* timeinfo;

//     // 将时间戳转换为本地时间
//     timeinfo = gmtime(&timestamp);

//     // 使用strftime函数格式化日期和时间
//     strftime(formattedDate, sizeof(formattedDate), "%a, %d %b %Y %H:%M:%S GMT", timeinfo);

//     return formattedDate;
// }

char *Response(char *message_buffer, int complete_message_length, int client_sock)
{
    // struct stat buf;
    // stat("./static_site/index.html", &buf);
    // printf("File size: %ld\n", buf.st_size);
    // char* formattedDate = convertTimestampToDate(buf.st_mtime);

    char *response_message;
    Request *request = parse(message_buffer, complete_message_length, client_sock);
    
    if (request == NULL)
    {
        response_message = RESPONSE_400;
    }
    else if (strcmp(request->http_version, "HTTP/1.1") != 0)
    {
        response_message = RESPONSE_505;
        printf(response_message);
    }
    else if (strcmp(request->http_method, "GET") == 0)
    {
            char tmpbuf[BUF_SIZE];
        // Handle GET request
            char head_URL[BUF_SIZE];
            memset(head_URL, 0, sizeof(head_URL));
            strcat(head_URL, root_path);
            int head_flag = GET_SUCCESS;
            if (strcmp(request->http_uri, "/") == 0)
            {
                strcat(head_URL, file_path);
            }
            else if (sizeof(request->http_uri) + sizeof(root_path) < URL_MAX_SIZE)
            {
                strcat(head_URL, request->http_uri);
            }
            else
            {
                head_flag = GET_FAILURE;
                memset(tmpbuf, 0, sizeof(tmpbuf));
                memcpy(tmpbuf, "HTTP/1.1 404 Not Found\r\n\r\n", sizeof("HTTP/1.1 404 Not Found\r\n\r\n"));
                // send(client_sock, buf, readret, 0);
                response_message=tmpbuf;
            }
            
            struct stat *file_state = (struct stat *)malloc(sizeof(struct stat));
            if (stat(head_URL, file_state) == -1)
            {
                response_message = RESPONSE_404;
            }
            
            int fd_in = open(head_URL, O_RDONLY);
         
            
            if (fd_in < 0)
            {
                printf("Failed to open the file\n");
                return GET_FAILURE;
            }

            char response1[BUF_SIZE];
            char response2[BUF_SIZE];
            ssize_t bytesRead = read(fd_in, response2, BUF_SIZE);
            if (bytesRead < 0)
            {
                printf("Failed to read the file\n");
                close(fd_in);
                free(file_state);
                return GET_FAILURE;
            }
            
            memcpy(response1, "HTTP/1.1 200 OK\r\n\r\n", sizeof(response1));
            strncat(response1, response2, bytesRead);
            //send(client_sock, response1, bytesRead + strlen("HTTP/1.1 200 OK\r\n\r\n"), 0);
            response_message=response1;
            close(fd_in);
            free(file_state);
               
            free(request->headers);
            free(request);

       /* FILE *file = fopen("/webServerStartCodes-new/static_site/index.html", "r");
        if (file == NULL) {
            response_message = RESPONSE_404;
        } else {
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            fseek(file, 0, SEEK_SET);
            
            response_message = (char *) malloc(file_size + sizeof(RESPONSE_200));
            strcpy(response_message, RESPONSE_200);
            fread(response_message + sizeof(RESPONSE_200) - 1, file_size, 1, file);
            fclose(file);
        }   */
    }
    else if (strcmp(request->http_method, "HEAD") == 0)
    {
        response_message = "HTTP/1.1 200 OK\r\n\r\n";;
    }
    else if (strcmp(request->http_method, "POST") == 0)
    {
        // Handle POST request
        response_message = message_buffer;
    }
    else
    {
        response_message = RESPONSE_501;
    }
    
    free(request);
    printf("Response message: %s\n", response_message);
    return response_message;
}
