#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 2025
#define BUFFER_SIZE 256
#define MAX_CLIENTS 3

// Cấu trúc để lưu trữ thông tin client
struct Clients
{
    int socket;
    char name[BUFFER_SIZE];
};

void broadcast_message (struct Clients clients[], int num_clients, int sender_socket, const char *message)
// Hàm gửi tin nhắn tới tất cả các client trừ người gửi
{
    for (int i = 0; i < num_clients; i++)
    {
        if (clients[i].socket != sender_socket)
            send (clients[i].socket, message, strlen (message), 0);
    }
}

void remove_client (struct Clients clients[], int i, int *num_clients, fd_set *read_fds) 
// Hàm xóa client khỏi danh sách và thông báo cho các client khác
{
    char notification[BUFFER_SIZE];

    // Đóng socket và xóa khỏi tập read_fds
    close (clients[i].socket);
    FD_CLR (clients[i].socket, read_fds);

    printf ("%s has left the chat\n", clients[i].name);

    // Thông báo cho các client khác
    sprintf (notification, " *) Notification: %s has left the chat", clients[i].name); // Prepare notification before client is removed from array
    broadcast_message (clients, *num_clients, clients[i].socket, notification);

    // Xóa client khỏi mảng bằng cách thay thế bằng client cuối cùng
    clients[i] = clients[*num_clients - 1];
    (*num_clients)--;
}

int main ()
{
    int server_socket, max_fd, num_clients = 0; // server_socket: socket của server, max_fd: giá trị file descriptor lớn nhất, num_clients: số lượng client hiện tại
    struct sockaddr_in server_address, client_address; // server_address: địa chỉ server, client_address: địa chỉ client
    struct Clients clients[MAX_CLIENTS]; // Mảng lưu trữ thông tin các client
    fd_set read_fds; // Tập hợp các file descriptor để kiểm tra đọc
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

    max_fd = server_socket;

    while (1)
    {
        max_fd = server_socket; // Khởi tạo lại max_fd trong mỗi vòng lặp
        FD_ZERO (&read_fds);
        FD_SET (server_socket, &read_fds);

        for (int i = 0; i < num_clients; i++) // Thêm các socket client vào tập read_fds
        {
            int cur_socket = clients[i].socket;
            if (cur_socket > 0)
                FD_SET (cur_socket, &read_fds);
            // Cập nhật max_fd nếu socket hiện tại lớn hơn
            max_fd = (cur_socket > max_fd) ? cur_socket : max_fd;
        }

        int activity = select (max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0)
        {
            perror ("Select error");
            continue;
        }

        if (FD_ISSET (server_socket, &read_fds)) // Kiểm tra nếu có kết nối mới đến
        {
            socklen_t address_length = sizeof (client_address);
            int new_socket = accept (server_socket, (struct sockaddr *) &client_address, &address_length); // Chấp nhận kết nối mới

            if (new_socket < 0)
            {
                perror ("Error! Cannot connect new user\n");
                continue;
            }
            // Kiểm tra số lượng client đã đạt giới hạn chưa
            if (num_clients < MAX_CLIENTS)
            {
                send (new_socket, "Welcome", 7, 0); // Gửi thông báo chào mừng
                clients[num_clients].socket = new_socket; // Lưu socket của client mới
                clients[num_clients].name[0] = '\0'; // Đánh dấu là chưa có tên (tên sẽ được nhận sau)

                // Nhận tên người dùng từ client mới kết nối
                int byte_received = recv (new_socket, clients[num_clients].name, BUFFER_SIZE, 0);
                clients[num_clients].name[byte_received] = '\0';

                printf ("%s has joined the chat\n", clients[num_clients].name);

                // Chuẩn bị thông báo client mới đã tham gia
                sprintf (notification, " *) Notification: %s has joined the chat", clients[num_clients].name); 
                // Gửi thông báo tới các client khác (trừ client mới)
                broadcast_message (clients, num_clients, new_socket, notification);

                num_clients++;
            }
            else
            {
                send (new_socket, "Full", 4, 0);
                printf ("Too many clients, rejecting connection.\n"); // Thông báo server đầy
    
                close (new_socket); // Đóng socket của client bị từ chối
            }
        }

        for (int i = 0; i < num_clients; i++) // Kiểm tra các socket client hiện có
        {
            if (FD_ISSET (clients[i].socket, &read_fds)) // Nếu có dữ liệu từ client
            {
                int receive = recv (clients[i].socket, client_message, sizeof (client_message), 0); // Nhận tin nhắn từ client

                if (receive <= 0) // Xử lý trường hợp client ngắt kết nối hoặc lỗi
                {
                    remove_client (clients, i, &num_clients, &read_fds);
                    i--; // Giảm i để kiểm tra lại vị trí hiện tại (vì client cuối cùng đã di chuyển lên)
                }
                else
                {
                    client_message[receive] = '\0';
                    if (strcmp (client_message, "Bye") == 0) // Nếu client gửi "Bye", xử lý rời đi
                    { 
                        remove_client (clients, i, &num_clients, &read_fds);
                        i--;
                    }
                    else
                    {
                        char send_message[BUFFER_SIZE];
                        sprintf (send_message, "%s: %s", clients[i].name, client_message);
                        printf ("%s\n", send_message); // In tin nhắn lên server console
                        
                        broadcast_message (clients, num_clients, clients[i].socket, send_message); // Gửi tin nhắn tới các client khác
                    }
                }
            }
        }
    }

    close (server_socket); // Đóng socket server
    return 0;
}