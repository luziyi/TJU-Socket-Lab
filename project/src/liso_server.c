/******************************************************************************
 * echo_server.c                                                               *
 *                                                                             *
 * Description: This file contains the C source code for an echo server.  The  *
 *              server runs on a hard-coded port and simply write back anything*
 *              sent to it by connected clients.  It supports persistent       *
 *              connections.                                                   *
 *                                                                             *
 * Authors: Athula Balachandran <abalacha@cs.cmu.edu>,                         *
 *          Wolf Richter <wolf@cs.cmu.edu>                                     *
 *                                                                             *
 *******************************************************************************/

#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include<time.h>

#include "response.h"
#include "logger.h"
#define ECHO_PORT 9999
#define BUF_SIZE 81920
 
// #define DEBUG

char message_buffer[BUF_SIZE * 2]; // 一个足够大的缓冲区来保存累积的数据
int message_length = 0;
int bias = 0;
int close_socket(int sock )
{
    if (close(sock))
    {
        log("error.log", "[INFO] Failed closing socket.\n", LOG_LEVEL_ERROR, NULL);
        fprintf(stderr, "Failed closing socket.\n");
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    time_t now;
    char *temp_log;
    char *access_log;
    int sock, client_sock;
    char *log_message;
    ssize_t readret;
    socklen_t cli_size;
    struct sockaddr_in addr, cli_addr;
    char buf[BUF_SIZE];

   fprintf(stdout, "========== Echo Server ==========\n");
        
     
    /* all networked programs must create a socket */
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        time(&now);
        fprintf(stderr, "Failed creating socket.\n");
        log_message = malloc(100);
        memset(log_message, 0, 100);
        sprintf(log_message, "[DATE]%s [INFO] Failed creating socket." );
        log("error.log", log_message, LOG_LEVEL_ERROR, NULL);
        return EXIT_FAILURE;
    }
 
    addr.sin_family = AF_INET;
    addr.sin_port = htons(ECHO_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    /* servers bind sockets to ports---notify the OS they accept connections */
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)))
    {
        close_socket(sock );
        fprintf(stderr, "Failed binding socket.\n");
        log_message = malloc(100);
        memset(log_message, 0, 100);
        sprintf(log_message, "[DATE]%s [INFO] Failed binding socket." );
        log("error.log", log_message, LOG_LEVEL_ERROR, NULL);
        return EXIT_FAILURE;
    }
    /* 服务器监听套接字以等待连接请求 */
    if (listen(sock, 5)) // listen
    {
        close_socket(sock );
        fprintf(stderr, "Error listening on socket.\n");
        log_message = malloc(100);
        memset(log_message, 0, 100);
        sprintf(log_message, "[DATE]%s [INFO] Error listening on socket." );
        log("error.log", log_message, LOG_LEVEL_ERROR, NULL);
        return EXIT_FAILURE;
    }
    // 获取t
      
    /* finally, loop waiting for input and then write it back */
   
    while (1)
    {
        /**
         * 从客户端接受传入的连接。
         *
         * @param sock 服务器套接字描述符。
         * @param cli_addr 客户端地址结构。
         * @param cli_size 客户端地址结构的大小。
         * @return 如果成功，则返回客户端套接字描述符；如果发生错误，则返回-1。
         */
        cli_size = sizeof(cli_addr);
        if ((client_sock = accept(sock, (struct sockaddr *)&cli_addr, &cli_size)) == -1)
        {
            close(sock);
            fprintf(stderr, "Error accepting connection.\n");
            log_message = malloc(100);
            memset(log_message, 0, 100);
            sprintf(log_message, "[DATE]%s [INFO] Error accepting connection." );
            log("error.log", log_message, LOG_LEVEL_ERROR,  NULL);
            return EXIT_FAILURE;
        }
     
        readret = 0;
     
        // 获取客户端ip
        char client_ip[2000];
        char *tmp=inet_ntop(AF_INET, &cli_addr.sin_addr, client_ip, sizeof(client_ip));
       
       if (inet_ntop(AF_INET, &cli_addr.sin_addr, client_ip, sizeof(client_ip)) == NULL) {
             perror("inet_ntop failed"); // 如果inet_ntop失败，输出错误信息
            exit(EXIT_FAILURE);
}     
       // printf(tmp);
        inet_ntop(AF_INET, &cli_addr.sin_addr, client_ip, sizeof(client_ip));
 
        char client_info[5000];
     
        snprintf(client_info, "[ClientIP:port]%s:%d\n", client_ip, ECHO_PORT);
 
        while ((readret = recv(client_sock, buf, BUF_SIZE, 0)) >= 1)
        { 
#ifdef DEBUG
            fprintf(stdout, "Received %d bytes.\n", (int)readret);
            fprintf(stdout, "Received message: %.*s\n", (int)readret, buf);
#endif

            // 将接收到的数据追加到消息缓冲区
            if (message_length + readret < sizeof(message_buffer))
            {
                memcpy(message_buffer + message_length, buf, readret);
                message_length += readret;
                message_buffer[message_length] = '\0'; // 确保缓冲区以NULL结尾
            }
            else
            {
                fprintf(stderr, "Message buffer overflow.\n");
                log_message = malloc(100);
                memset(log_message, 0, 100);
                sprintf(log_message, "[DATE]%s [INFO] Message buffer overflow.");
                log("error.log", log_message, LOG_LEVEL_ERROR, client_ip);
                break;
            }
  
            char response_buffer[1024];
            strncpy(response_buffer, message_buffer + bias, 1024);
            // 如果message_buffer末尾的4个字符是\r\n\r\n，则表示消息结束
            char *end_of_message = strstr(response_buffer, "\r\n\r\n");
            // 将第一个请求放入待解析区域
            while (end_of_message != NULL)
            { 
                void *response_message;
                int complete_message_length = end_of_message - response_buffer + 4;         // 计算完整消息的长度
                temp_log = Response(response_buffer, complete_message_length, client_sock); // 解析完整的HTTP请求
        
                log_message = malloc(strlen(client_info) +   strlen(temp_log) + 16);
           
                snprintf(log_message, "[DATE]%s %s [INFO] %s",   client_info, temp_log);
        
                log("access.log", temp_log, LOG_LEVEL_INFO, client_ip);
           
                free(temp_log);
                bias += complete_message_length;
                memset(response_buffer, 0, 1024);
                strncpy(response_buffer, message_buffer + bias, 1024);
             
                end_of_message = strstr(response_buffer, "\r\n\r\n");
            }
      
            if (readret == -1) // 如果读取失败
            {
                close_socket(client_sock );
                close_socket(sock );
                fprintf(stderr, "Error reading from client socket.\n");
                return EXIT_FAILURE;
            }

            if (close_socket(client_sock ))
            {
                close_socket(sock );
                fprintf(stderr, "Error closing client socket.\n");
                return EXIT_FAILURE;
            }
        }
    }

    close_socket(sock );

    return EXIT_SUCCESS;
}
