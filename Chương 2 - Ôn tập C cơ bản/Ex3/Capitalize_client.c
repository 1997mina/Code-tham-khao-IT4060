#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h> 
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main ()
{
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char str[BUFFER_SIZE];

    // Creating socket
    if ((sock = socket (AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        printf ("Socket creation error\n"); 
        return -1;
    }
    
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons (PORT);

    // Convert IP addresses from text to binary
    if (inet_pton (AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        printf ("Invalid address or address not supported\n"); 
        return -1;
    }
    
    // Connect to the server
    if (connect (sock, (struct sockaddr *)&serv_addr, sizeof (serv_addr)) < 0)
    {
        printf ("Connection Failed\n");
        return -1;
    }

    printf ("Enter a string: "); 
    fgets (str, BUFFER_SIZE, stdin);

    // Send a message to the server
    send (sock, str, strlen (str), 0);
    printf ("Message sent to server: %s", str);

    // Read server's response
    read (sock, buffer, BUFFER_SIZE);
    printf ("Message read from server: %s", buffer);

    // Close the socket
    close (sock);
    
    return 0;
}