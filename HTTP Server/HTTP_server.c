#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>

#define PORT 2004
#define BUFFER_SIZE 1024

void signal_handler (int i);
void send_response (int client_socket, char *status, char *content_type, char *body, long body_length);
void handle_request (int client_socket);

int main ()
{
    int main_socket = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in server_address, client_address;

    if (main_socket < 1)
    {
        printf ("Error! Cannot create Socket\n");
        return -1;
    }
    
    server_address.sin_port = htons (PORT);
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;

    if (bind (main_socket, (struct sockaddr*) &server_address, sizeof (server_address)) < 0)
    {
        printf ("Error! Cannot bind server address\n");
        return -1;
    }

    if (listen (main_socket, 5) < 0)
    {
        printf ("Error! Cannot listen to clients\n");
        return -1;
    }

    printf ("\nHTTP server is running at http://localhost:%d/\n", PORT);

    char request[BUFFER_SIZE];
    while (1)
    {
        socklen_t client_socket_size = sizeof (client_address);
        int client_socket = accept (main_socket, (struct sockaddr *) &client_address, &client_socket_size);

        if (client_socket < 1)
        {
            printf ("Error! Cannot connect to clients\n");
            break;
        }

        pid_t process = fork ();

        if (process == 0)
        {
            handle_request (client_socket);
            close (client_socket);

            break;
        }

        else if (process > 0)
        {
            signal (SIGCHLD, signal_handler);
            close (client_socket);
        }

        else
            printf ("Error! Cannot create child process\n");
    }

    close (main_socket);
    return 0;
}

void signal_handler (int i)
{
    wait (NULL);
}

void send_response (int client_socket, char *status, char *content_type, char *body, long body_length) 
{
    char header[BUFFER_SIZE], date[50];

    time_t now = time (NULL);
    struct tm *t = gmtime (&now);
    
    char wday[][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    char month[][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    
    sprintf (date, 
        "%s, %d %s %d %02d:%02d:%02d GMT", 
        wday[t -> tm_wday], 
        t -> tm_mday, month[t -> tm_mon], 1900 + t -> tm_year,
        t -> tm_hour, t -> tm_min, t -> tm_sec);

    if (!strcmp (status, "204 No Content"))
    {
        sprintf (header,
            "HTTP/1.1 %s\r\n"
            "Date: %s\r\n"
            "Allow: GET, POST, PUT, DELETE, OPTIONS\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n"
            "\r\n",
            status, date, body_length);
    }
    else
    {
        sprintf (header,
            "HTTP/1.1 %s\r\n"
            "Date: %s\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n"
            "\r\n",
            status, date, content_type, body_length); 
    }
       

    char *response = malloc (strlen (header) + body_length + 1); 
    sprintf (response, "%s%s", header, body);

    send (client_socket, response, strlen (response), 0);

    free (response);
}

void handle_request (int client_socket)
{
    char request[BUFFER_SIZE];
    memset (request, 0, BUFFER_SIZE);

    recv (client_socket, request, sizeof (request), 0);
    printf ("\nReceive request:\n%s\n", request);

    char method[8], file_path[BUFFER_SIZE];
    sscanf (request, "%s %s", method, file_path);

    if (!strcmp (file_path, "/"))
        strcat (file_path, "index.html");

    char *file_name = strrchr (file_path, '/') + 1;
    
    if (!strcmp (method, "GET") || !strcmp (method, "HEAD"))
    {
        char content_type[20];

        FILE *file = fopen (file_path + 1, "r");
        if (file == NULL)
        {
            char message[BUFFER_SIZE];
            sprintf (message, "<html>\n<body>\n\t<h1>File named %s does not exist</h1>\n</body>\n</html>", file_name);
            
            send_response (client_socket, "404 Not Found", "text/html", message, strlen (message));
            return;
        }

        if (!strcmp (strrchr (file_name, '.') + 1, "js"))
            strcpy (content_type, "text/javascript");
        else
            sprintf (content_type, "text/%s", strrchr (file_name, '.') + 1);

        fseek (file, 0, SEEK_END);
        long file_size = ftell (file);
        fseek (file, 0, SEEK_SET);

        char *body = malloc (file_size + 1);
        fread (body, 1, file_size, file);
        body[file_size] = '\0';

        fclose (file);

        if (!strcmp (method, "GET"))
            send_response (client_socket, "200 OK", content_type, body, file_size);
        else if (!strcmp (method, "HEAD"))
            send_response (client_socket, "200 OK", content_type, "", file_size);

        free (body);
    }

    else if (!strcmp (method, "POST") || !strcmp (method, "PUT"))
    {
        char *body = strstr (request, "\r\n\r\n") + 4;
        char message[BUFFER_SIZE];

        FILE *file;

        if (!strcmp (method, "POST"))
        {
            strcat (file_path, "/post_data.txt");
            file = fopen (file_path + 1, "a");
        }
        else if (!strcmp (method, "PUT"))
            file = fopen (file_path + 1, "w");


        if (file == NULL)
        {
            sprintf (message, "<html>\n<body>\n\t<h1>Cannot save content to file %s</h1>\n</body>\n</html>", file_name);
            send_response (client_socket, "500 Internal Server Error", "text/html", message, strlen (message));
        }
        else
        {
            fprintf (file, "%s", body);

            if (!strcmp (method, "POST"))
            {
                sprintf (message, 
                    "<html>\r\n<body>\r\n"
                    "\t<h1>Successfully received POST data</h1>\r\n"
                    "\t<h2>Content: %s</h2>\r\n"
                    "</body>\n</html>", body);
            }
            else if (!strcmp (method, "PUT"))
            {
                sprintf (message, 
                    "<html>\r\n<body>\r\n"
                    "\t<h1>File %s saved successfully</h1>\r\n"
                    "\t<h2>Content: %s</h2>\r\n"
                    "</body>\n</html>", 
                    file_name, body);
            }

            send_response (client_socket, "200 OK", "text/html", message, strlen (message));
            fclose (file);
        }
    }

    else if (!strcmp (method, "DELETE"))
    {
        char message[BUFFER_SIZE], status[20];

        if (!remove (file_path + 1))
        {
            sprintf (message, "<html>\n<body>\n\t<h1>File %s deleted successfully</h1>\n</body>\n</html>", file_name);
            strcpy (status, "200 OK");
        }
        else
        {
            sprintf (message, "<html>\n<body>\n\t<h1>File named %s does not exist</h1>\n</body>\n</html>", file_name);
            strcpy (status, "404 Not Found");
        }

        send_response (client_socket, status, "text/html", message, strlen (message));
    }

    else if (!strcmp (method, "OPTIONS"))
        send_response (client_socket, "204 No Content", "", "", 0);

    else
    {
        char message[] = "<html>\r\n<body>\r\n"
            "\t<h1>This method is not allowed</h1>\r\n"
            "\t<h2>Only GET, POST, PUT, DELETE, HEAD, OPTIONS supported</h2>\r\n"
            "</body>\r\n</html>";
        
        send_response
        (
            client_socket, 
            "405 Method Not Allowed", 
            "text/html", 
            message, strlen (message)
        );
    }
}