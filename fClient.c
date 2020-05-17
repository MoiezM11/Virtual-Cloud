#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 

#define BUFLEN 1024

int main(int argc, char const *argv[]) 
{ 
    //variables declared
    int listenFD,valread,bytesReceived;
    
    struct sockaddr_in serverAddr; 

    //charater arrays and buffers
    char readingBuffer[BUFLEN] = {0};
    char value[5] = {0};
    char fileName[20];

    //first argument of main is address of server
    const char* ipAddr = argv[1];
    const char *clientAuth = "client";

    int portNumber = atoi(argv[2]);
    int nRead = 0;
    //file pointer created
    FILE *filePointer;
    

    //assigning descriptor
    if ((listenFD = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        //if descriptor not assigned value -1 returned
        printf("[ERROR] Socket Creation Error\n"); 
        exit(1);
    } 
   
    //setting to zero before assigning value
    memset(&serverAddr, '0', sizeof(serverAddr)); 
   
    serverAddr.sin_family = AF_INET; 
    serverAddr.sin_port = htons(portNumber); 
       
    if(inet_pton(AF_INET, ipAddr, &serverAddr.sin_addr)<=0)  
    { 
        //checks if IP address exists or not 
        printf("[ERROR] Invalid Address\n"); 
        exit(1); 
    } 
   
    if (connect(listenFD,(struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) 
    {
        //establish connection with server
        printf("[ERROR] Connection Error\n");
        exit(1); 
    } 

    write(listenFD,clientAuth,sizeof(clientAuth));
    //clienrAuth already set to 'client' 
    //send to server for identification of client

    valread = read( listenFD , readingBuffer, 1024); 
    printf("%s\n",readingBuffer );
    //printing instruction menu in client terminal
    
    gets(value);
    //takes value from client to specify if client wants to receive or send file
    write(listenFD,value,sizeof(value));
    //send value to server socket 

    if(strcmp(value,"1") == 0)
    {   
        //compare string 
        //if 1, client must start sending file
        //get file name from client
        valread = read(listenFD,readingBuffer,1024);
        printf("%s",readingBuffer);
        gets(fileName);

        write(listenFD,fileName,sizeof(fileName));
        //send file name to server 

        //file opens to begin reading from file and writing to server socket buffer
        filePointer = fopen(fileName,"rb");

        if(filePointer==NULL)
        {
            //if file name incorrect or file doesnt exist
            printf("[ERROR] File Cannot Be Opened\n");
            exit(1);   
        }   

        while(1)
        {
            //setting buffer to zero
            memset(readingBuffer, '0', sizeof(readingBuffer));
            //readsn from file
            //checks to see how many bytes read
            nRead = fread(readingBuffer,1,1024,filePointer);

            //checks to see if bytes came in buffer
            if(nRead > 0)
            {
                //if buffer has contents
                //writes to buffer 
                write(listenFD, readingBuffer, nRead);
            }
            //if buffer contents less than buffer length
            if (nRead < 1024)
            {
                //check if file ended
                if (feof(filePointer))
                {
                    printf("[INFO] File End Reached\n");
                    printf("[INFO] File Transfer Completed For ID: %d\n",listenFD);
                }
                //if error
                if (ferror(filePointer))
                {
                    printf("[ERROR] File Cannot Be Read\n");
                }
                //close file
                fclose(filePointer);
                break;
                //file read, loop may be broken
            }
        }
        //close socket
        close(listenFD);
        exit(1);
    }

    if(strcmp(value,"2") == 0)
    {
        //if client wants to retrieve file
        //read into buffer filename from server socket
        valread = read(listenFD,readingBuffer,1024);
        printf("%s",readingBuffer);
        gets(fileName);
        write(listenFD,fileName,sizeof(fileName));
        //send file name to server

        filePointer = fopen(fileName, "ab");
        //opens file to write
        //'b' allows binary writing, meaning all types of files may be written (jpg,text,mp3 etc)

        if(filePointer == NULL)
        {
            printf("[ERROR] File Cannot Be Opened\n");
            exit(1);
        }

        while((bytesReceived = read(listenFD, readingBuffer, 1024)) > 0)
        {
            //read from buffer and write into file
            fwrite(readingBuffer, 1,bytesReceived,filePointer);
        }

        if(bytesReceived < 0)
        {
            //throw error if file cannot be retireved
            printf("[ERROR] File Cannot Be Retrieved");
        }

        //notify client when file receievd in their folder
        printf("[INFO] File Recieved Successfully\n");
        //close fike
        fclose(filePointer);
        //close socket
        close(listenFD);
        //terminate
        exit(1);
    }

    return 0; 
} 