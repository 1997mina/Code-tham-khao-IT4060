#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 2004
#define BUFFER_SIZE 128

pthread_mutex_t lock;

int check_syntax (char *receive_message)
{
    char email[BUFFER_SIZE], password[BUFFER_SIZE];
    FILE *file;
    
    // Không có dấu phẩy, thiếu thông tin
    if (sscanf (receive_message, "%[^,],%[^,]", email, password) != 2)
        return -1;
    // Sai định dạng email
    if (strstr (email, "example.com") == NULL || strchr (email, '@') == NULL)
        return -1;

    file = fopen ("users.txt", "r");

    if (file != NULL)
    {
        char account[BUFFER_SIZE];
        while (fgets (account, sizeof (account), file) != NULL)
        {
            account[strcspn (account, "\n")] = '\0';
            // Email đã tồn tại
            if (strstr (account, email) != NULL)
                return 0;
        }
    }

    fclose (file);

    file = fopen ("users.txt", "a");
    fprintf (file, "%s:%s\n", email, password);

    printf ("Đã lưu tài khoản %s\n", email);
    fclose (file);
    
    return 1;
}

void *client_handler (void *arg) 
{
    int client_socket, byte_received;
    char receive_message[BUFFER_SIZE];

    pthread_mutex_lock (&lock);

    client_socket = *(int *) arg;
    free (arg);

    if ((byte_received = recv (client_socket, receive_message, sizeof (receive_message), 0)) < 0)
    {
        printf ("ERROR Lỗi khi nhận dữ liệu từ người dùng\n");
        pthread_exit (NULL);
    }
    
    receive_message[byte_received] = '\0';

    int check = check_syntax (receive_message);
    if (check < 0)
    {
        char *invalid = "ERROR Cú pháp không hợp lệ";
        send (client_socket, invalid, strlen (invalid), 0);
    }
    else if (check == 0)
    {
        char *exist = "ERROR Email đã tồn tại";
        send (client_socket, exist, strlen (exist), 0);
    }
    else
    {
        char *success = "OK Đăng ký thành công";
        send (client_socket, success, strlen (success), 0);
    }

    close (client_socket);
    pthread_mutex_unlock (&lock);
    pthread_exit (NULL);
}

int main ()
{
    int main_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_size = sizeof (client_address);

    main_socket = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    server_address.sin_port = htons (PORT);
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;

    if (bind (main_socket, (struct sockaddr *) &server_address, sizeof (server_address)) < 0)
    {
        printf ("ERROR Không thể gán địa chỉ IP\n");
        return -1;
    }

    if (listen (main_socket, 5) < 0)
    {
        printf ("ERROR Không thể lắng nghe tới người dùng\n");
        return -1;
    }

    printf ("Máy chủ đang hoạt động ở cổng %d ...\n", PORT);

    while (1)
    {
        int *client_socket = (int *) malloc (sizeof (int));
        *client_socket = accept (main_socket, (struct sockaddr *) &client_address, &client_size);

        if (*client_socket < 1)
        {
            printf ("ERROR Không thể kết nối tới người dùng mới\n");
            break;
        }

        pthread_t thread;
        pthread_create (&thread, NULL, client_handler, client_socket);
        pthread_join (thread, NULL);
    }

    close (main_socket);
    return 0;
}