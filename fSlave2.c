#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

#define BUFLEN 1024

int main(int argc, char *argv[])
{
    //declaring variables
    int listenFD, bytesReceived, nRead;
    char readingBuffer[BUFLEN] = {0};

    //set buffer to zero
    memset(readingBuffer, '0', sizeof(readingBuffer));
    
    //getting port number 
    //creating character arrays 
    struct sockaddr_in serverAddr;
    int portNumber = atoi(argv[2]);
    char input[5] = {0};
    char fileName[20];

    //creating file type pointer
    FILE *filePointer;

    //create array for password, and initialize as null
    //take first argument as ip address 
    const char *password = NULL;
    const char* ipAddr = argv[1];

    //create socket 
    if((listenFD = socket(AF_INET, SOCK_STREAM, 0))< 0)
    {
        printf("[ERROR] Socket Creation Error\n");
        exit(1);
    }


    serverAddr.sin_family = AF_INET;                 //contains code address for family
    serverAddr.sin_port = htons(portNumber);        //htons to convert from host byte order to network byte order
                                                   //in_port contains port

    if(inet_pton(AF_INET, ipAddr, &serverAddr.sin_addr)<=0)  
    { 
        printf("[ERROR] Invalid Address\n"); 
        exit(1); 
    } 

    //check if arguments are at least 3 when program run
    if (argc < 3) 
    {
        printf("[ERROR] Invalid Arguments\n");
        exit(1);
    }

    //connect socket to server
    if(connect(listenFD, (struct sockaddr *)&serverAddr, sizeof(serverAddr))<0)
    {
        printf("[ERROR] Connection Error\n");
        exit(1);
    }

    //third argument is password
    password = argv[3];

    //write on server
    //to identify and verify slave
    write(listenFD, password, sizeof(password));

    printf("[INFO] Connection Successfull\n");
    
    while(1)
    {
        //input checks if user wants to retrieve or send data
        //1 means client wants to send a file
        //2 means that client wants to retrieve a file
        read(listenFD, input, sizeof(input));

        if(strcmp(input,"1") == 0)
        {
            //if 1 start recieving file from client through master
            read(listenFD, fileName, 256);
            printf("[INFO] Recieving File: %s\n",fileName);
            filePointer = fopen(fileName, "ab"); 
            //get file name and create a file of that name
            //assign pointer at start 
            //"ab" to allow all types of files

            if(filePointer == NULL)
            {
                //throw error if file cant open
                printf("[ERROR] File Cannot Be Opened\n");
                exit(1);
            }

            //keep writing till buffer readingBuffer has data
            while((bytesReceived = read(listenFD, readingBuffer, 1024)) > 0)
            {
                fwrite(readingBuffer, 1,bytesReceived,filePointer);
            }

            if(bytesReceived < 0)
            {
                printf("[ERROR] File Cannot Be Read\n");
            }

            //notify when file recieved
            printf("[INFO] File Recieved Successfully\n");
            //close file
            fclose(filePointer);

            //clear buffer
            bzero(input,sizeof(input));
        }

        if(strcmp(input,"2") == 0)
        {   
            //2 means client wants to retireve data 
            read(listenFD, fileName, 256);

            filePointer = fopen(fileName,"rb");

            if(filePointer==NULL)
            {
                printf("[ERROR] File Cannot Be Opened\n");
                exit(1);   
            }   

            while(1)
            {
                //setting buffer to zero
                memset(readingBuffer, '0', sizeof(readingBuffer));
                nRead = fread(readingBuffer,1,1024,filePointer);

                if(nRead > 0)
                {
                    //read from file and write to buffer
                    write(listenFD, readingBuffer, nRead);
                }
                if (nRead < 1024)
                {
                    //if end of file reached tell user
                    if (feof(filePointer))
                    {
                        printf("[INFO] File End Reached\n");
                        printf("[INFO] File Transfer Completed For ID: %d\n",listenFD);
                    }
                    if (ferror(filePointer))
                    {
                        printf("[ERROR] File Cannot Be Read\n");
                    }
                    break;
                    //exit loop when file ends
                }
            }
            //set buffer to zero
            bzero(input,sizeof(input));
        }
    }

    //close socket
    //terminate program
    close(listenFD);
    exit(1);
    return 0;
}
