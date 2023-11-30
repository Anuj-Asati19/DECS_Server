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
#include<queue>
#include<pthread.h>
#include "FileQueue.h"
#include <chrono>
#include<random>
#include<cassert>
#include<string>

using namespace std;
namespace fs = std::filesystem;

struct ThreadArgs {
    std::string request_id;
    int client_sockfd;
};

int Size;
pthread_mutex_t Lock=PTHREAD_MUTEX_INITIALIZER, fileLock=PTHREAD_MUTEX_INITIALIZER,StatusFileLock=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t space_available=PTHREAD_COND_INITIALIZER, items_available=PTHREAD_COND_INITIALIZER, data_available=PTHREAD_COND_INITIALIZER;

FileQueue fileQueue("RequestQueue.txt"); // Pass the filename to the constructor
FileQueue statusQueue("StatusQueue.txt");
FileQueue Queue("queue.txt");
const int BUFFER_SIZE = 1024;
const int MAX_FILE_SIZE_BYTES = 4;
const int MAX_QUEUE = 50;

const char SUBMISSIONS_DIR[] = "./submissions/";
const char EXECUTABLES_DIR[] = "./executables/";
const char OUTPUTS_DIR[] = "./outputs/";
const char COMPILER_ERROR_DIR[] = "./compiler_error/";
const char RUNTIME_ERROR_DIR[] = "./runtime_error/";
const char DIFF_ERROR_DIR[]="./diff_error/";
const char EXPECTED_OUTPUT[] = "./expected/output.txt";
const char PASS_MSG[] = "PROGRAM RAN";
const char COMPILER_ERROR_MSG[] = "COMPILER ERROR";
const char RUNTIME_ERROR_MSG[] = "RUNTIME ERROR";
const char OUTPUT_ERROR_MSG[] = "OUTPUT ERROR";

mutex thread_mutex;
int total_requests = 0;
int served_requests = 0;

string generateUUID() {
    auto currentTime = chrono::system_clock::now();
    auto durationSinceEpoch = currentTime.time_since_epoch();
    auto secondsSinceEpoch = chrono::duration_cast<std::chrono::nanoseconds>(durationSinceEpoch);
    time_t timeInNano = secondsSinceEpoch.count();
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(1000, 9999);
    int randomNum = dis(gen);
    string uuid = to_string(timeInNano) + to_string(randomNum);
    return uuid;
}


struct Result {
    string result_msg;
    string result_details;
};

Result findResult(const string& filename) {
    ifstream file("StatusQueue.txt");
    string line;
    Result result;

    while (getline(file, line)) {
        istringstream iss(line);
        string file, result_msg, result_details;
        if (getline(iss, file, ',') &&
            getline(iss, result_msg, ',') &&
            getline(iss, result_details)) {
            if (file == filename) {
                result.result_msg = result_msg;
                result.result_details = result_details;
                return result; // Return result when filename is found
            }
        }
    }

    // If filename is not found, set an indicator in the result
    result.result_msg = "File not found or no data associated";
    return result;
}

int send_file(int sockfd, const char* file_path)
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

void* start_worker(void *)
{
    while(1){
        string filename;
        pthread_mutex_lock(&Lock);
        while(Queue.size()==0){
            pthread_cond_wait(&items_available,&Lock);
        }
        filename=Queue.pop();
        pthread_cond_signal(&space_available);
        pthread_mutex_unlock(&Lock);
        
	int worker_id = (int) pthread_self();
		
    string source_file =  filename;
    size_t lastSlashPos = source_file.find_last_of('/');
    size_t lastDotPos = source_file.find_last_of('.');
    string result = source_file.substr(lastSlashPos + 1, lastDotPos - lastSlashPos - 1);
    string executable = EXECUTABLES_DIR + result + ".o";
    string output_file = OUTPUTS_DIR + result + ".txt";
    string compiler_error_file = COMPILER_ERROR_DIR + result + ".err";
    string runtime_error_file = RUNTIME_ERROR_DIR + result + ".err";
    string diff_error_file=DIFF_ERROR_DIR + result+ ".err";
    string compile_command = "g++ " + source_file + " -o " + executable + " > /dev/null 2> " + compiler_error_file;
    string run_command = executable + " > " + output_file + " 2> " + runtime_error_file;
    string diff_command="diff ./expected/output.txt " + output_file + " > " + diff_error_file;

    string result_msg = "";
    string result_details = "";

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
    else if (system(diff_command.c_str())!=0){
    	result_msg=OUTPUT_ERROR_MSG;
    	result_details = diff_error_file;
    }
    else 
    {
        result_msg = PASS_MSG;
        result_details = output_file;
    }
    
    string status=filename+","+result_msg+","+result_details;
    
    pthread_mutex_lock(&StatusFileLock);
    statusQueue.push(status.c_str());
    pthread_mutex_unlock(&StatusFileLock);
 
    }

}

void* grader(void *){
	while(1){
	pthread_mutex_lock(&fileLock);
	if(fileQueue.isEmpty()){
		pthread_cond_wait(&data_available,&fileLock);
	}
	string filename=fileQueue.pop();
	
	pthread_mutex_unlock(&fileLock);
	
	pthread_mutex_lock(&Lock);
       	while(Queue.size()==Size){
            pthread_cond_wait(&space_available,&Lock);
        }
        Queue.push(filename.c_str());
        pthread_cond_signal(&items_available);
        pthread_mutex_unlock(&Lock);
	
 	}
 	pthread_exit(nullptr);

}


void* serve_request(void* args) {
    ThreadArgs* threadArgs = reinterpret_cast<ThreadArgs*>(args);

    // Access the parameters in the thread function

    string request_id = threadArgs->request_id;
    int client_sockfd = threadArgs->client_sockfd;
    string submit(SUBMISSIONS_DIR);
    string filename=submit+"testfile_"+request_id+".cpp";
   
    int pos=Queue.findPosition(filename.c_str());
    if(pos!=-1){
    	string position=to_string(pos);
    	send(client_sockfd,"0",sizeof(int),0);
    	send(client_sockfd,position.c_str(),position.length(),0);
    	pthread_exit(nullptr);
    }
    pos=fileQueue.findPosition(filename.c_str());
    if(pos!=-1){
    	pos+=Queue.size();
    	string position=to_string(pos);
    	send(client_sockfd,"0",sizeof(int),0);
    	send(client_sockfd,position.c_str(),position.length(),0);
    	pthread_exit(nullptr);
    }
    
    string result_Msg,result_Details;
    Result foundResult = findResult(filename.c_str());
    if (!foundResult.result_msg.empty() && foundResult.result_msg != "File not found or no data associated") {
        result_Msg=foundResult.result_msg;
        result_Details=foundResult.result_details;
    } else {
    	send(client_sockfd,"1",sizeof(int),0);
        pthread_exit(nullptr);
    }
    // File containing data
    ifstream inputFile(result_Details); 
    string file_to_send="./results/result_"+request_id+".txt";

	// New file to store combined content
    ofstream outputFile(file_to_send.c_str(), ios::app); 

    if (outputFile.is_open() && inputFile.is_open()) {
        outputFile << result_Msg << endl; // Append result_Msg to the new file

        // Append data from the input file to the new file
        outputFile << inputFile.rdbuf();

        inputFile.close(); // Close the input file
        outputFile.close(); // Close the output file

    } 
    send(client_sockfd,"2",sizeof(int),0);
    if(send_file(client_sockfd,file_to_send.c_str())!=0){
    	perror("Error in sending result to client");
    	pthread_exit(nullptr);
    }
    
    pthread_exit(nullptr);     
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cerr << "Usage: ./server  <port> <pool-size>\n";
        return -1;
    }
     
    Size=atoi(argv[2]);
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

	// creating the directories
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
        if (!fs::exists(DIFF_ERROR_DIR))
            fs::create_directories(DIFF_ERROR_DIR);
        if (!fs::exists(EXPECTED_OUTPUT))
            fs::create_directories(EXPECTED_OUTPUT);
        
    }
    catch (fs::filesystem_error &e)
    {
        cerr << "Error creating directories: " << e.what() << "\n";
        close(sockfd);
        return -1;
    }
    
    // creating the threadpool
    pthread_t receive_thread[Size];
	for (int i = 0; i < Size; i++) 
	{
        if (pthread_create(&receive_thread[i], nullptr, start_worker, nullptr) != 0)
		{
            fprintf(stderr, "Error Creating Thread\n");
        }
    }
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    
    pthread_t grader_var;
    if(pthread_create(&grader_var,nullptr, grader,nullptr) !=0){
    	fprintf(stderr,"Error creating grading thread\n");
    	return 1;
    }

    // server will run forever
    while (true)
    {
        int client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        int size_int=sizeof(int);
        char request_type[size_int];
    	if (recv(client_sockfd, request_type, sizeof(request_type), 0) == -1)
    	{
        	perror("Error receiving file size");
        	return -1;
    	}
    	int type=atoi(request_type);
    	
    	//new request
    	if(type==0){ 
    		string request_id=generateUUID();
    		string submit(SUBMISSIONS_DIR);
    		string filename=submit+"testfile_"+request_id+".cpp";
    		pthread_mutex_lock(&fileLock);	
    		fileQueue.push(filename.c_str());
    		 if (recv_file(client_sockfd, filename) != 0)
    		{
        		close(client_sockfd);
        		continue;
    		}
    		pthread_cond_signal(&data_available);
    		pthread_mutex_unlock(&fileLock);
    		cout<<"File Received"<<endl;
    		string message="Submission accepted for grading , your request id is: "+request_id;
    		send(client_sockfd,message.c_str(),message.length(),0);
    		
    	}
    	
    	// status request
    	else{
    		char request[256];
    		recv(client_sockfd,request,255,0);
    		string request_id=request;
    		ThreadArgs* args = new ThreadArgs();
    		args->request_id = request_id;
    		args->client_sockfd = client_sockfd;
    		cout<<"Sending Status"<<endl;
    		pthread_t find_request;
    		int rc=pthread_create(&find_request,nullptr, serve_request,  reinterpret_cast<void*>(args));
    		assert(rc==0);
    		pthread_detach(rc);
    		
    	}        
    }

    close(sockfd);
    return 0;
}
