//OC - Ubuntu

#include <iostream>
#include <algorithm>
#include <set>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/eventfd.h>
//#include <sys/event.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
//#include <poll.h>
#include <sys/epoll.h>
#include <errno.h>
#include <string.h>


#define MAX_EPOLL_EVENTS  100
#define MAX_MESSAGE_LENGTH 1024



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

int main (int argc, char **argv) {
  	//------------------------------------------------
  	char buffer[MAX_EPOLL_EVENTS][MAX_MESSAGE_LENGTH] = {}; 
    size_t buffer_length[MAX_EPOLL_EVENTS] = {};
	
	//------------------------------------------------
	std::cout<<"starting server"<<std::endl;
	//-----------------------------------------------------  
  	int MasterSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(MasterSocket == -1)	{
		std::cout << strerror(errno) << std::endl;
		return 1;
	}
	
	struct sockaddr_in SockAddr;
	bzero(&SockAddr, sizeof(SockAddr));
	
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_port = htons(3100); 
	SockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	//-----------------------------------------------------
	//включение опции so_reuseaddr
	int yes = 1;
	if ( setsockopt(MasterSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 ){
    	std::cout << strerror(errno) << std::endl;
		return 1;
	}
  	//----------------------------------------------------
	//связывание сокета с адресом
 	int Result = bind(MasterSocket, (struct sockaddr *)&SockAddr, sizeof(SockAddr));

	if(Result == -1) 	{
		std::cout << strerror(errno) << std::endl;
		return 1;
	} 
	//------------------------------------------------------
	set_nonblock(MasterSocket); //неблокирующий сокет
	//------------------------------------------------------
	//создается очередь запросов на соединение, сокет переводится в режим ожидания запросов со стороны клиентов
	Result = listen(MasterSocket, SOMAXCONN); //2 параметр - максимальное количество клиентских соединений

	if(Result == -1){
		std::cout << strerror(errno) << std::endl;
		return 1;
	}
	//--------------------------------------------------------------
	//создание epoll объекта
	struct epoll_event Event;
	Event.data.fd = MasterSocket;
	Event.events = EPOLLIN | EPOLLET; //|EPOLLPRI
	
	//массив дескрипторов
	struct epoll_event * Events;
	Events = (struct epoll_event *) calloc(MAX_EPOLL_EVENTS, sizeof(struct epoll_event));

	int EPoll = epoll_create1(0); //дескриптор epoll
	if (EPoll == -1) {
    	std::cout << strerror(errno) << std::endl;
		return 1;	  
        }
	
	//Добавляем дескриптор ввода/вывода в массив ожидания
	if (epoll_ctl(EPoll, EPOLL_CTL_ADD, MasterSocket, &Event)<0) {
		std::cout << strerror(errno) << std::endl;
		return 1;	  
	}
	
	
  	//---------------------------------------------------------------
  	while (true) {
	  	//Запускаем ожидание готовности
	  	int N = epoll_wait(EPoll, Events, MAX_EPOLL_EVENTS, -1);
		if (N == -1) {
        	std::cout << strerror(errno) << std::endl;
			return 1;	
        	}
		std::cout << "number of events: " << N << "\n" << std::endl; //********************
		for(unsigned int i = 0; i < N; i++) {
		  
		 	 if((Events[i].events & EPOLLERR)||(Events[i].events & EPOLLHUP)) {//ошибка соединения
				shutdown(Events[i].data.fd, SHUT_RDWR);
				close(Events[i].data.fd);
				std::cout << "connection terminated" << std::endl;
            	for (unsigned int j=0;j<MAX_MESSAGE_LENGTH;j++)
				  	buffer[i][j]=0;
				//memset (buffer[i], 0, MAX_MESSAGE_LENGTH); //************************
            	buffer_length[i] = 0;
			}
			
			else if(Events[i].data.fd == MasterSocket){ //установка соединения
				//Функция accept создаёт для общения с клиентом новый сокет и возвращает его дескриптор. 
	  			//1 параметр задаёт слушающий сокет. После вызова он остаётся в слушающем состоянии 
	  			//и может принимать другие соединения.
	  			//берет первый элемент из очереди клиентов			  
				int SlaveSocket;
				while ( (SlaveSocket = accept (MasterSocket, 0, 0)) != -1 ) {
				//int SlaveSocket = accept(MasterSocket, 0, 0); //*************************
					//if(SlaveSocket == -1) 	{
						//std::cout << strerror(errno) << std::endl;
						//return 1;
					//} 			
				// Недостаточно места в массиве ожидания //*****************
    			/*if (events_cout == MAX_EPOLL_EVENTS-1) {
     				std::cout << "Event array is full" << std::endl;
     				close(SlaveSocket);
     				continue;
    			} */ 
    			
				//-----------------
					int yes = 1;
					if ( setsockopt(MasterSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 ){
   		 				std::cout << strerror(errno) << std::endl;
						return 1;
					}
				//----------------
					set_nonblock(SlaveSocket);
				//---------------
					struct epoll_event Event;
					Event.data.fd = SlaveSocket;
					Event.events = EPOLLIN | EPOLLET;
					if (epoll_ctl(EPoll, EPOLL_CTL_ADD, SlaveSocket, &Event)<0) {
						std::cout << strerror(errno) << std::endl;
						return 1;	  
					}
				//---------------------
				//отправка приветствия 
					const char *msg = "Welcome\n";
		        	if( send (SlaveSocket,msg, strlen (msg), MSG_NOSIGNAL) == -1 ) { //ошибка
		        		shutdown (SlaveSocket, SHUT_RDWR);
		           		close    (SlaveSocket);
		               	std::cout << "connection terminated" << std::endl;
		            }
		            std::cout << "accepted connection" << std::endl;
					continue;
				}
				
			}
			
			else { //клиент готов, чтение из сокета
				static char client_msg[MAX_MESSAGE_LENGTH]; 
				char* ptr_msg;
				int size_msg;
				
				
				
				do {
				  	size_msg = recv (Events[i].data.fd, client_msg, MAX_MESSAGE_LENGTH-(unsigned int)1, MSG_NOSIGNAL);
					client_msg[size_msg] = '\0';
					
					
					
					if (size_msg==-1) {//ошибка чтения
					  	std::cout << strerror (errno) << std::endl;    
						shutdown(Events[i].data.fd, SHUT_RDWR);
						close(Events[i].data.fd);
						std::cout << "connection terminated" << std::endl;
            			for (unsigned int j=0;j<MAX_MESSAGE_LENGTH;j++)
				  			buffer[i][j]=0;
						//memset (buffer[i], 0, MAX_MESSAGE_LENGTH); //*******************
            			buffer_length[i] = 0;
						break;
					}
					if (size_msg == 0 ){//пустое сообщение
						std::cout << "No message" << std::endl;
   	               		shutdown(Events[i].data.fd, SHUT_RDWR);
						close(Events[i].data.fd);
						std::cout << "connection terminated" << std::endl;
						break;
	                   }
	                                
	                
					
					 for( unsigned int k = 0; k < N; k++ ){ 
					   	if ( send (Events[k].data.fd, client_msg, size_msg, MSG_NOSIGNAL) == -1 ) {
	                    	shutdown (Events[k].data.fd, SHUT_RDWR);
	    					close (Events[k].data.fd);
	                   		std::cout << "connection terminated" << std::endl;
	                    	for (unsigned int j=0;j<MAX_MESSAGE_LENGTH;j++)
				  				buffer[k][j]=0;        
	                        //memset (user_buffer[j], 0, MSG_LIMIT); //***************
	                        buffer_length[k] = 0;   
						  
						}
	                }
	                
					
					std::cout << "Message: " << client_msg << std::endl;
					
					
	                if ( size_msg != 0 ) { //сообщение
					  	memcpy (buffer[i], client_msg, size_msg);
	                   	buffer_length[i] = size_msg;
	                   	memset (client_msg, 0, MAX_MESSAGE_LENGTH);
						size_msg = (unsigned int)0;
	                   	}   
	                else {
					  	size_msg=(unsigned int)0;
						memset (client_msg, 0, MAX_MESSAGE_LENGTH);  
					 }
					 
					 
					
				} while (true);
			}
		}
	  	
		
	}
	
	shutdown (MasterSocket, SHUT_RDWR);
	close    (MasterSocket);
	free     (Events);
	return 0;
}


