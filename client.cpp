#include <stdio.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/eventfd.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#define MAX_MSG_LENGTH 1024
using namespace std;

int set_nonblock(int fd)
{
	int flags;
#if defined(O_NONBLOCK)
	if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
		flags = 0;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
	flags = 1;
	return ioctl(fd, FIOBIO, &flags);
#endif
} 


int main(int argc, char** argv) {
	//----------------------------------------------------------
	char buffer[MAX_MSG_LENGTH]={};
	
	//----------------------------------------------------------
	//signal (SIGPIPE, SIG_IGN);
	
	//----------------------------------------------------------
	int ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	if(ClientSocket == -1)
	{
		std::cout << strerror(errno) << std::endl;
		return 1;
	}
	//------------------------------------------------------------
	int yes = 1;
	if ( setsockopt(ClientSocket, SOL_SOCKET, SO_NOSIGPIPE, &yes, sizeof(int)) == -1 ){
    	std::cout << strerror(errno) << std::endl;
		return 1;
	}
	//------------------------------------------------------------
	struct sockaddr_in SockAddr;
	
	bzero(&SockAddr, sizeof(SockAddr));
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_port = htons(3100);
	SockAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	//-------------------------------------------------------------
	set_nonblock(ClientSocket);
	//-------------------------------------------------------------
	//активное открытие соединения - отправка SYN (заголовок ip и tcp)
	if ( (connect(ClientSocket, (struct sockaddr *) &SockAddr, sizeof (SockAddr)))<0) {
		std::cout<< strerror(errno)<<std::endl;
		return 1; 	 
	};
	
	int c;
	int i=0;
	int read;
	while ( (c=getchar())!=EOF) {
		if ( c == '\n' ) {
			buffer[i++]=c; 
			printf ("msg_size = %d, msg = %s", i, buffer);		  
			send (ClientSocket, buffer, i,0);
			memset (buffer, 0, MAX_MSG_LENGTH);
			i = 0;
			
			while ( (read = recvfrom (ClientSocket, buffer, (MAX_MSG_LENGTH - (unsigned int)1), 0, (struct sockaddr*) &SockAddr, sizeof(SockAddr))) >0 ) {
	    		buffer[read] = '\0';   
    			printf ("Message from server: %s", buffer);
	   		}
	   		if( read <= 0 ) 	    	
			  	printf ("Nothing from server\n");
	  	}
		else 
			buffer[i++]=c;   
	  
	}
	
	
	
	shutdown (ClientSocket, SHUT_RDWR);
	close    (ClientSocket);
	return 0;
	
}