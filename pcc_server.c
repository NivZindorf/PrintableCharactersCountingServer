/* This code uses parts from  Bash session 10!! */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

//there is 95 integers between 32 and 126
#define PCC_SIZE 95
//define buffer size of 1MB
#define BUFF_SIZE 1048576

//make those variables global in order to get access when SIGINT arrive 
int sigflag=0;
int connfd=-1;
uint32_t pcc_total[PCC_SIZE];

void printAll(uint32_t * pcc_total){
    for(int i=0;i<PCC_SIZE;i++){
        printf("char '%c' : %u times\n",(i+32),pcc_total[i]);
    }
}

void action(){
    if(connfd == -1){
    //because of the flag SA_RESTART, we need to exclude accept() syscall from write/read
    // connfd == -1 iff accept() didnt return yet with a socket
    //so in that case we want to terminate the server
        printAll(pcc_total);
        exit(0);
    }
    else{
    //its mean that we're in the middle of connection with client,so in the next iter we
        sigflag=1;
    }
}
void modSignal(){
    struct sigaction sig;
    memset(&sig,0,sizeof(struct sigaction));
    sig.sa_handler=action;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags=SA_RESTART; // so it won't interrupt read/write from client
    if (sigaction(SIGINT,&sig,NULL)<0){
        perror("Error in SIGACTION");
        exit(1);
    }
    
}
uint32_t readData(char * buff,int connfd,uint32_t * pcc_current,uint32_t * pcc_total,uint32_t N,int * error){
    memset(pcc_current,0,sizeof(uint32_t)*PCC_SIZE);
    uint32_t C=0;
    int curr_size;
    int curr_total_read;
    int total_read;
    int nread;
    total_read=0;
    while(total_read<N){
        if(N-total_read>=BUFF_SIZE){
            curr_size=BUFF_SIZE;
        }
        else{
            curr_size=N;
        }
        curr_total_read=0;
        while(curr_total_read<curr_size){
            if((nread = read(connfd,buff+curr_total_read,curr_size-curr_total_read))<=0){
             perror("Error occured read data");
             if(!(nread ==0 ||errno== ETIMEDOUT || errno == ECONNRESET || errno == EPIPE)){
                exit(1);//this is a real error so we need to exit
             }
             *error=0;
             return 0; // so its a "TCP error" and we  continue to next client 
            }
            curr_total_read +=nread;
        }
        total_read += curr_total_read;
        for(int i=0;i<curr_size;i++){
            if(buff[i]>=32 && buff[i]<=126){
                ++pcc_current[buff[i]-32];
                ++C;
            }
        }
    }
    for(int i=0;i<PCC_SIZE;i++){
        pcc_total[i] += pcc_current[i];
    }
    return C;
}
uint32_t writeData(int connfd,uint32_t C){
    C=htonl(C);
    char * Cbuff=(char*)&C;
    memcpy(Cbuff,&C,sizeof(uint32_t));
    int totalsent=0;
    int nsent;
    while(totalsent<4){
        nsent=write(connfd,Cbuff+totalsent,4-totalsent);
        if (nsent<0){
            perror("Error sending data to server");
            return 0;
            }
        totalsent += nsent;
    }
    return 1;
}
uint32_t setN(int connfd,char *buff){ //todo: check about how to send an INTEGER!!!!!!!!!!
    int size=sizeof(uint32_t); 
    int nread=1;
    int totalread=0;
    while(totalread<size){
        nread = read(connfd,buff+totalread,size-totalread);
        if (nread<=0){
            perror("Error occured read data");
             if(!(nread ==0 ||errno== ETIMEDOUT || errno == ECONNRESET || errno == EPIPE)){
                exit(1);//this is a real error so we need to exit
             }
             return 0; // so its a "TCP error" and we  continue to next client 
            }
        totalread+= nread;
    }
    return 1;
}
int main(int argc, char *argv[]){
    uint32_t pcc_current[PCC_SIZE];
    char buff[BUFF_SIZE];
    if (argc!=2){
        perror("Wrong number of arguments\n");
        return 1;
    }
    //init the counting structure
    //pcc_total[i] = count the times that char #(i+32) recieved
    memset(pcc_total,0,sizeof(uint32_t)*PCC_SIZE);
    modSignal();
    //init the server
    int listenfd = socket( AF_INET, SOCK_STREAM, 0 );
    //setting SO_REUSEADDR from https://stackoverflow.com/questions/24194961/how-do-i-use-setsockoptso-reuseaddr
    const int enable=1;
    if(setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&enable,sizeof(int))<0){
        perror("Error setting SO_REUSEADDR");
        return 1;
    }
    struct sockaddr_in serv_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in );
    memset( &serv_addr, 0, addrsize);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));
    if( 0 != bind( listenfd,(struct sockaddr*) &serv_addr,addrsize) ){
    perror("Error binding the socket\n");
    return 1;
    }
    if(0 != listen(listenfd,10)){
    perror("Error in listen\n");
    return 1;
    }
    int error=0;
    uint32_t N;
    char * Nbuff;
    while(!sigflag){
        error=0;
        connfd=-1;
        connfd=accept(listenfd,NULL,NULL);
        if(connfd<0){
            perror("Error accepting");
            return 1;// Check about this maybe I dont need to exit but continue instead 
        }
        Nbuff=(char *)&N;
        if(setN(connfd,Nbuff)){
            N=ntohl(N);
            uint32_t C =readData(buff,connfd,pcc_current,pcc_total,N,&error);
            if (!error){
                writeData(connfd,C);
                close(connfd);
            }
        }
    }
    printAll(pcc_total);
    return 0;    
}