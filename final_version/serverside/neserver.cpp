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
const int MAX_FILE_STAT_BYTES = 10;
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

// Initialize Conditional Variables
mutex queue_mutex,thread_mutex;
condition_variable queue_condition;

// Initializing queue
queue<int> client_queue;

// Implementing the queue
void threadTask(int sockfd)
{
    start_worker(sockfd);
    {
        lock_guard<mutex> lock(queue_mutex);
        served_requests++;
    }
    queue_condition.notify_one();
}

void enqueueClient(int client_sockfd)
{
    unique_lock<mutex> lock(queue_mutex);
    if (client_queue.size() >= MAX_QUEUE)
    {
        close(client_sockfd);
        return;
    }
    client_queue.push(client_sockfd);
    queue_condition.notify_one();
}

int dequeueClient()
{
    unique_lock<mutex> lock(queue_mutex);
    while (client_queue.empty())
    {
        queue_condition.wait(lock);
    }
    int client_sockfd = client_queue.front();
    client_queue.pop();
    return client_sockfd;
}

//At first we will receive either the client sent "new" request or "status" request
int rec_stat(int sockfd, string file_path){
    char file_stats[MAX_FILE_STAT_BYTES];
    if (recv(sockfd, file_stats, sizeof(file_stats), 0) == -1)
    {
        perror("Error receiving file size");
        return -1;
    }
    if(strcmp(file_stats,"new")==0){
        return 1;
    }
    else{
        return 2;
    }
}

// Here we will receive the new file request
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

// Receive status id request
int recv_status_id(int sockfd){
    char status_id[10];
    if (recv(sockfd, status_id, sizeof(status_id), 0) == -1)
    {
        perror("Error receiving file size");
        return -1;
    }
}


// Main Function
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

    vector<thread> workers;
    for (int i = 0; i < MAX_QUEUE; ++i)
    {
        workers.emplace_back([](){
            while (true) {
                int client_sockfd = dequeueClient();
                threadTask(client_sockfd);
            } });
    }

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (true)
    {
        int client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        enqueueClient(client_sockfd);
    }

    close(sockfd);
    return 0;
}