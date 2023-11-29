#include <helper.cpp>
#include <iostream>
#include <fstream>
#include <cstring>
#include <thread>
#include <filesystem>
#include <vector>
#include <queue>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <uuid/uuid.h>

using namespace std;
namespace fs = std::filesystem;

const int BUFFER_SIZE = 1024;
const int MAX_FILE_SIZE_BYTES = 4;
const int MAX_FILE_STAT_BYTES = 10;
const int MAX_QUEUE = 50;

// const char SUBMISSIONS_DIR[] = "./submissions/";
// const char EXECUTABLES_DIR[] = "./executables/";
// const char OUTPUTS_DIR[] = "./outputs/";
// const char COMPILER_ERROR_DIR[] = "./compiler_error/";
// const char RUNTIME_ERROR_DIR[] = "./runtime_error/";
// const char EXPECTED_OUTPUT[] = "./expected/output.txt";

// const char PASS_MSG[] = "PROGRAM RAN\n";
// const char COMPILER_ERROR_MSG[] = "COMPILER ERROR\n";
// const char RUNTIME_ERROR_MSG[] = "RUNTIME ERROR\n";
// const char OUTPUT_ERROR_MSG[] = "OUTPUT ERROR\n";

// Initialize pthread locks and condition variables
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_condition = PTHREAD_COND_INITIALIZER;

string generateUUID() {
    uuid_t uuid;
    uuid_generate(uuid);

    char uuidStr[37];
    uuid_unparse(uuid, uuidStr);

    return std::string(uuidStr);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Usage: ./server <port>\n";
        return -1;
    }
    int port = stoi(argv[1]);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Socket creation failed");
        return 1;
    }
    struct sockaddr_in serv_addr,cli_addr;
    socklen_t clilen;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    int iSetOption = 1;
    clilen = sizeof(cli_addr);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&iSetOption, sizeof(iSetOption));
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return -1;
    }
    if (listen(sockfd, MAX_QUEUE) != 0) {
        perror("Listen failed");
        close(sockfd);
        return -1;
    }
    cout << "Server listening on port: " << port << "\n";

    int threadPoolSize = atoi(argv[2]);
     pthread_t grader_threads[threadPoolSize];
    for (int i = 0; i < threadPoolSize; i++) 
	{
        if (pthread_create(&grader_threads[i], nullptr, gradeTheFile, NULL) != 0){
            fprintf(stderr, "There is an error in creating the thread\n");
        }
    }

	pthread_t receive_thread[20];
    for (int i = 0; i < 20; i++) {
        if (pthread_create(&receive_thread[i], nullptr, receiveTheFile, nullptr) != 0){
            fprintf(stderr, "Error Creating Thread\n");
        }
    }

    while (true) 
	{
        int *newsockfd = (int *)(malloc(sizeof(int)));
        if (newsockfd == nullptr) 
		{
            fprintf(stderr, "Error: Unable to allocate memory for newsockfd\n");
            continue;
        }
        *newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

        if (*newsockfd < 0) 
		{
            fprintf(stderr, "Error: accept failed\n");
            free(newsockfd);
            continue;
        }
		
		char req_type[10];
		read(*newsockfd,&req_type,sizeof(req_type));

		if(strcmp(req_type,"new")){
			string id = generateUUID();
			someenqueue(id,somequeue);
		}
		else
		{
			string id;
			read(*newsockfd,&id,sizeof(id));
			status_enqueue(*newsockfd, id);
			pthread_t status_thread;
			if (pthread_create(&status_thread, nullptr, checkStatus, NULL) != 0)
			{
				fprintf(stderr, "Error Creating Thread for checking status\n");
			}
		}

        
    }
    return 0;
}
