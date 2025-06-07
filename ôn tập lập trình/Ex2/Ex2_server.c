#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 2004
#define BUFFER_SIZE 128

pthread_mutex_t lock;

bool check_phone_number (char *number)
{
    if (strlen (number) < 10 || strlen (number) > 11)
        return false;
    
    for (int i = 0; number[i] != '\0'; ++i)
    {
        if (!isdigit (number[i]))
            return false;
    }

    return true;
}

int check_syntax (char *receive_message)
{
    char username[BUFFER_SIZE], old_number[BUFFER_SIZE], new_number[BUFFER_SIZE];
    char account[BUFFER_SIZE];
    bool is_username_exist = false;
    FILE *file, *temp_file;
    
    // Thiếu dấu |, không đủ 3 phần
    if (sscanf (receive_message, "%[^|]|%[^|]|%[^|]", username, old_number, new_number) != 3)
        return -2;
    // Số điện thoại không hợp lệ
    if (!check_phone_number (old_number) || !check_phone_number (new_number))
        return -2;

    // Cập nhật nội dung file contacts.txt
    file = fopen ("contacts.txt", "r");
    temp_file = fopen ("temp.txt", "w");
    
    if (file == NULL)
        return -1;

    while (fgets (account, sizeof (account), file) != NULL)
    {
        account[strcspn (account, "\n")] = '\0';
        if (strstr (account, username) != NULL)
        {
            is_username_exist = true;
            if (strstr (account, old_number) == NULL)
            {
                remove ("temp.txt");
                return 0;
            }
            else
                fprintf (temp_file, "%s:%s\n", username, new_number);
        }

        else
            fprintf (temp_file, "%s\n", account);
    }

    fclose (file);
    fclose (temp_file);

    remove ("contacts.txt");
    rename ("temp.txt", "contacts.txt");

    // Tài khoản có tồn tại
    if (is_username_exist)
    {
        printf ("Đã cập nhật số điện thoại cho tài khoản %s\n", username);
        return 1;
    }
    else // Tài khoản không tồn tại
        return -1;
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
    if (check == -2)
    {
        char *invalid = "ERROR Cú pháp không hợp lệ";
        send (client_socket, invalid, strlen (invalid), 0);
    }
    else if (check == -1)
    {
        char *exist = "ERROR Tài khoản không tồn tại";
        send (client_socket, exist, strlen (exist), 0);
    }
    else if (check == 0)
    {
        char *exist = "ERROR Số điện thoại cũ không đúng";
        send (client_socket, exist, strlen (exist), 0);
    }
    else
    {
        char *success = "OK Cập nhật thành công";
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