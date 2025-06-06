#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 128

int main ()
{
    int main_socket;
    struct sockaddr_in server_address;
    char email[BUFFER_SIZE], password[BUFFER_SIZE];
    char send_message[BUFFER_SIZE], response[BUFFER_SIZE];

    main_socket = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

    server_address.sin_port = htons (2004);
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr ("127.0.0.1");

    if (connect (main_socket, (struct sockaddr *) &server_address, sizeof (server_address)) < 0)
    {
        printf ("Không thể kết nối tới máy chủ");
        return -1;
    }

    printf ("\nNhập tài khoản mới: ");
    fgets (email, BUFFER_SIZE, stdin);
    email[strcspn (email, "\r\n")] = '\0';

    printf ("Mật khẩu: ");
    fgets (password, BUFFER_SIZE, stdin);
    password[strcspn (password, "\r\n")] = '\0';

    sprintf (send_message, "%s,%s", email, password);
    send (main_socket, send_message, strlen (send_message), 0);

    int byte_received = recv (main_socket, response, sizeof (response), 0);
    if (byte_received < 1)
    {
        printf ("Có lỗi xảy ra ở máy chủ\n");
        return -1;
    }

    response[byte_received] = '\0';

    printf ("\n%s\n", response);
    
    close (main_socket);
    return 0;
}