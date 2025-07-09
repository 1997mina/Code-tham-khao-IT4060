#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <poll.h>

#define BUFFER_SIZE 256

int main ()
{
    int main_socket;
    struct sockaddr_in server_address;
    char username[BUFFER_SIZE], message[BUFFER_SIZE];
    struct pollfd pfds[2]; // 0 for stdin, 1 for socket

    printf ("\nWelcome to Trial Version of Chat Room"); // Chào mừng người dùng
    printf ("\nPlease Enter your name to continue: "); // Yêu cầu nhập tên

    fgets (username, BUFFER_SIZE, stdin); // Đọc tên người dùng từ bàn phím
    username[strcspn (username, "\n")] = '\0'; // Xóa ký tự xuống dòng

    main_socket = socket (AF_INET, SOCK_STREAM, 0); // Tạo socket cho client

    server_address.sin_port = htons (2025); // Thiết lập cổng
    server_address.sin_family = AF_INET; // Thiết lập họ địa chỉ
    inet_pton (AF_INET, "127.0.0.1", &server_address.sin_addr); // Thiết lập địa chỉ IP của server

    if (connect (main_socket, (struct sockaddr *) &server_address, sizeof (server_address)) < 0)
    {
        perror ("Error! Cannot connect to server");
        return -1;
    }

    // Nhận thông báo ban đầu từ server để kiểm tra kết nối có thành công không
    int initial_receive = recv (main_socket, message, BUFFER_SIZE, 0); 
    message[initial_receive] = '\0';

    if (!strcmp (message, "Full")) 
    {
        printf ("\nServer is full. Cannot join the chat room.\n"); // Thông báo server đầy

        close (main_socket); // Đóng socket
        return -1;
    }

    printf ("\nYou have joined the chat room\n"); // Thông báo đã tham gia phòng chat
    printf ("Type 'Leave' to leave chat room\n"); // Hướng dẫn cách rời phòng chat

    send (main_socket, username, strlen (username), 0); // Gửi tên người dùng đến server

    printf ("\n\n------------------------- Chat Room -------------------------\n\n"); // In tiêu đề phòng chat

    while (1)
    {
        pfds[0].fd = STDIN_FILENO;
        pfds[0].events = POLLIN;
        pfds[1].fd = main_socket;
        pfds[1].events = POLLIN;

        int activity = poll (pfds, 2, -1); // Wait indefinitely for an event
        if (activity < 0)
            perror ("Poll error");

        if (pfds[1].revents & POLLIN) // If there is data from the server socket
        {
            int receive = recv (main_socket, message, BUFFER_SIZE, 0); // Nhận tin nhắn từ server
            if (receive < 1) 
            {
                printf ("\n\n-------------------------------------------------------------\n\n");
                
                printf ("Server has stopped working\n"); // Thông báo server đã dừng hoạt động
                printf ("Leaving the chat room ...\n"); // Thông báo rời phòng chat
                break;
            }

            message[receive] = '\0'; // Đảm bảo chuỗi kết thúc bằng null
            printf ("%s\n", message); // In tin nhắn nhận được
        }

        if (pfds[0].revents & POLLIN) // If there is data from stdin
        {
            fgets (message, BUFFER_SIZE, stdin);
            message[strcspn (message, "\n")] = '\0';

            printf ("You: %s\n", message);
            send (main_socket, message, strlen (message), 0); // Gửi tin nhắn đến server

            if (!strcmp (message, "Bye"))
            {
                printf ("\n\n-------------------------------------------------------------\n\n");
                
                printf ("Leaving the chat room ...\n"); // Thông báo rời phòng chat
                break; // Thoát vòng lặp
            }
        }
    }

    close (main_socket);
    return 0;
}