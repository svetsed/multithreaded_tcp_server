#include "server.h"

int main() {
    int server;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Открываем (или создаём) лог-файл server.log для записи (режим добавления в конец)
    log_fd = open("server.log", O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (log_fd < 0) {
        perror("Ошибка при открытии лог-файла");
        exit(EXIT_FAILURE);
    }

    // Создаем новый сокет с IPv4(AF_INET), TCP(SOCK_STREAM)
    if ((server = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Ошибка при создании сокета");
        exit(EXIT_FAILURE);
    }

    // Инициализируем структуру адреса сервера
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // задаем IP-адрес, на котором сервер будет принимать соединения
    // INADDR_ANY - означает, что сервер будет принимать соединения на всех IP-адресах, подключенных к серверу
    server_addr.sin_port = htons(PORT); // задаем порт на котором сервер будет слушать входящие подключения
    // host to network short - преобразует порт из хостового порядка байтов в сетевой порядок байтов, для корректной передачи по сети
    
    // Привязываем сокет к адресу сервера
    if (bind(server, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Ошибка при привязке (bind) сокета к адресу сервера");
        close(server);
        exit(EXIT_FAILURE);
    }

    // Начинаем слушать входящие соединения
    if (listen(server, 5) < 0) {
        perror("Ошибка при слушании (listen) входящих соединений");
        close(server);
        exit(EXIT_FAILURE);
    }

    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    printf("Сервер запущен и слушает порт %d...\n", PORT);
    printf("Для завершения работы нажмите Ctrl+C\n");
    
    while (!stop_server) {
        int *client = malloc(sizeof(int));
        if (client == NULL) {
            perror("Ошибка при выделении памяти для сокета");
            continue;
        }

        *client = accept(server, (struct sockaddr *)&client_addr, &client_addr_len);
        if (*client < 0) {
            if (stop_server) {
                free(client);
                break;
            }
            perror("Ошибка при принятии (accept) входящего соединения");
            free(client);
            continue;

        }

        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, client) != 0) {
            perror("Ошибка при создании потока");
            close(*client);
            free(client);
            continue;
        }

        pthread_detach(thread);
    }

    // Закрытие серверного сокета и лог-файла
    printf("\nЗавершение работы сервера...\n");
    close(server);
    close(log_fd);
    sleep(3);
    printf("Работа сервера завершена.\n");

    return 0;

}

void handle_signal(int sig) {
    if (sig == SIGINT) {
        stop_server = 1;
    }
}

void *handle_client(void *arg) {
    printf("Поток %lu обрабатывает нового клиента\n", pthread_self());
    int client = *(int *)arg;
    free(arg); // Освобождаем выделенную ранее память для дескриптора сокета
    char buffer[BUFFER_SIZE];
    ssize_t message_read;

    // Устанавливаем таймаут
    struct timeval tv = { .tv_sec = 15, .tv_usec = 0 };
    if (setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Ошибка установки таймаута");
    }

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        message_read = read(client, buffer, BUFFER_SIZE - 1);

        if (message_read <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("Таймаут соединения\n");
            } else {
                perror("Клиент отключился");
            }
            break;
        }

        buffer[message_read] = '\0';

        // Получаем текущее время для лога
        time_t now = time(NULL);
        char time_str[50];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));

        // Безопасная запись в лог (с мьютексом)
        pthread_mutex_lock(&log_mutex);
        dprintf(log_fd, "[%s] %s\n", time_str, buffer);
        pthread_mutex_unlock(&log_mutex);

        // Отправка подтверждения
        const char* confirm = "Сообщение получено\n";
        if (write(client, confirm, strlen(confirm)) < 0) {
            perror("Ошибка отправки подтверждения");
            break;
        }
    }

    close(client);
    pthread_exit(NULL);
}
