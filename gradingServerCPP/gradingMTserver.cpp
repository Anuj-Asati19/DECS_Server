#include <iostream>
#include <fstream>
#include <cstring>
#include <thread>
#include <filesystem>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <netinet/in.h>

using namespace std;
namespace fs = std::filesystem;

const int BUFFER_SIZE = 1024;
const int MAX_FILE_SIZE_BYTES = 4;
const int MAX_QUEUE = 50;

const char SUBMISSIONS_DIR[] = "./submissions/";
const char EXECUTABLES_DIR[] = "./executables/";
const char OUTPUTS_DIR[] = "./outputs/";
const char COMPILER_ERROR_DIR[] = "./compiler_error/";
const char RUNTIME_ERROR_DIR[] = "./runtime_error/";
const char EXPECTED_OUTPUT[] = "./expected/output.txt";

const char PASS_MSG[] = "PROGRAM RAN\n";
const char COMPILER_ERROR_MSG[] = "COMPILER ERROR\n";
const char RUNTIME_ERROR_MSG[] = "RUNTIME ERROR\n";
const char OUTPUT_ERROR_MSG[] = "OUTPUT ERROR\n";

mutex thread_mutex;
int total_requests = 0;
int served_requests = 0;

int recv_file(int sockfd, string file_path)
{
    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);
    FILE *file = fopen(file_path.c_str(), "wb");
    if (!file)
    {
        perror("Error opening file");
        return -1;
    }

    char file_size_bytes[MAX_FILE_SIZE_BYTES];
    if (recv(sockfd, file_size_bytes, sizeof(file_size_bytes), 0) == -1)
    {
        perror("Error receiving file size");
        fclose(file);
        return -1;
    }
    int file_size;
    memcpy(&file_size, file_size_bytes, sizeof(file_size_bytes));

    size_t bytes_read = 0;
    while (true)
    {
        size_t bytes_recvd = recv(sockfd, buffer, BUFFER_SIZE, 0);
        bytes_read += bytes_recvd;
        if (bytes_read <= 0)
        {
            perror("Error receiving file data");
            fclose(file);
            return -1;
        }
        fwrite(buffer, 1, bytes_recvd, file);
        bzero(buffer, BUFFER_SIZE);
        if (bytes_read >= file_size)
            break;
    }
    fclose(file);
    return 0;
}

int start_worker(int sockfd)
{


		int worker_id = (int) pthread_self();
		
    string source_file = SUBMISSIONS_DIR + to_string(worker_id) + ".cpp";
    string executable = EXECUTABLES_DIR + to_string(worker_id) + ".o";
    string output_file = OUTPUTS_DIR + to_string(worker_id) + ".txt";
    string compiler_error_file = COMPILER_ERROR_DIR + to_string(worker_id) + ".err";
    string runtime_error_file = RUNTIME_ERROR_DIR + to_string(worker_id) + ".err";

    string compile_command = "g++ " + source_file + " -o " + executable + " > /dev/null 2> " + compiler_error_file;
    string run_command = executable + " > " + output_file + " 2> " + runtime_error_file;

    
    cout << compile_command << endl;;
    cout << run_command << endl;
    

    string result_msg = "";
    string result_details = "";

    if (recv_file(sockfd, source_file) != 0)
    {
        close(sockfd);

        return 0;
    }

    // Some progress response
    int n = send(sockfd, "I got your code file for grading\n", 33, 0);
    
    if (system(compile_command.c_str()) != 0)
    {
        result_msg = COMPILER_ERROR_MSG;
        result_details = compiler_error_file;
    }
    else if (system(run_command.c_str()) != 0)
    {
        result_msg = RUNTIME_ERROR_MSG;
        result_details = runtime_error_file;
    }
    else 
    {
        result_msg = PASS_MSG;
        result_details = output_file;
    }
 

    if (send(sockfd, result_msg.c_str(), strlen(result_msg.c_str()), 0) == -1)
    {
        perror("Error sending result message");
        close(sockfd);
        lock_guard<mutex> lock(thread_mutex);
        served_requests++;
        return 0;
    }

    close(sockfd);
  
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cerr << "Usage: ./server  <port>\n";
        return -1;
    }

    int port = stoi(argv[1]);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("Socket creation failed");
        return 1;
    }
    struct sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    int iSetOption = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&iSetOption, sizeof(iSetOption));
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0)
    {
        perror("Bind failed");
        close(sockfd);
        return -1;
    }
    if (listen(sockfd, MAX_QUEUE) != 0)
    {
        perror("Listen failed");
        close(sockfd);
        return -1;
    }

    cout << "Server listening on port: " << port << "\n";

    try
    {
        if (!fs::exists(SUBMISSIONS_DIR))
            fs::create_directory(SUBMISSIONS_DIR);
        if (!fs::exists(EXECUTABLES_DIR))
            fs::create_directory(EXECUTABLES_DIR);
        if (!fs::exists(OUTPUTS_DIR))
            fs::create_directory(OUTPUTS_DIR);
        if (!fs::exists(COMPILER_ERROR_DIR))
            fs::create_directory(COMPILER_ERROR_DIR);
        if (!fs::exists(RUNTIME_ERROR_DIR))
            fs::create_directories(RUNTIME_ERROR_DIR);
    }
    catch (fs::filesystem_error &e)
    {
        cerr << "Error creating directories: " << e.what() << "\n";
        close(sockfd);
        return -1;
    }

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (true)
    {
        int client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);


        thread worker(start_worker,  client_sockfd);
        worker.detach();
    }

    close(sockfd);
    return 0;
}

