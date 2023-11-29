#include "funtion_declarations.h"

void *receive_file(void *arg)
{
	while(1)
	{
		struct receiveQueue* t = receive_dequeue(); 
		// cout << "re" << endl;
		int newsockfd = t->sockfd;
		int reqID = t->requestid;
		
		char buffer[BUFFER_SIZE];
		char grade_file[50];

		sprintf(grade_file,"gradeFile%d.c",reqID);

		int file_size = 0;
		int n = read(newsockfd,&file_size,sizeof(file_size));
		cout << "File: " << file_size << endl;
		if (n < 0)
			error("ERROR reading from socket");

		int newGradeFd = open(grade_file,O_RDWR | O_CREAT,S_IRUSR | S_IWUSR | S_IXUSR);
		if (newGradeFd < 0)
			error("ERROR opening file");

		memset(buffer, 0, BUFFER_SIZE);
		while (file_size > 0)
		{	
			int readBytes = read(newsockfd, buffer, BUFFER_SIZE);
			if (readBytes < 0)
				error("ERROR reading from socket");

			int wroteBytes = write(newGradeFd,buffer,readBytes);

			// for(int i=0;i<BUFFER_SIZE;i++)
			// 	cout << buffer[i];

			if (wroteBytes < 0)
				error("ERROR writing to file");

			file_size -= readBytes;

			memset(buffer, 0, BUFFER_SIZE);
		}
		cout << endl;
		process_enqueue(newsockfd,reqID);
		request_status_map[reqID].first = 0;

		write(newsockfd,&reqID,sizeof(reqID));
		// cout << "Sent ReqID: " << reqID << endl;
		close(newGradeFd);
	}
	
}

void *gradeTheFile(void* f)
{
	while(1)
	{
		sleep(30);
		struct processq* t = process_dequeue();
		//cout << "ge" << endl;
		int newsockfd = t->sockfd;
		int reqID = t->requestid;
		
		request_status_map[reqID].first=1;
		
		if (newsockfd < 0) {
			error("ERROR on accept");
		}

		// pthread_t threadID = pthread_self();		
		char buffer[BUFFER_SIZE];
		char Cerror_file[50];
		char Rerror_file[50];
		char output_file[50];
		char diff_file[50];
		char grade_file[50];
		char grade_file_exe[50];

		char compile_command[150];
		char run_command[150];
		char diff_command[150];
		char delete_files[150];

		sprintf(output_file,"output%d.txt",reqID);
		sprintf(Rerror_file,"Rerror%d.txt",reqID);
		sprintf(Cerror_file,"Cerror%d.txt",reqID);
		sprintf(diff_file,"diff%d.txt",reqID);
		sprintf(grade_file,"gradeFile%d.c",reqID);
		sprintf(grade_file_exe,"file%d",reqID);
		
					

		sprintf(compile_command,"gcc gradeFile%d.c -o file%d 2>Cerror%d.txt",reqID,reqID,reqID);
		int compiling = system(compile_command);
		
		if(compiling != 0)
		{
			sprintf(delete_files,"rm gradeFile%d.c",reqID);
			system(delete_files);

			request_status_map[reqID].second = 0;
			// close(cerror);
		}
		else
		{
			sprintf(run_command,"./file%d >output%d.txt",reqID,reqID);
			int runTheFile = system(run_command);

			if(runTheFile != 0)
			{
				//int x=0;
				//write(newsockfd,&x,sizeof(x));
				sprintf(delete_files,"rm gradeFile%d.c file%d Cerror%d.txt",reqID,reqID,reqID);
				system(delete_files);

				request_status_map[reqID].second = 1;
				// close(rerror);
			}
			else
			{
				sprintf(diff_command,"echo -n '1 2 3 4 5 6 7 8 9 10 ' | diff - output%d.txt > diff%d.txt",reqID,reqID);
				int difference = system(diff_command);
				if(difference != 0)
				{
					sprintf(delete_files,"rm gradeFile%d.c file%d output%d.txt Cerror%d.txt",reqID,reqID,reqID,reqID);
					system(delete_files);

					request_status_map[reqID].second = 2;
					// close(diffFd);
				}
				else
				{
					// int x=0;
					// write(newsockfd,&x,sizeof(x));
					// sresult(newsockfd,-1,-1,buffer);
					request_status_map[reqID].second = 3;
					sprintf(delete_files,"rm gradeFile%d.c file%d diff%d.txt Cerror%d.txt",reqID,reqID,reqID,reqID);
					system(delete_files);
				}
				
			}
		}
		close(newsockfd);

		request_status_map[reqID].first=2;
	}
}

void sresult(int newsockfd, int fp, int a, char *buffer) 
{
    memset(buffer, 0, BUFFER_SIZE);
    int x, n = 0;
    
    if(a == 0)
    {
        strcat(buffer, "\nCOMPILE ERROR\n\n");
        x = strlen("\nCOMPILE ERROR\n\n");
    } 
    else if(a == 1) 
    {
        n = send(newsockfd, "\nRUNTIME ERROR\n\n", strlen("\nRUNTIME ERROR\n\n"), 0);
        if (n < 0)
            error("Send failed");
        return;
    } 
    else if(a == 2)
    {
        strcat(buffer, "\nOUTPUT ERROR \n\n");
        x = strlen("\nOUTPUT ERROR \n\n");
    }
	else
    {
        n = send(newsockfd, "PASS\n", strlen("PASS\n"), 0);
        if (n < 0)
            error("Send failed");
        return;
    } 
    int k, flag = 0;
    while (true) 
    {
        if (flag)
            x = 0;
        k = read(fp, buffer + x, BUFFER_SIZE - x);

        flag = 1;
        n = send(newsockfd, buffer, k + x, 0);
        if (n < 0)
            error("Send failed");
        if (k <= 0)
            break;

        memset(buffer, 0, BUFFER_SIZE);
    }
}
