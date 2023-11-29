#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/time.h>
#include <math.h>
#include <errno.h>
#include <stdbool.h>
#include <bits/stdc++.h>

using namespace std;
#define BUFFER_SIZE 128

int successfulRes = 0;
int numErrors = 0;
double totalTime = 0;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    // Intialize the socket file descriptor, port number and a variable n
    int sockfd, portno, n;

    // Initialize structures for timeval sockaddr_in and hostnet
    struct timeval tv;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    // intialize a character buffer
    char buffer[BUFFER_SIZE];
    if (argc != 5)
    {
        cout << "Usage: ./submit <new|status> <serverIP> <port> <sourceCodeFileTobeGraded|requestID>" << endl;
        exit(1);
    }

    // Store the type of the request in a string
    string request_type = argv[1];
    transform(request_type.begin(), request_type.end(), request_type.begin(), [](unsigned char c)
              { return std::tolower(c); });
    portno = atoi(argv[3]);

    // Store the source File or the request ID in the string
    string s_code_reqID = argv[4];

    // Create a sockfd for the TCP stream
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error("ERROR opening socket");
    }

    // Create setsockopt for the client
    int iSetOption = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&iSetOption, sizeof(iSetOption)) < 0)
    {
        error("Setsockopt");
    }

    // Get the server address
    server = gethostbyname(argv[2]);
    if (server == NULL)
    {
        error("ERROR, no such host");
    }

    // set server address bytes to zero
    bzero((char *)&serv_addr, sizeof(serv_addr));

    // Address Family is IP
    serv_addr.sin_family = AF_INET;

    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

    serv_addr.sin_port = htons(portno);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        error("ERROR Connecting!");
    }

    // set buffer to zero
    memset(buffer, 0, BUFFER_SIZE);

    // Send "new" or "status" to the client
    n = write(sockfd, request_type.c_str(), request_type.length());

    // We will Now do separate work for "new" and "status"

    // If the request type is status
    if (request_type == "status")
    {
        string requestID = s_code_reqID;
        // Write the sockfd to the server
        n = write(sockfd, &requestID, sizeof(requestID));
        if (n < 0){
            error("ERROR writing to socket");
        }

        // Server will return a number
        int returned_num;
        n = read(sockfd, &returned_num, sizeof(returned_num));
        if (returned_num == 0){
            int position_at_q = -1;

            // server will send the positin at the queue
            n = read(sockfd, &position_at_q, sizeof(position_at_q));
            cout << "Your grading request ID " << requestID << "has been accepted. It is currently at position " << position_at_q << "in the queue.";
        }
        else if (returned_num == 1){
            // Request is at the Processing Queue
            cout << "Your grading request ID " << requestID << "has been accepted. It is currently under processing.";   
        }
        else if(returned_num == 2){
            // The processing is Completed
            cout << "Processing is completed!\nHere's Server Response: ";
            bzero(buffer,sizeof(buffer));
            int reab=recv(sockfd,buffer,sizeof(buffer),0);
            write(1,buffer,sizeof(buffer));   
        }
        else{
            // The request ID does not exists
            cout << "Request Not Found!" << endl;
        }
    }
    else if (request_type == "new")
    {
        FILE *files = fopen(s_code_reqID.c_str(), "rb");
        fseek(files, 0, SEEK_END);
        int file_size = ftell(files);
        fclose(files);

        n = write(sockfd, &file_size, sizeof(file_size));
        if (n < 0)
        {
            error("ERROR writing to socket");
        }

        int grading = open(s_code_reqID.c_str(), O_RDONLY);

        memset(buffer, 0, BUFFER_SIZE);
        int readBytes;
        while ((readBytes = read(grading, buffer, sizeof(buffer))) > 0)
        {
            n = write(sockfd, buffer, readBytes);
            if (n < 0)
            {
                error("ERROR writing to socket");
            }
            memset(buffer, 0, BUFFER_SIZE);
        }
        string requestID;
        read(sockfd, &requestID, sizeof(requestID));
        cout << "Your grading request ID" << requestID << "has been accepted and is currently being processed.";
    }
    else
    {
        error("Enter new|status");
    }
    close(sockfd);
    return 0;
}