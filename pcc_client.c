/* This code uses parts from  Bash session 10!! */

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

//define buffer size of 1MB
#define BUFF_SIZE 1048576
//#define BUFF_SIZE 32

int sendData(int sockfd,char * buff,int size){
        int totalsent=0;
        int nsent;
        while(totalsent<size){
            nsent=write(sockfd,buff+totalsent,size-totalsent);
            if (nsent<0){
                perror("Error sending data to server");
                return 0;
                }
            totalsent += nsent;
        }
        return 1;
}
int readData(int sockfd,char *buff){
    int size=sizeof(uint32_t); 
    int nread=1;
    int totalread=0;
    while(nread>0){
        nread = read(sockfd,buff+totalread,size-totalread);
        totalread+= nread;
    }
    if (totalread == size){
        return 1;
    }
    else{//an error ocuured
    return -1;
    }

}

int main(int argc, char *argv[]){
    // checking input
    if (argc !=4){
        perror("Wrong number of arguments\n");
        return 1;
    }
    FILE * filedesc = fopen(argv[3],"r");
    if (filedesc<0){
        perror("Error: cant open file\n");
        return 1;
    }
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(struct sockaddr_in));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd<0){
        perror("Error: cant create socket\n");
        return 1;
    }
    if( connect(sockfd,(struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Error: cant connet to server\n");
        return 1;
    }
    // checking the length of the file,
    // from : https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
    fseek(filedesc,0,SEEK_END);
    uint32_t N=ftell(filedesc);
    uint32_t Nserver = htonl(N);
    char * Nbuff = (char *)&Nserver;
    fseek(filedesc,0,SEEK_SET);
    if (!sendData(sockfd,Nbuff,sizeof(uint32_t))){
        return 1;
    }
    int nread;
    char buff [BUFF_SIZE];
    //send the file to server by chunks of 1MB
    while((nread=fread(buff,1,BUFF_SIZE,filedesc))>0){
        if (!sendData(sockfd,buff,nread)){
            return 1;
        }
    }
    //getting C back from server
    uint32_t C;
    char * Cbuff=(char*)&C;
    if(read(sockfd,Cbuff,sizeof(uint32_t))<0){
        perror("Error getting C");
        return 1;
    }
    C= ntohl(C); // transtle back to human's lang
    close(sockfd);
    printf("# of printable characters: %u\n",C);
    return 0;
}