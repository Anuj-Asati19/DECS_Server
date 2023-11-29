Project DECServer - An Asynchronous Autograding Server with Simulation

Members:  
- Anuj Asati | 23M0763
- Pulkit | 23M0782

Github Link : https://github.com/Anuj-Asati19/DECS_Server

1.lab_06 Version 1. 

You must require a client and a server i.e. 2 different laptops/computers.

The two computers should be connected to the same LAN

*** Server Setup:
   - ssh into the other computer where we want to run the client, say labuser
     ```
     ssh labuser@sl2-12.cse.iitb.ac.in
     ```
   - clone the lab_06 file from the GitHub repository
     ```
     Git clone https://github.com/Anuj-Asati19/DECS_Server.git
     ```
   - now change directory to lab06_version1 where you have the lab06 code
     ```
     cd DECS_Server/lab06_version1
     ```
   - Compile the server now
     ```
     g++ -o server server.cpp
     ```
   - Expose it to a port and run
     ```
     ./server 8080
     ```

*** Client Setup:
   - ssh into the other computer where we want to run the client, say labuser
     ```
     ssh labuser@sl2-13.cse.iitb.ac.in
     ```
   - clone the lab_06 file from the GitHub repository
     ```
     Git clone https://github.com/Anuj-Asati19/DECS_Server.git
     ```
   - now change directory to lab06_version1 where you have the lab06 code
     ```
     cd DECS_Server/lab06_version1
     ```
   - Compile the client now
     ```
     g++ -o client client.cpp
     ```
   - Consider the ip address of server is 10.130.154.12. Run client connecting to it
     ```
     ./client compiler_error.cpp 10.130.154.12 8080
     ```

2. Lab_07 Performance analysis:

Building over lab06 version1 only, we will do performance analysis.

*** Server Setup:
   - ssh into the other computer where we want to run the client, say labuser
     ```
     ssh labuser@sl2-14.cse.iitb.ac.in
     ```
   - clone the lab_07 file from the GitHub repository
     ```
     git clone https://github.com/Anuj-Asati19/DECS_Server.git
     ```
   - now change directory to lab07_performance where you have the lab07 code
     ```
     cd DECS_Server/lab07_performance/serverside/
     ```
   - Compile the server now
     ```
     gcc -o server server.c
     ```
   - Expose it to a port and run
     ```
     ./server 8080
     ```

*** Client Setup:
   - ssh into the other computer where we want to run the client, say labuser
     ```
     ssh labuser@sl2-14.cse.iitb.ac.in
     ```
   - clone the lab_07 file from the GitHub repository
     ```
     Git clone https://github.com/Anuj-Asati19/DECS_Server.git
     ```
   - now change directory to lab07_performance where you have the lab07 code
     ```
     cd DECS_Server/lab07_performance/clientside/
     ```
   - run analysis.sh
     ```
     Bash analysis.sh
     ```

3. Lab_08 Version 2:

*** Server Setup:
   - ssh into the other computer where we want to run the client, say labuser
     ```
     ssh labuser@sl2-15.cse.iitb.ac.in
     ```
   - clone the lab_08 file from the GitHub repository
     ```
     git clone https://github.com/Anuj-Asati19/DECS_Server.git
     ```
   - now change directory to lab08_version2
     ```
     cd DECS_Server/lab08_version2/servers/
     ```
   - Compile the server now
     ```
     gcc -o multiserver multiserver.c
     ```
   - Expose it to a port and run
     ```
     ./server 8080
     ```

*** Client Setup:
   - ssh into the other computer where we want to run the client, say labuser
     ```
     ssh labuser@sl2-16.cse.iitb.ac.in
     ```
   - clone the lab_08 file from the GitHub repository
     ```
     git clone https://github.com/Anuj-Asati19/DECS_Server.git
     ```
   - now change directory to lab08_version2
     ```
     cd DECS_Server/lab08_version2/clients/
     ```
   - run analysis.sh
     ```
     Bash analysis.sh
     ```

4. Lab_09 Version 3:

*** Server Setup:
   - ssh into the other computer where we want to run the client, say labuser
     ```
     ssh labuser@sl2-17.cse.iitb.ac.in
     ```
   - clone the lab_09 file from the GitHub repository
     ```
     git clone https://github.com/Anuj-Asati19/DECS_Server.git
     ```
   - now change directory to lab09_version3
     ```
     cd DECS_Server/lab09_version3/servers/
     ```
   - Compile the server now
     ```
     gcc -o multiserver multiserver.c
     ```
   - Expose it to a port and run
     ```
     ./multiserver 8080 10
     ```

*** Client Setup:
   - ssh into the other computer where we want to run the client, say labuser
     ```
     ssh labuser@sl2-18.cse.iitb.ac.in
     ```
   - clone the lab_09 file from the GitHub repository
     ```
     Git clone https://github.com/Anuj-Asati19/DECS_Server.git
     ```
   - now change directory to lab09_version3
     ```
     cd DECS_Server/lab09_version3/clients/
     ```
   - run analysis.sh
     ```
     Bash analysis.sh
     ```

5. Version 4:

*** Server Setup:
   - ssh into the other computer where we want to run the client, say labuser
     ```
     ssh labuser@sl2-17.cse.iitb.ac.in
     ```
   - clone the final_version file from the GitHub repository
     ```
     git clone https://github.com/Anuj-Asati19/DECS_Server.git
     ```
   - now change directory to final_version
     ```
     cd DECS_Server/final_version/serverside/
     ```
   - Compile the server now
     ```
     g++ -o server server.c FileQueue.cpp
     ```
   - Expose it to a port and run
     ```
     ./server 8080 10
     ```

*** Client Setup:
   - ssh into the other computer where we want to run the client, say labuser
     ```
     ssh labuser@sl2-18.cse.iitb.ac.in
     ```
   - clone the lab_09 file from the GitHub repository
     ```
     git clone https://github.com/Anuj-Asati19/DECS_Server.git
     ```
   - now change directory to final_version
     ```
     cd DECS_Server/final_version/clientside/
     ```
   - run analysis.sh
     ```
     bash analysis.sh
     ```






















