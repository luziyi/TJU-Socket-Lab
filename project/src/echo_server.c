/******************************************************************************
* echo_server.c                                                               *
*                                                                             *
* Description: This file contains the C source code for an echo server.  The  *
*              server runs on a hard-coded port and simply write back anything*
*              sent to it by connected clients.  It does not support          *
*              concurrent clients.                                            *
*                                                                             *
* Authors: Athula Balachandran <abalacha@cs.cmu.edu>,                         *
*          Wolf Richter <wolf@cs.cmu.edu>                                     *
*                                                                             *
*******************************************************************************/

#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "parse.h" 
#define ECHO_PORT 9999
#define BUF_SIZE 4096

#define DEBUG


int close_socket(int sock)
{
    if (close(sock))
    {
        fprintf(stderr, "Failed closing socket.\n");
        return 1;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    int sock, client_sock;
    ssize_t readret;
    socklen_t cli_size;
    struct sockaddr_in addr, cli_addr;
    char buf[BUF_SIZE];

    fprintf(stdout, "----- Echo Server -----\n");
    
    /* all networked programs must create a socket */
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "Failed creating socket.\n");
        return EXIT_FAILURE;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(ECHO_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    /* servers bind sockets to ports---notify the OS they accept connections */
    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)))
    {
        close_socket(sock);
        fprintf(stderr, "Failed binding socket.\n");
        return EXIT_FAILURE;
    }


    if (listen(sock, 5))
    {
        close_socket(sock);
        fprintf(stderr, "Error listening on socket.\n");
        return EXIT_FAILURE;
    }

    /* finally, loop waiting for input and then write it back */
    while (1)
    {
        cli_size = sizeof(cli_addr);
        if ((client_sock = accept(sock, (struct sockaddr *) &cli_addr,&cli_size)) == -1)
        {
            close(sock);
            fprintf(stderr, "Error accepting connection.\n");
            return EXIT_FAILURE;
        }

        readret = 0;

        while((readret = recv(client_sock, buf, BUF_SIZE, 0)) >= 1) // 如果读取成功
        {
#ifdef DEBUG
            fprintf(stdout, "Received %d bytes.\n", (int) readret);
            fprintf(stdout, "Received message: %s\n", buf);
#endif
        // Parse the buffer
            Request* request = parse(buf, readret, client_sock);
            if (request == NULL)
            {
                fprintf(stderr, "Failed to parse request.\n");
                // Send error response to client
                char error_msg[] = "HTTP/1.1 400 Bad Request\r\n\r\n";
                send(client_sock, error_msg, sizeof(error_msg) - 1, 0);
            }
            else
            {
                // Here you can create a response based on the parsed request
                // For simplicity, let's just send back a success message
                char success_msg[] = "HTTP/1.1 200 OK\r\n\r\nParsed Successfully";
                send(client_sock, success_msg, sizeof(success_msg) - 1, 0);

                // Free the request object
                free(request->headers);
                free(request);
            }

       /*     if (send(client_sock, buf, readret, 0) != readret) // 这里是在发送数据，将读取到的buf中的数据发送回去，但是我们要实现将buf中的内容解析之后返回正确的结果。要用到lex和yacc
            {
                close_socket(client_sock);
                close_socket(sock);
                fprintf(stderr, "Error sending to client.\n");
                return EXIT_FAILURE;
            }*/
            memset(buf, 0, BUF_SIZE);
        } 

        if (readret == -1) // 如果读取失败
        {
            close_socket(client_sock);
            close_socket(sock);
            fprintf(stderr, "Error reading from client socket.\n");
            return EXIT_FAILURE;
        }

        if (close_socket(client_sock))
        {
            close_socket(sock);
            fprintf(stderr, "Error closing client socket.\n");
            return EXIT_FAILURE;
        }
    }

    close_socket(sock);

    return EXIT_SUCCESS;
}
