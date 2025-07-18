#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <ctype.h>
#include <unistd.h> 
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

char *capitalize (char *str)
{
    int n = strlen (str);
    char *result = (char *) calloc (n, sizeof (char));

    for (int i = 0; i < n; ++i)
        result[i] = toupper (str[i]);

    return result;
}

int main ()
{
    int server_fd, new_socket; 
    struct sockaddr_in address; 
    int addrlen = sizeof (address); 
    char buffer[BUFFER_SIZE] = {0};

    // Creating socket file descriptor
    if ((server_fd = socket (AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror ("Socket failed");
        return -1;
    }

    // Bind to address and port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons (PORT);

    if (bind (server_fd, (struct sockaddr *) &address, sizeof (address)) < 0)
    {
        perror ("Bind failed");
        return -1;
    }
    
    // Start listening for connections 
    if (listen (server_fd, 3) < 0)
    {
        perror ("Listen failed");
        return -1;
    }

    printf ("Server listening on port %d ... \n", PORT);

    // Accept a connection from the client
    if ((new_socket = accept (server_fd, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0)
    {
        perror ("Accept falled");
        return -1;
    }

    // Read string by the client
    read (new_socket, buffer, BUFFER_SIZE);
    printf ("Message read from client: %s", buffer);

    // Send new string to the client
    char *new_buffer = capitalize (buffer);
    send (new_socket, new_buffer, strlen (new_buffer), 0); 
    printf ("Message sent to client: %s", new_buffer);

    // Close the socket 
    close (new_socket); 
    close (server_fd);
    
    return 0;
}