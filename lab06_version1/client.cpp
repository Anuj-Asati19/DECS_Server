#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <File> <serverIP> <port>" << endl;
        return 1;
    }

    char* sourceCodeFile = argv[1];
    char* serverIp = argv[2];
    int port = atoi(argv[3]);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        return 1;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, serverIp, &serv_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error connecting to server");
        return 1;
    }

    ifstream sourceFile(sourceCodeFile);
    if (!sourceFile.is_open()) {
        perror("Error opening source code file");
        close(sockfd);
        return 1;
    }

    string sourceCode;
    string line;
    while (getline(sourceFile, line)) {
        sourceCode += line + '\n';
    }
    sourceFile.close();

    ssize_t bytesSent = write(sockfd, sourceCode.c_str(), sourceCode.size());
    if (bytesSent != static_cast<ssize_t>(sourceCode.size())) {
        perror("Error sending source code");
        close(sockfd);
        return 1;
    }

    char responseBuffer[1024];
    memset(responseBuffer, 0, sizeof(responseBuffer));
    while (true) {
        ssize_t bytesRead = read(sockfd, responseBuffer, sizeof(responseBuffer) - 1);

        if (bytesRead == 0) {
            // The server has closed the connection (end of response)
            break;
        } else if (bytesRead < 0) {
            // Handle read error
            perror("Error receiving response from server");
            close(sockfd);
            return 1;
        }
    }


    cout << "Server Response: " << responseBuffer << endl;

    close(sockfd);
    return 0;
}
