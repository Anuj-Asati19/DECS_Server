#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <error.h>
#include <stdlib.h>
#include <stdbool.h>

const int BUFFER_SIZE = 1024; 
const int MAX_FILE_SIZE_BYTES = 4;
const int MAX_TRIES = 5;

//Utility Function to send a file of any size to the grading server
int send_file(int sockfd, char* file_path)
//Arguments: socket fd, file name (can include path)
{
    char buffer[BUFFER_SIZE]; //buffer to read  from  file
    bzero(buffer, BUFFER_SIZE); //initialize buffer to all NULLs
    FILE *file = fopen(file_path, "rb"); //open the file for reading, get file descriptor 
    if (!file)
    {
        perror("Error opening file");
        return -1;
    }
		
		//for finding file size in bytes
    fseek(file, 0L, SEEK_END); 
    int file_size = ftell(file);
    printf("File size is: %d\n", file_size);
    
    //Reset file descriptor to beginning of file
    fseek(file, 0L, SEEK_SET);
		
		//buffer to send file size to server
    char file_size_bytes[MAX_FILE_SIZE_BYTES];
    //copy the bytes of the file size integer into the char buffer
    memcpy(file_size_bytes, &file_size, sizeof(file_size));
    
    //send file size to server, return -1 if error
    if (send(sockfd, &file_size_bytes, sizeof(file_size_bytes), 0) == -1)
    {
        perror("Error sending file size");
        fclose(file);
        return -1;
    }

	//now send the source code file 
    while (!feof(file))  //while not reached end of file
    {
    
    		//read buffer from file
        size_t bytes_read = fread(buffer, 1, BUFFER_SIZE -1, file);
        
     		//send to server
        if (send(sockfd, buffer, bytes_read+1, 0) == -1)
        {
            perror("Error sending file data");
            fclose(file);
            return -1;
        }
        
        //clean out buffer before reading into it again
        bzero(buffer, BUFFER_SIZE);
    }
    //close file
    fclose(file);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        perror("Usage: ./submit  <serverIP> <port>  <sourceCodeFileTobeGraded>\n");
        return -1;
    }

    char server_ip[40], ip_port[40], file_path[256];
    int server_port;

//get the arguments into the corresponding variables    
    strcpy(server_ip, argv[1]);
    server_port = atoi(argv[2]);

    strcpy (file_path,argv[3]);
    
//create the socket file descriptor
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("Socket creation failed");
        return -1;
    }

// setup the server side variables
    struct sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &serv_addr.sin_addr.s_addr);

    int tries = 0;
    while (true)
    {
    	  //connect to the server using the socket fd created earlier
        if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == 0)
            break;
        sleep(1);
        tries += 1;
        if (tries == MAX_TRIES)
        {
            printf ("Server not responding\n");
            return -1;
        }
    }
    
    //send the file by calling the send file utility function

    if (send_file(sockfd, file_path) != 0)
    {
        printf ("Error sending source file\n");
        close(sockfd);
        return -1;
    };

 		printf ("Code sent for grading, waiting for response\n");
    size_t bytes_read;
    //buffer for reading server response
    char buffer[BUFFER_SIZE];
    memset(buffer,0,BUFFER_SIZE);
    while (true)
    {
    
    	//read server response
        bytes_read = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if (bytes_read <= 0)
            break;
        printf("Server Response: ");
        printf("%s\n", buffer);
        memset(buffer,0,BUFFER_SIZE);
    }

//close socket file descriptor
    close(sockfd);

    return 0;
}
