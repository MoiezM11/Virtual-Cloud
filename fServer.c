#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <stdbool.h> 
#include <pthread.h>

#define BUFLEN 1024

//defining all variables and structures to be used 

struct sockaddr_in clientAddr;
struct sockaddr_in serverAddr;

//file descriptors for slaves and android 
int slaveOne, slaveTwo, androidDevice, bytesReceived;
char fileName[20];
char readingBuffer[BUFLEN] = {0};

//function names defined
void dataSending(int,int);
void recieveFileFromClient(int);
void retrieveData(int);
void* client(void *);
void* android(void *);

int main(int argc, char const *argv[])
{
	int connFD, listenFD;
	//accept socket and listensocket
	pthread_t pid;
	//thread ID defined by pid

    listenFD = socket(AF_INET, SOCK_STREAM, 0);
    //using TCP protocol 
    socklen_t clientLen = sizeof(clientAddr);

    int port = atoi(argv[1]);
    bool slaveFlag = false;

    if(listenFD<0)
	{
	  //if socket descriptor is -1
	  printf("[ERROR] Socket Creation Error\n");
	  exit(2);
	}

    printf("[INFO] Socket Created Successfully\n");

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(port);

    if((bind(listenFD,(struct sockaddr*)&serverAddr,sizeof(serverAddr)))<0)
    {
      //if socket already in use, bind will fail	
      printf("[ERROR] Socket Binding Failed\n");
      exit(2);
    }

    if(listen(listenFD, 10) == -1)
    { 
    	//maximum 10 connections 
        printf("[ERROR] Socket Listening Error\n");
        exit(2);
    }

    while(1)
    {    
        while(!slaveFlag)
        {	
        	//slave flag will be true when both slaves connect
        	//no other client allowed to connect until cloud is active(slaves are connected)
        	//loop continues till both slaves connect 

        	printf("[INFO] Waiting For Slave(s) To Be Connected\n");

            connFD = accept(listenFD, (struct sockaddr*)&clientAddr,&clientLen);
    		//slave connected 

        	if(connFD<0)
        	{
            	printf("[ERROR] Accept Error\n");
            	continue;	
        	}
        	else
        	{
                bytesReceived = read(connFD, readingBuffer, sizeof(readingBuffer));
                //takes password from slave to verify 

                if(strcmp(readingBuffer,"12") == 0)
                {	//password for slave1 = 12
                    slaveOne = connFD;
                    printf("[INFO] Slave One Connected With IP: %s\n", inet_ntoa(clientAddr.sin_addr));
                    continue;
                }
                if(strcmp(readingBuffer,"34") == 0)
                {	
                	//password for slave two = 34
                    slaveTwo = connFD;
                    printf("[INFO] Slave Two Connected With IP: %s\n", inet_ntoa(clientAddr.sin_addr));
                    slaveFlag = true;
                    break;
                }
                else
                {
                	//if incorrect password
                    printf("[ERROR] Invalid Slave. Server Exit.\n");
                    exit(1);
                }
        	}
       	}

        connFD = accept(listenFD, (struct sockaddr*)&clientAddr,&clientLen);
        //once slaves connected, android or client may connect to cloud

        bytesReceived = read(connFD, readingBuffer, sizeof(readingBuffer));
        //android sends 'androidO' when connected, client must enter 'client'
        //compare string

        if(strcmp(readingBuffer,"client") == 0)
        {	
        	//if client
            printf("[WELCOME] PC Client Connected\n"); 
            printf("[INFO] Welcome Client\n");
            //new thread generated
            //thread will go to function 'client'
            pthread_create(&pid, NULL, client, (void *)(intptr_t)connFD); 
        }
          
        if(strcmp(readingBuffer,"androidO") == 0)
        {
        	//if android
            androidDevice = connFD;
            printf("[WELCOME] Android Device Connected\n");
            printf("[INFO] Welcome Android Client\n");
            //new thread created
            //thread goed to function 'android'
            pthread_create(&pid, NULL, android, (void *)(intptr_t)androidDevice);
            //intptr_t for castings
        }
    }

    //closing all sockets upon code termination
    close(slaveOne);
    close(slaveTwo);
    close(androidDevice);
    close(listenFD);
	return 0;
}


//datasending called in android and receivefilefromclient functions
//when android sends location/temperature data

void dataSending(int connFD, int slaveFD)
{

	//file bytes loaded directly to buffer
	//server writes bytes to slave buffer
	//slave will save as file
    while((bytesReceived = read(connFD, readingBuffer, 1024)) > 0)
    { 
        write(slaveFD, readingBuffer, bytesReceived);
    }

    if(bytesReceived < 0)
    {
        printf("[ERROR] File Cannot Be Read\n");
    }
   
    printf("[INFO] File Sent Successfully\n");
}

//when client wants to transfer files
//all client files transferred to slave1
void recieveFileFromClient(int connFD)
{   
	//'1' will inform slave that client wants to send file
    write(slaveOne,"1",2);
    write(slaveOne, fileName, sizeof(fileName));
    //function called to send data, slaveOne passed as parameter
    dataSending(connFD,slaveOne);
}


//client may ask to retrieve files saved in slaves
void retrieveData(int connFD)
{
    if(strcmp(fileName,"androiddata.txt") == 0)
    {
    	//android data saved in slaveTwo
    	//'2' indicates to slave that file requested to be retrieved
        write(slaveTwo,"2",2);
        write(slaveTwo,fileName,sizeof(fileName));
        dataSending(slaveTwo,connFD);
    }
    else
    {	
    	//all other data of client stored in slaveOne 
    	//'2' to specify retrieval
        write(slaveOne,"2",2);
        write(slaveOne,fileName,sizeof(fileName)); //send name of file to slaves buffer
        dataSending(slaveOne,connFD);
    } 
}

//client thread will run this function in parallel to main thread
void* client(void *arg)
{
    int connFD=(intptr_t)arg;
    char Menu[] = "\nWELCOME TO VIRTUAL CLOUD\n\nPress 1 To Send A File\nPress 2 To Retrieve A File\nInput: ";
    //welcome message displayed to client
    write(connFD,Menu,sizeof(Menu));
    bytesReceived = read(connFD, readingBuffer, BUFLEN);
    //client enters number, retrieve or store
    readingBuffer[bytesReceived] = '\0';

    if(strcmp(readingBuffer,"1") == 0)
    {
    	//filename, global character, taken from client's buffer(reading buffer)
        write(connFD,"Enter File Name: ",18);
        bytesReceived = read(connFD, readingBuffer, BUFLEN);
        readingBuffer[bytesReceived] = '\0';
        strcpy(fileName,readingBuffer);
        //function called
        recieveFileFromClient(connFD);
    }
    if(strcmp(readingBuffer,"2") == 0)
    {
        write(connFD,"Enter File Name: ",18);
        bytesReceived = read(connFD, readingBuffer, BUFLEN);
        readingBuffer[bytesReceived] = '\0';
        strcpy(fileName,readingBuffer);
        //function called
        retrieveData(connFD);
    }
}

void* android(void *arg)
{
	//data from android sent to slaveTwo
    androidDevice = (intptr_t)arg;
    write(slaveTwo,"1",2);
    write(slaveTwo,"androiddata.txt", 16);
    dataSending(androidDevice,slaveTwo);
}