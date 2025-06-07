#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 128

int receive_ftp_response (int control_socket, char *response)
{
    int byte_received = recv (control_socket, response, BUFFER_SIZE, 0);
    int response_code;

    response[byte_received] = '\0';
    sscanf (response, "%d", &response_code);
    return response_code;
}

void extract_data_ip_port (char *response, char *ip, int *port)
{
    int h1, h2, h3, h4, p1, p2;

    sscanf (response, 
        "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d).",
        &h1, &h2, &h3, &h4, &p1, &p2);

    sprintf (ip, "%d.%d.%d.%d", h1, h2, h3, h4);
    *port = p1 * 256 + p2;
}

int main (int argc, char *argv[])
{
    int control_socket, data_socket;
    struct sockaddr_in control_address, data_address;
    FILE *file;

    int data_port, file_length;
    char data_ip[INET_ADDRSTRLEN], *file_content;

    char command[BUFFER_SIZE], response[BUFFER_SIZE];
    int response_code;

    printf ("\nConnecting to FTP server ...\n");
    sleep (1);

    control_socket = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

    control_address.sin_port = htons (21);
    control_address.sin_family = AF_INET;
    control_address.sin_addr.s_addr = inet_addr ("127.0.0.1");

    if (connect (control_socket, (struct sockaddr *) &control_address, sizeof (control_address)) < 0)
    {
        printf ("Cannot connect to FTP server");
        return -1;
    }

    receive_ftp_response (control_socket, response);

    printf ("Checking username ...\n");
    sleep (1);

    sprintf (command, "USER %s\n", argv[2]);
    send (control_socket, command, strlen (command), 0);

    receive_ftp_response (control_socket, response);

    printf ("Checking password ...\n");
    sleep (1);

    sprintf (command, "PASS %s\n", argv[3]);
    send (control_socket, command, strlen (command), 0);

    if (receive_ftp_response (control_socket, response) == 530)
    {
        printf ("Username or password is incorrect\n");
        return 0;
    }

    printf ("Entering passive mode ...\n");
    sleep (1);

    send (control_socket, "PASV\r\n", 6, 0);

    if (receive_ftp_response (control_socket, response) != 227)
    {
        perror ("ERROR occurs when entering passive mode");
        return -1;
    }

    extract_data_ip_port (response, data_ip, &data_port);

    printf ("Opening data connection ...\n");
    sleep (1);

    data_socket = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

    data_address.sin_port = htons (data_port);
    data_address.sin_family = AF_INET;
    data_address.sin_addr.s_addr = inet_addr (data_ip);

    if (connect (data_socket, (struct sockaddr *) &data_address, sizeof (data_address)) < 0)
    {
        perror ("ERROR occurs when opening data connection");
        return -1;
    }

    printf ("Downloading files ... \n");

    sprintf (command, "RETR %s\r\n", argv[4]);
    send (control_socket, command, strlen (command), 0);

    if (receive_ftp_response (control_socket, response) != 150)
    {
        printf ("\nThis file does not exist\n");

        close (data_socket);
        close (control_socket);
        return 0;
    }

    file = fopen (argv[4], "w");

    sscanf (response, 
        "150 Opening BINARY mode data connection for %s (%d bytes).",
        command, &file_length);

    file_content = malloc (file_length + 1);
    recv (data_socket, file_content, file_length, 0);

    close (data_socket);

    fwrite (file_content, 1, file_length, file);

    if (receive_ftp_response (control_socket, response) == 226)
        printf ("\nDownload file complete\n");
    else
        perror ("\nERROR occurs while downloading this file");

    free (file_content);
    fclose (file);

    printf ("\n");

    close (control_socket);
    return 0;
}