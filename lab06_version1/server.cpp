#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sstream>
#include <cstdlib>
#include <ctime>

using namespace std;

// Custom function to compare two strings and check if they are equal
bool areStringsEqual(const string& str1, const string& str2) {
    return str1 == str2;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <port>" << endl;
        return 1;
    }

    int port = atoi(argv[1]);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        return 1;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error binding socket");
        return 1;
    }

    listen(sockfd, 1);
    cout << "Server is listening on port " << port << endl;

    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    int newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);

    if (newsockfd < 0) {
        perror("Error accepting connection");
        return 1;
    }

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    int bytesRead = recv(newsockfd, buffer, 5000, 0);

    if (bytesRead <= 0) {
        perror("Error receiving data from client");
        close(newsockfd);
        return 1;
    }

    string sourceCode(buffer);
    cout << "Received source code from client:\n" << sourceCode << endl;

    // Create a temporary file to hold the source code
    int sourcefd = open("temp.cpp", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    if (sourcefd < 0) {
        perror("Error creating temporary file");
        const char* errorMsg = "COMPILER ERROR\nError creating temporary file";
        send(newsockfd, errorMsg, strlen(errorMsg), 0);
        close(newsockfd);
        return 1;
    }

    // Write the source code to the temporary file
    int bytesWritten = write(sourcefd, sourceCode.c_str(), sourceCode.size());
    close(sourcefd);

    if (bytesWritten != static_cast<int>(sourceCode.size())) {
        perror("Error writing source code to temporary file");
        const char* errorMsg = "COMPILER ERROR\nError writing source code to temporary file";
        send(newsockfd, errorMsg, strlen(errorMsg), 0);
        close(newsockfd);
        return 1;
    }

    // Compile the source code
    int compileStatus = system("g++ -o temp_exe temp.cpp 2> compile_error.txt");

    if (compileStatus != 0) {
        perror("Compilation error");
        ifstream compileErrorFile("compile_error.txt");
        string compileError((istreambuf_iterator<char>(compileErrorFile)), istreambuf_iterator<char>());
        compileErrorFile.close();

        const char* errorMsg = ("COMPILER ERROR\n" + compileError).c_str();

        send(newsockfd, errorMsg, strlen(errorMsg), 0);
        close(newsockfd);
        return 1;
    }

    // Run the compiled program and capture its output
    string command = "./temp_exe > program_output.txt 2> runtime_error.txt &";
    int status = system(command.c_str());

    // Set a timeout of 5 seconds for program execution
    time_t startTime = time(nullptr);
    const int timeoutSeconds = 5;
    bool timeout = false;

    while (true) {
        int childStatus;
        pid_t result = waitpid(-1, &childStatus, WNOHANG);

        if (result > 0) {
            // Child process has terminated
            if (WIFEXITED(childStatus)) {
                break; // Program has terminated
            } else {
                // Handle other termination cases here if needed
                perror("Child process terminated abnormally");
                break;
            }
        }

        // Check for timeout
        time_t currentTime = time(nullptr);
        if (difftime(currentTime, startTime) >= timeoutSeconds) {
            timeout = true;
            break;
        }
    }

    if (timeout) {
        // Handle the case where the program execution times out
        const char* errorMsg = "RUNTIME ERROR\nProgram execution timed out";
        send(newsockfd, errorMsg, strlen(errorMsg), 0);
    } else {
        // Compare the output with the expected output using the custom function
        ifstream programOutputFile("program_output.txt");
        string programOutput((istreambuf_iterator<char>(programOutputFile)), istreambuf_iterator<char>());
        programOutputFile.close();

        string expectedOutput = "1 2 3 4 5 6 7 8 9 10\n";

        if (areStringsEqual(programOutput, expectedOutput)) {
            const char* successMsg = "PASS";
            send(newsockfd, successMsg, strlen(successMsg), 0);
        } else {
            const char* errorMsg = "OUTPUT ERROR\nOutput does not match expected output";
            send(newsockfd, errorMsg, strlen(errorMsg), 0);
        }
    }

    close(newsockfd);
    close(sockfd);
    return 0;
}
