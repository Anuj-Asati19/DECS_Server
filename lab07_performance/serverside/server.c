#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/time.h>

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int compareContent(const char *file_output) {
    const char *desired_output = "1 2 3 4 5 6 7 8 9 10 ";
    return strcmp(file_output, desired_output);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Port number not provided\n");
        exit(1);
    }

    int sockfd, newsockfd, portno, n;
    char buffer[5000];
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("Error opening socket\n");
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));

    portno = atoi(argv[1]);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        error("Binding Failed\n");
    }

    listen(sockfd, 25);
    clilen = sizeof(cli_addr);

    while (1) { // Infinite loop to keep the server running
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

        if (newsockfd < 0) {
            error("Error on accept");
        }

        bzero(buffer, 5000);
        int fp = open("received_code.c", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fp < 0) {
            error("Error opening file");
        }
        n = read(newsockfd, buffer, sizeof(buffer));
        write(fp, buffer, n);
        printf("The file has been received successfully\n");
        close(fp);
        const char *filename = "received_code.c";
        int compile_result = system("gcc received_code.c -o received_code 2> compiler_error.txt");
        if (compile_result != 0) {
            printf("Compilation failed\n\n");
            int filec;
            char buffc[5000];
            bzero(buffc, 5000);
            filec = open("compiler_error.txt", O_RDONLY);
            if (filec < 0) {
                error("Error opening compiler error file");
            }
            while ((n = read(filec, buffc, sizeof(buffc))) > 0) {
                write(newsockfd, buffc, n);
            }
            close(filec);
        } else {
            int second_run = system("./received_code 1> output.txt 2> runtime_error.txt");
            if (second_run != 0) {
                printf("Runtime error\n\n");
                int filer;
                char buffr[5000];
                bzero(buffr, 5000);
                filer = open("runtime_error.txt", O_RDONLY);
                if (filer < 0) {
                    error("Error opening compiler error file");
                }
                while ((n = read(filer, buffr, sizeof(buffr))) > 0) {
                    write(newsockfd, buffr, n);
                }
                close(filer);
            } else {
                system("echo >> output.txt");
                int diffResult = system("diff expected_output.txt output.txt 1> diff_out.txt");
                
                int file;
                file = open("diff_out.txt", O_RDONLY);
                if (file < 0) {
                    error("Error opening output file");
                }
                char file_output[5000];
                size_t bytes_read = read(file, file_output, sizeof(file_output) - 1);
                file_output[bytes_read] = 'F';
                file_output[bytes_read+1] = 'A';
                file_output[bytes_read+2] = 'I';
                file_output[bytes_read+3] = 'L';
                file_output[bytes_read+4] = '\n';
                file_output[bytes_read+5] = '\0';
                
                close(file);
                
                if (bytes_read == 0) {
                    printf("File contents match the desired output.\n\n");
                    write(newsockfd, "PASS\n\n", sizeof("PASS\n\n"));
                } else {
                    printf("File contents do not match the desired output.\n\n");
                    write(newsockfd, file_output, sizeof(file_output));
                }
            }
        }
        close(newsockfd);
    }

    close(sockfd);
    return 0;
}
