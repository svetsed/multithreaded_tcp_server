#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>       // для open()
#include <unistd.h>      // для read(), write(), close()
#include <arpa/inet.h>   // для работы с преобразованием IP-адресов из строки в число и обратно, также преобразования порядка байтов
#include <netinet/in.h>  // для работы с структурой sockaddr_in(для описания адреса в IPv4)
#include <sys/socket.h>  // для работы с сокетами, а также типы сокетов(SOCK_STREAM, SOCK_DGRAM, SOCK_RAW)

#define PORT 8080
#define BUFFER_SIZE 1024

volatile sig_atomic_t stop_server = 0;
int log_fd;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void handle_signal(int sig);
void *handle_client(void *arg);

#endif // SERVER_H