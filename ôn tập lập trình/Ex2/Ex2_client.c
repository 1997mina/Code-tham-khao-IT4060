#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 128

int main () 
{
    int main_socket;
    struct sockaddr_in server_address;
    char send_message[BUFFER_SIZE], response[BUFFER_SIZE];
    char username[BUFFER_SIZE], old_number[BUFFER_SIZE], new_number[BUFFER_SIZE];

    printf ("Nhập tài khoản: ");
    fgets (username, sizeof (username), stdin);
    username[strcspn (username, "\r\n")] = '\0';

    printf ("Nhập số điện thoại cũ: ");
    fgets (old_number, sizeof (old_number), stdin);
    old_number[strcspn (old_number, "\r\n")] = '\0';

    printf ("Nhập số điện thoại mới: ");
    fgets (new_number, sizeof (new_number), stdin);
    new_number[strcspn (new_number, "\r\n")] = '\0';

    main_socket = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons (2004);
    server_address.sin_addr.s_addr = inet_addr ("127.0.0.1");

    if (connect (main_socket, (struct sockaddr *) &server_address, sizeof (server_address)) < 0) 
    {
        printf ("Không thể kết nối tới server\n");
        return -1;
    }

    snprintf (send_message, sizeof (send_message), "%s|%s|%s", username, old_number, new_number);
    send (main_socket, send_message, strlen (send_message), 0);

    int bytes_received = recv (main_socket, response, sizeof (response), 0);
    if (bytes_received < 1)
    {
        printf ("Có lỗi xảy ra ở máy chủ\n");
        return -1;
    }
        
    response[bytes_received] = '\0';
    printf ("Phản hồi từ server: %s\n", response);

    close (main_socket);
    return 0;
}