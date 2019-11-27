/*
Name: Paris Estes
Date: 11/21/18
obj: to create a client 
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/un.h>
#include <sys/time.h>
#include <netdb.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

void * remote_server(void *remote_port);

int main(int argc, char *argv[])
{
	if (argc < 4){
        fprintf(stderr,"usage: %s hostname port rem_ipaddr\n", argv[0]);
		exit(EXIT_FAILURE);
    }

	int sockfd = 0; // socket fd
	int num = 0; // number of read bits
	int read_recv = 0; // number of sent or recieved bits
	int nread = 0; // int for select
	int portnum = 0; // port
	int max_fd = 0; // biggest fd for select
	int count = 0; // for getting num from server input
	int i = 0;
	int total = 0;
	int strSize = 0;

	char buf[1025]; // char array for recv input
	char msg[1025]; // temp char array
	char temp[1025]; // "    "     "
	char super_temp[1025]; // "    "     "

	bool num_exists = false;
	bool just_got_remote_num = false;

	pthread_t remote_server_id;

	struct sockaddr_in serv_addr;
	struct hostent *server;

	fd_set fds;
	
	portnum = atoi(argv[2]);

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("Socket");
        exit(EXIT_FAILURE);
    } 

	if((server = gethostbyname(argv[1])) == NULL){
        fprintf(stderr,"ERROR, no such host\n");
		exit(EXIT_FAILURE);
    }

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portnum);

	if(connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
        perror("connect");
        exit(EXIT_FAILURE);
    }

	max_fd = sockfd + 1;

	while(1)
	{
		write(1, "Enter Client Data: ", 19);
		bzero(buf, sizeof(buf));
		bzero(msg, sizeof(msg));
		bzero(temp, sizeof(temp));
		bzero(super_temp, sizeof(super_temp));

		// setting up all the polling
    	FD_ZERO(&fds);		  // sets fds to zero
    	FD_SET(sockfd, &fds); // sets select for server input
    	FD_SET(0, &fds); 	  // sets select for terminal input

		nread = select(max_fd, &fds, (fd_set *) 0, (fd_set *) 0, (struct timeval *) 0);
		
		if(nread == 1000000){
			printf("I keep getting warnings that nread doesnt get used so here ya go. nread: %d\n", nread);
		}

		if(FD_ISSET(sockfd, &fds)){

			void *remote_total = 0;

			read_recv = read(sockfd, buf, sizeof(buf));

            if(read_recv < 1){
                close(sockfd);
                exit(2);
            }

			memcpy(msg, buf, read_recv-1);
			strncat(msg, "\0", 1);

			if(read_recv < 1){
				close(sockfd);
				exit(0);
			}

			write(1, buf, read_recv);
			write(1, "\n", 1);

			for(i=0;i<read_recv;i++){//scrolls through message 

				if(isdigit(buf[i])){//if the char is a digit

					strncat(temp, &buf[i], 1);//add it to a temp char array

					count++;//increment count
					num_exists = true;
				}
			}
			if(num_exists){
				sscanf(temp, "%d", &num);
				num_exists = false;
			}

			if(total >= 1024 && total <= 49151){//become a server my boi

				write(1, "Assigning handler\n", 18);

				pthread_create(&remote_server_id, NULL, remote_server, &num);

				pthread_join(remote_server_id, &remote_total);

				total = *((int*)remote_total);

				just_got_remote_num = true;
			}
			else{
				write(sockfd, "0", 1);
				close(sockfd);

				write(1, "Disconnecting from Server\n", 26);

				sleep(1);

				int new_sockfd = 0; // socket fd
				int s_port = 0;
				char remote_total_tosend[1025];
				struct sockaddr_in cli_serv_addr;
				char *ip = argv[3];

				bzero(remote_total_tosend, sizeof(remote_total_tosend));
				s_port = num;

				if((new_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
					perror("Socket");
					exit(EXIT_FAILURE);
				} 
			
				bzero((char *) &cli_serv_addr, sizeof(cli_serv_addr));
				cli_serv_addr.sin_family = AF_INET;
				cli_serv_addr.sin_addr.s_addr = inet_addr(ip);
				cli_serv_addr.sin_port = htons(s_port);

				write(1, "Connecting to Client\n", 21);

				if(connect(new_sockfd, (struct sockaddr *) &cli_serv_addr, sizeof(cli_serv_addr)) < 0){
					perror("connect");
					exit(EXIT_FAILURE);
				}
				
				strSize = snprintf( NULL, 0, "%d", total);
            	snprintf(remote_total_tosend, strSize+1, "%d", total);

				write(1, "Sending current total to Remote Client\n", 39);

				send(new_sockfd, remote_total_tosend, strSize+1, 0);
				close(new_sockfd);

				write(1, "Disconnecting from Remote Client\n", 33);
				return 0;
			}
		}
		//if input is recieved from the terminal
		if(FD_ISSET(0, &fds)){

			//reads from terminal
			read_recv = read(0, buf, sizeof(buf));

			//transfers buf to msg and adds \0
			memcpy(msg, buf, read_recv-1);
			strncat(msg, "\0", 1);

			if(read_recv < 1){
				close(sockfd);
				exit(0);
			}
			if(strcmp(msg, "0") == 0){
				write(sockfd, "0", 1);
				sleep(1);
				close(sockfd);
				exit(1);
			}
			if(just_got_remote_num){

				sscanf(buf, "%d", &num);//converts user msg to num
				total+=num; // adds num to the remote clients num

				//converts the number to a string
				strSize = snprintf( NULL, 0, "%d", total);
            	snprintf(super_temp, strSize+1, "%d", total);

				send(sockfd, super_temp, strSize+1, 0); // send server num

				just_got_remote_num = false;
				total = 0;
			}
			else{
				//sends server the msg
				send(sockfd, msg, read_recv + 1, 0);
			}
			
			//resets buf and reads msg from server
			bzero(buf, sizeof(buf));
			read_recv = read(sockfd, buf, sizeof(buf));

			write(1, "Sever Total: ", 13);
			write(1, buf, read_recv); // print message from server
			
			for(i=0;i<read_recv;i++){//scrolls through message 

				if(isdigit(buf[i])){//if the char is a digit

					strncat(temp, &buf[i], 1);//add it to a temp char array

					count++;//increment count
					num_exists = true;
				}
			}
			if(num_exists){
				sscanf(temp, "%d", &num);
				total = num;
				num_exists = false;
			}

			bzero(buf, sizeof(buf)); // reset buffer
			write(1, "\n", 1); // endline
			count = 0; // reset size of number from server
		}//end of if input from terminal
	}
	close(sockfd);
	return 0;
}

void * remote_server(void *remote_port){
	
	write(1, "Handler Assigned\n", 17);

	int new_port = 0; // port for when become server
	int svr_sock = 0;
	int cli = 0;
	int temp = 0;
	struct sockaddr_in my_addr, remote_cli_addr;
	socklen_t remote_cli_len;
	char buffer[1025];
	int sizeof_num = 0;

	bzero(buffer, sizeof(buffer));

	new_port = *((int*)remote_port);

	if((svr_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{
		perror("ERROR opening socket\n");
		exit(0);
	}

	memset(&my_addr, '0', sizeof(my_addr));
  	memset(buffer, '0', sizeof(buffer));

	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	my_addr.sin_port = htons(new_port);

	int on = 1;
  	setsockopt(svr_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	if(bind(svr_sock, (struct sockaddr *) &my_addr, sizeof(my_addr)) < 0){
		perror("bind");
		exit(1);
	}

	listen(svr_sock, 1);
	remote_cli_len = sizeof(remote_cli_addr);

	if((cli = accept(svr_sock, (struct sockaddr *) &remote_cli_addr, &remote_cli_len)) < 0){
		perror("accept");
		exit(2);
	}

	sizeof_num = read(cli, buffer, sizeof(buffer));

	if(sizeof_num < 1){
		close(cli);
		close(svr_sock);
		pthread_exit(0);
	}

	sscanf(buffer, "%d", &temp);

	write(1, "Received ", 9);
	write(1, buffer, sizeof_num);
	write(1, " from Remote Client\n", 20);

	close(cli);
	close(svr_sock);

	write(1, "Remote Client Disconnected\n", 27);

	pthread_exit(&temp);
}