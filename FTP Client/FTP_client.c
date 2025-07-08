#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 256

int receive_ftp_response (int control_socket, char *response);
bool ftp_list (int control_socket);
void ftp_retr (int control_socket, const char *file_name);
void ftp_stor (int control_socket, const char *file_name);
void ftp_delete (int control_socket, const char *file_name);
void ftp_rename (int control_socket, const char *old_file_name, const char *new_file_name);
void ftp_cwd (int control_socket, const char *directory);
void login (int control_socket);
void draw_line ();

int main ()
{
    int control_socket;
    struct sockaddr_in control_address;
    char response[BUFFER_SIZE];

    control_socket = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

    control_address.sin_port = htons (21);
    control_address.sin_family = AF_INET;
    inet_pton (AF_INET, "127.0.0.1", &control_address.sin_addr);

    if (connect (control_socket, (struct sockaddr *) &control_address, sizeof (control_address)) < 0)
    {
        perror ("ERROR occurs when connecting to FTP server");
        return -1;
    }

    if (receive_ftp_response (control_socket, response) != 220)
    {
        perror ("ERROR occurs when connecting to FTP server");
        return -1;
    }

    printf ("\r\nWelcome to FTP service\r\n");

    draw_line ();

    printf ("\r\nPlease login to continue\r\n");
    login (control_socket);
    sleep (1);

    int choice;

    while (1)
    {
        draw_line ();
        
        printf ("\r\nOptions:\r\n");
        printf (" 1. Show list of files\r\n");
        printf (" 2. Download a file from server\r\n");
        printf (" 3. Upload a file to server\r\n");
        printf (" 4. Delete a file on server\r\n");
        printf (" 5. Rename a file on server\r\n");
        printf (" 6. Change working directory on the server\r\n");
        printf (" 7. Exit\r\n");

        printf ("\r\nYour choice: ");
        scanf ("%d", &choice);

        draw_line ();
        getchar ();

        char input1[BUFFER_SIZE], input2[BUFFER_SIZE];

        switch (choice)
        {
            case 1:
                if (!ftp_list (control_socket))
                    printf ("\r\nThere is no file found on working directory\r\n");

                break;

            case 2:
                if (!ftp_list (control_socket))
                {
                    printf ("\r\nThere is no file found to download\r\n");
                    break;
                }

                printf ("\r\nWhich file do you want to download?: ");
                fgets (input1, BUFFER_SIZE, stdin);
                input1[strcspn (input1, "\r\n")] = '\0';

                ftp_retr (control_socket, input1);
                // Gửi lệnh RETR để tải xuống tệp
                break;
            // Xử lý mã phản hồi 150 (File status okay; about to open data connection)
            case 3:
                printf ("\r\nWhich file do you want to upload?: ");
                fgets (input1, BUFFER_SIZE, stdin);
                input1[strcspn (input1, "\r\n")] = '\0';

                ftp_stor (control_socket, input1);
                // Gửi lệnh STOR để tải lên tệp
                break;
            // Xử lý mã phản hồi 150 (File status okay; about to open data connection)
            case 4:
                if (!ftp_list (control_socket))
                {
                    printf ("\r\nThere is no file found to delete\r\n");
                    break;
                }

                printf ("\r\nWhich file do you want to delete?: ");
                fgets (input1, BUFFER_SIZE, stdin);
                input1[strcspn (input1, "\r\n")] = '\0';

                ftp_delete (control_socket, input1);
                // Gửi lệnh DELE để xóa tệp
                break; // Xử lý mã phản hồi 250 (Requested file action okay, completed)

            case 5:
                if (!ftp_list (control_socket))
                {
                    printf ("\r\nThere is no file found to rename\r\n");
                    break;
                }

                printf ("\r\nWhich file do you want to rename?: ");
                fgets (input1, BUFFER_SIZE, stdin);
                input1[strcspn (input1, "\r\n")] = '\0';

                printf ("\r\nEnter the new name for this file: ");
                fgets (input2, BUFFER_SIZE, stdin);
                input2[strcspn (input2, "\r\n")] = '\0';

                ftp_rename (control_socket, input1, input2);
                // Gửi lệnh RNFR và RNTO để đổi tên tệp
                break; // Xử lý mã phản hồi 350 (Requested file action pending further information) và 250 (Requested file action okay, completed)

            case 6:
                send (control_socket, "PWD\r\n", 5, 0);
                receive_ftp_response (control_socket, response);
                // Gửi lệnh PWD để lấy thư mục làm việc hiện tại

                sscanf (response, "257 \"/%s", input1);
                input1[strcspn (input1, "\"")] = '\0';

                printf ("\r\nCurrent working directory: /ftpdata/hailong/%s\r\n", input1);
                
                printf ("\r\nEnter the new directory: ");
                fgets (input2, BUFFER_SIZE, stdin);
                input2[strcspn (input2, "\r\n")] = '\0';

                ftp_cwd (control_socket, input2);
                // Gửi lệnh CWD để thay đổi thư mục làm việc
                break; // Xử lý mã phản hồi 250 (Requested file action okay, completed)
        
            case 7:
                printf ("\r\nDisconnecting from FTP server ...\r\n\r\n");
                
                send (control_socket, "QUIT\r\n", 6, 0);
                // Gửi lệnh QUIT để thoát khỏi phiên FTP
                receive_ftp_response (control_socket, response);
                // Xử lý mã phản hồi 221 (Service closing control connection)
                sleep (1);
                
                close (control_socket);
                return 0;

            default:
                sleep (1);
                printf ("\r\nInvalid input. Please type again\r\n");
                break;
        }

        if (choice < 1 || choice > 6)
            continue;

        sleep (1);
        draw_line ();

        printf ("\r\nDo you want to continue? (Y: yes, N: no): ");
        while (1)
        {
            char c;
            scanf ("%c", &c);
            getchar ();

            if (c == 'y' || c == 'Y')
            {
                sleep (1);
                break;
            }

            else if (c == 'n' || c == 'N')
            {
                printf ("\r\nDisconnecting from FTP server ...\r\n\r\n");
                sleep (1);
                
                close (control_socket);
                return 0;
            }

            else
            {
                sleep (1);
                printf ("\r\nInvalid input. Please type again (Y: yes, N: no): ");
            }
        }
    }
}

int receive_ftp_response (int control_socket, char *response)
{
    int byte_received = recv (control_socket, response, BUFFER_SIZE, 0);
    int response_code;

    response[byte_received] = '\0';
    sscanf (response, "%d", &response_code);
    return response_code;
}

int open_data_connection (int control_socket)
{
    int data_socket;
    int h1, h2, h3, h4, p1, p2;
    struct sockaddr_in data_address;
    char data_ip[INET_ADDRSTRLEN], response[BUFFER_SIZE];
    
    send (control_socket, "PASV\r\n", 6, 0);
    // Gửi lệnh PASV để yêu cầu chế độ thụ động

    if (receive_ftp_response (control_socket, response) != 227)
    {
        perror ("ERROR occurs when opening data connection"); // Xử lý mã phản hồi 227 (Entering Passive Mode)
        exit (EXIT_FAILURE);
    }

    sscanf (response, 
        "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d).",
        &h1, &h2, &h3, &h4, &p1, &p2);

    sprintf (data_ip, "%d.%d.%d.%d", h1, h2, h3, h4);
    // Phân tích địa chỉ IP cho kết nối dữ liệu

    data_socket = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

    data_address.sin_port = htons (p1 * 256 + p2);
    data_address.sin_family = AF_INET;
    inet_aton (data_ip, &data_address.sin_addr);

    if (connect (data_socket, (struct sockaddr *) &data_address, sizeof (data_address)) < 0)
    {
        perror ("ERROR occurs when opening data connection");
        exit (EXIT_FAILURE);
    }

    return data_socket;
}

bool ftp_list (int control_socket)
{
    char list_data[BUFFER_SIZE], response[BUFFER_SIZE];
    bool found = false;
    int data_socket = open_data_connection (control_socket);

    send (control_socket, "LIST\r\n", 6, 0);
    // Gửi lệnh LIST để yêu cầu danh sách tệp

    if (receive_ftp_response (control_socket, response) != 150)
    {
        perror ("ERROR occurs when receiving files"); // Xử lý mã phản hồi 150 (File status okay; about to open data connection)
        exit (EXIT_FAILURE);
    }

    printf ("\r\nLoading files and subfolders...\r\n\r\n");
    sleep (1);

    while (1)
    {
        int byte_received = recv (data_socket, list_data, sizeof (list_data), 0);

        if (byte_received <= 0)
            break;

        found = true;
        list_data[byte_received] = '\0';
        printf ("%s", list_data);
    }

    close (data_socket);
    receive_ftp_response (control_socket, response);
    // Xử lý mã phản hồi 226 (Closing data connection. Requested file action successful)
    return found;
}

void ftp_retr (int control_socket, const char *file_name)
{
    int content_length, data_socket = open_data_connection (control_socket);
    char response[BUFFER_SIZE], command[BUFFER_SIZE], *file_content;
    FILE *download_file = fopen (file_name, "w");
    
    sprintf (command, "RETR %s\r\n", file_name);

    send (control_socket, command, strlen (command), 0);
    // Gửi lệnh RETR để tải xuống tệp

    if (receive_ftp_response (control_socket, response) != 150)
    {
        printf ("\r\nThis file does not exist\r\n"); // Xử lý mã phản hồi 150 (File status okay; about to open data connection)
        return;
    }

    sscanf (response, 
        "150 Opening BINARY mode data connection for %s (%d bytes).",
        command, &content_length);
        
    file_content = malloc (content_length + 1);
    recv (data_socket, file_content, content_length, 0);

    close (data_socket);

    fwrite (file_content, 1, content_length, download_file);

    if (receive_ftp_response (control_socket, response) == 226)
        printf ("\r\nDownload file complete\r\n"); // Xử lý mã phản hồi 226 (Closing data connection. Requested file action successful)
    else
        perror ("\r\nERROR occurs while downloading this file");

    fclose (download_file);
    free (file_content);
}

void ftp_stor (int control_socket, const char *file_name)
{
    int data_socket = open_data_connection (control_socket);
    char response[BUFFER_SIZE], command[BUFFER_SIZE], *file_content;
    FILE *upload_file = fopen (file_name, "r");

    if (upload_file == NULL)
    {
        printf ("\r\nThis file does not exist in your local directory\r\n");
        return;
    }
    
    sprintf (command, "STOR %s\r\n", file_name);

    send (control_socket, command, strlen (command), 0);
    // Gửi lệnh STOR để tải lên tệp

    if (receive_ftp_response (control_socket, response) != 150)
    {
        perror ("ERROR occurs when uploading a file"); // Xử lý mã phản hồi 150 (File status okay; about to open data connection)
        return;
    }

    fseek (upload_file, 0, SEEK_END);
    long content_length = ftell (upload_file);
    fseek (upload_file, 0, SEEK_SET);
        
    file_content = malloc (content_length + 1);

    fread (file_content, 1, content_length, upload_file);
    send (data_socket, file_content, content_length, 0);

    close (data_socket);

    if (receive_ftp_response (control_socket, response) == 226)
        printf ("\r\nUpload file complete\r\n"); // Xử lý mã phản hồi 226 (Closing data connection. Requested file action successful)
    else
        perror ("\r\nERROR occurs while uploading this file");

    fclose (upload_file);
    free (file_content);
}

void ftp_rename (int control_socket, const char *old_file_name, const char *new_file_name)
{
    char command[BUFFER_SIZE], response[BUFFER_SIZE];

    sprintf (command, "RNFR %s\r\n", old_file_name);
    send (control_socket, command, strlen (command), 0); // Gửi lệnh RNFR (Rename From)

    if (receive_ftp_response (control_socket, response) != 350)
    {
        printf ("\r\nThis file does not exist\r\n"); // Xử lý mã phản hồi 350 (Requested file action pending further information)
        return;
    }

    // Gửi lệnh RNTO (Rename To)

    sprintf (command, "RNTO %s\r\n", new_file_name);
    send (control_socket, command, strlen (command), 0); 

    if (receive_ftp_response (control_socket, response) == 250)
        printf ("\r\nRename file complete\r\n");
    else
        printf ("\r\nThis file is already exist\r\n");
}

void ftp_delete (int control_socket, const char *file_name)
{
    char command[BUFFER_SIZE], response[BUFFER_SIZE];

    sprintf (command, "DELE %s\r\n", file_name);
    // Gửi lệnh DELE để xóa tệp
    send (control_socket, command, strlen (command), 0);

    if (receive_ftp_response (control_socket, response) == 250)
        printf ("\r\n%s is already deleted\r\n", file_name); // Xử lý mã phản hồi 250 (Requested file action okay, completed)
    else
        printf ("\r\nThis file is not exist on server\r\n");
}

void ftp_cwd (int control_socket, const char *directory)
{
    char command[BUFFER_SIZE], response[BUFFER_SIZE];

    sprintf (command, "CWD %s\r\n", directory);
    // Gửi lệnh CWD để thay đổi thư mục làm việc
    send (control_socket, command, strlen (command), 0);

    if (receive_ftp_response (control_socket, response) == 250)
        printf ("\r\nChanging directory complete\r\n"); // Xử lý mã phản hồi 250 (Requested file action okay, completed)
    else
        printf ("\r\nThis directory does not exist\r\n");
}

void type_password (char *password, size_t size) 
{
    struct termios old_termio, new_termio;
    int ch, index = 0;

    tcgetattr (STDIN_FILENO, &old_termio);
    new_termio = old_termio;
    new_termio.c_lflag &= ~(ECHO | ICANON);
    tcsetattr (STDIN_FILENO, TCSANOW, &new_termio);

    fflush (stdout);

    while ((ch = getchar ()) != '\n' && ch != EOF && index < size - 1) 
    {
        if (ch == 127 || ch == 8) 
        {
            if (index > 0) 
            {
                index--;
                printf ("\b \b");
                fflush (stdout);
            }
        } 
        else 
        {
            password[index++] = ch;
            printf ("*");
            fflush (stdout);
        }
    }
    password[index] = '\0';

    tcsetattr (STDIN_FILENO, TCSANOW, &old_termio);
    printf ("\n");
}

void login (int control_socket)
{
    char username[BUFFER_SIZE], password[BUFFER_SIZE];
    char command[BUFFER_SIZE], response[BUFFER_SIZE];
    int incorrect_login_count = 0;
    int response_code;

    while (1)
    {
        printf ("Username: ");
        fgets (username, BUFFER_SIZE, stdin);

        sprintf (command, "USER %s", username);
        // Gửi lệnh USER để gửi tên người dùng
        send (control_socket, command, strlen (command), 0);

        response_code = receive_ftp_response (control_socket, response);

        if (response_code != 331)
        {
            perror ("ERROR occurs after entering username"); // Xử lý mã phản hồi 331 (User name okay, need password)
            exit (EXIT_FAILURE);
        }

        printf ("Password: ");
        type_password (password, sizeof (password));

        sprintf (command, "PASS %s\r\n", password);
        // Gửi lệnh PASS để gửi mật khẩu
        send (control_socket, command, strlen (command), 0);

        response_code = receive_ftp_response (control_socket, response);

        if (response_code == 230) // Xử lý mã phản hồi 230 (User logged in, proceed)
            break;
        else if (response_code == 530) // Xử lý mã phản hồi 530 (Not logged in)
        {
            if (++incorrect_login_count == 3)
            {
                printf ("\r\nSigned out because you have logged in wrong too much\r\n\r\n");
                exit (EXIT_SUCCESS);
            }

            else
                printf ("\r\nLogin incorrect. Please type again\r\n");
        }
        else
        {
            perror ("ERROR occurs after entering password");
            exit (EXIT_FAILURE);
        }
    }
}

void draw_line ()
{
    printf ("\r\n");
    for (int i = 0; i < 75; ++i)
        printf ("=");
    printf ("\r\n");
}