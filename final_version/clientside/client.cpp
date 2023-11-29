#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <fstream>
#include <sys/time.h>

using namespace std;

int sendDataToSocket(int sockfd, const string& data) {
    int bytesSent = write(sockfd, data.c_str(), data.size());
    if (bytesSent < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            perror("Timeout occurred at writing.");
        } else {
            perror("Error in writing");
        }
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    return bytesSent;
}

int main(int argc, char *argv[]) {
    string fname;
    int sockfd = 0;

    if (argc != 5) {
        cout << "Usage: ./submit <new|status> <serverIP> <port> <sourceCodeFileTobeGraded|requestID>" << endl;
        exit(1);
    }

    struct sockaddr_in serv_addr;
    struct hostent *server;

    string decision = argv[1];
    server = gethostbyname(argv[2]);
    int portno = stoi(argv[3]);

    if (server == nullptr) {
        cout << "No such host available" << endl;
        exit(0);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));

    serv_addr.sin_port = htons(portno);
    serv_addr.sin_family = AF_INET;

    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

    // create a buffer to store the name of the file to be graded
    string fbuff;
    fname = argv[4];
    int fd = open(fname.c_str(), O_RDONLY);
    char buffer[10000];
    int bytesRead;
    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0) {
        fbuff.append(buffer, bytesRead);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        cout << "Error in creating a socket" << endl;
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            perror("Timeout occurred at connection.");
        } else {
            perror("Connection error in client");
        }
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (decision == "new" || decision == "status") {
        int fbw1 = sendDataToSocket(sockfd, decision);
        int fbw2 = sendDataToSocket(sockfd, fbuff);

        char res[10000];
        int resbytes = read(sockfd, &res, 10000);
        if (resbytes < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                perror("Timeout occurred at reading.");
            } else {
                perror("Error in reading");
            }
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        if (decision == "new") {
            cout << "Your grading request ID " << res << " has been accepted and is currently being processed." << endl;
        } else if (decision == "status") {
            cout << "Status gotten" << endl;
            const char* filePath = "tokenContainer.txt";
            ofstream fileStream(filePath, ios::app);
            if (!fileStream.is_open()) {
                perror("Error opening file: ");
                close(sockfd);
                exit(EXIT_FAILURE);
            }

            string dataToAppend = res;
            fileStream << dataToAppend << endl;
            fileStream.close();
        }

        close(sockfd);
    } else {
        cout << "Request can be either 'new' or 'status'. No other type is accepted." << endl;
        exit(1);
    }

    return 0;
}