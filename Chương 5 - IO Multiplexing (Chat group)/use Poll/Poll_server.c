#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <poll.h>

#define PORT 2025
#define BUFFER_SIZE 256
#define MAX_CLIENTS 3

// Cấu trúc để lưu trữ thông tin client
struct Clients
{
    int socket;
    char name[BUFFER_SIZE];
};

void broadcast_message (struct Clients clients[], int num_clients, int sender_socket, const char *message) // Hàm gửi tin nhắn tới tất cả các client trừ người gửi
{
    for (int i = 0; i < num_clients; i++)
    {
        if (clients[i].socket != sender_socket)
            send (clients[i].socket, message, strlen (message), 0);
    }
}

void remove_client (struct Clients clients[], int i, struct pollfd pfds[], int *num_pfds) // Hàm xóa client khỏi danh sách và thông báo cho các client khác
{
    char notification[BUFFER_SIZE];

    close (pfds[i + 1].fd); // Đóng socket (pfds[0] là server_socket)

    printf ("%s has left the chat\n", clients[i].name);

    // Thông báo cho các client khác
    sprintf (notification, " *) Notification: %s has left the chat", clients[i].name); // Prepare notification before client is removed from array
    broadcast_message (clients, *num_pfds - 1, clients[i].socket, notification);

    // Xóa client khỏi mảng bằng cách thay thế bằng client cuối cùng và cập nhật pollfd
    clients[i] = clients[*num_pfds - 2]; // Di chuyển client cuối cùng đến vị trí của client bị xóa (trừ server và client cuối cùng)
    pfds[i + 1] = pfds[*num_pfds - 1]; // Move the last pollfd to the removed pollfd's position
    (*num_pfds)--;
}

int main ()
{
    int server_socket, num_pfds = 1; // server_socket: socket của server, num_clients: số lượng client hiện tại, num_pfds: số lượng pollfd hiện tại (server + clients)
    struct sockaddr_in server_address, client_address; // server_address: địa chỉ server, client_address: địa chỉ client
    struct Clients clients[MAX_CLIENTS]; // Mảng lưu trữ thông tin các client (MAX_CLIENTS)
    char client_message[BUFFER_SIZE], notification[BUFFER_SIZE]; // client_message: tin nhắn từ client, notification: thông báo

    server_socket = socket (AF_INET, SOCK_STREAM, 0); // Tạo socket cho server

    server_address.sin_port = htons (PORT); // Thiết lập cổng
    server_address.sin_family = AF_INET; // Thiết lập họ địa chỉ
    server_address.sin_addr.s_addr = INADDR_ANY; // Chấp nhận kết nối từ bất kỳ địa chỉ nào

    if (bind (server_socket, (struct sockaddr *) &server_address, sizeof (server_address)) == -1)
    // Gán địa chỉ cho socket server
    {
        perror ("Error! Cannot bind the address");
        return -1;
    }

    listen (server_socket, MAX_CLIENTS); // Lắng nghe kết nối đến, MAX_CLIENTS là số lượng kết nối tối đa trong hàng đợi
    printf ("\nChat Room Server is Listening on port %d ... \n", PORT);

    struct pollfd pfds[MAX_CLIENTS + 1]; // Mảng các cấu trúc pollfd (+1 cho socket server)
    pfds[0].fd = server_socket;
    pfds[0].events = POLLIN; // Kiểm tra dữ liệu đến

    while (1)
    {
        int activity = poll (pfds, num_pfds, -1); // Chờ đợi sự kiện vô thời hạn
        if (activity < 0)
        {
            perror ("Poll error");
            continue;
        }

        // Check for activity on the server socket (new connection)
        if (pfds[0].revents & POLLIN) // Nếu có hoạt động trên socket server (kết nối mới)
        {
            socklen_t address_length = sizeof (client_address);
            int new_socket = accept (server_socket, (struct sockaddr *) &client_address, &address_length); // Chấp nhận kết nối mới

            if (new_socket < 0)
            {
                perror ("Error! Cannot accept new connection\n");
                continue;
            }
            // Kiểm tra số lượng client đã đạt giới hạn chưa
            if (num_pfds - 1 < MAX_CLIENTS) // Nếu số lượng client chưa đạt giới hạn
            {
                send (new_socket, "Welcome", 7, 0); // Gửi thông báo chào mừng
                clients[num_pfds - 1].socket = new_socket; // Lưu socket của client mới
                clients[num_pfds - 1].name[0] = '\0'; // Đánh dấu là chưa có tên (tên sẽ được nhận sau)

                // Nhận tên người dùng từ client mới kết nối
                int byte_received = recv (new_socket, clients[num_pfds - 1].name, BUFFER_SIZE, 0);
                clients[num_pfds - 1].name[byte_received] = '\0';

                printf ("%s has joined the chat\n", clients[num_pfds - 1].name);

                // Chuẩn bị thông báo client mới đã tham gia
                sprintf (notification, " *) Notification: %s has joined the chat", clients[num_pfds - 1].name); 
                // Gửi thông báo tới các client khác (trừ client mới) (num_clients chưa tăng nên vẫn là số lượng client cũ)
                broadcast_message (clients, num_pfds - 1, new_socket, notification);

                // Thêm socket client mới vào mảng pollfd
                pfds[num_pfds].fd = new_socket; 
                pfds[num_pfds].events = POLLIN;
                num_pfds++;
            }
            else
            {
                send (new_socket, "Full", 4, 0);
                printf ("Too many clients, rejecting connection.\n"); // Thông báo server đầy
    
                close (new_socket); // Đóng socket của client bị từ chối
            }
        }

        // Check for activity on client sockets
        for (int i = 0; i < num_pfds - 1; i++) // Lặp qua các client (pfds[0] là server_socket)
        {
            if (pfds[i + 1].revents & POLLIN) // Nếu có dữ liệu từ client (pfds[i+1] tương ứng với clients[i])
            {
                int receive = recv (clients[i].socket, client_message, sizeof (client_message), 0); // Nhận tin nhắn từ client
                if (receive <= 0) // Xử lý client ngắt kết nối hoặc lỗi
                {
                    remove_client (clients, i, pfds, &num_pfds);
                    i--; // Giảm i để kiểm tra lại vị trí hiện tại (vì client cuối cùng đã di chuyển lên)
                }
                else // Nếu nhận được tin nhắn
                {
                    client_message[receive] = '\0';
                    if (strcmp (client_message, "Bye") == 0) // Nếu client gửi "Bye", xử lý rời phòng
                    { 
                        remove_client (clients, i, pfds, &num_pfds);
                        i--;
                    }
                    else
                    {
                        char send_message[BUFFER_SIZE];
                        sprintf (send_message, "%s: %s", clients[i].name, client_message);
                        printf ("%s\n", send_message); // In tin nhắn lên server console

                        broadcast_message (clients, num_pfds - 1, clients[i].socket, send_message); // Gửi tin nhắn tới các client khác
                    }
                }
            }
        }
    }

    close (server_socket); // Đóng socket server
    return 0;
}