#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <pthread.h>

//struct to store total and fd for each client
struct Client{
	int cli_fd;
	int total;
};

//declare 2 global Clients 
struct Client client1;
struct Client client2;

/* Function to add to client's total and respond to client based on that total */
int addNewNumber(int val, int sock, int client_no);

/* Function to check if the client's total is within range */
/* Returns 1 if within range or 0 if not */
int checkSum(int val);

/* thread handler function to read messages from clients */
void *connect_client(void *);

int main(int argc, char *argv[])
{
	/* set the client totals to zero */
	/* set the client fds to -1 */
	client1.total=0;
	client1.cli_fd=-1;
	client2.total=0;
	client2.cli_fd=-1;
	
	int sockfd, new_fd;
	struct sockaddr_in my_addr, their_addr;
	size_t sin_size;
	int port;

	// stdderr if < 2 
	if (argc < 2)
	{
		printf("usage: %s <svr_port>\n", argv[0]);
		exit(1);
	}
	
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("ERROR: socket\n");
		exit(1);
	}
	
	
	port = atoi(argv[1]);
	
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(my_addr.sin_zero), 8);

	int on = 1;
  	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	
	if(bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1){
		perror("ERROR: bind\n");
		exit(1);
	}
	
	if(listen(sockfd, 10) == -1){
		perror("ERROR: listen");
		exit(1);
	}
	
	printf("Waiting for incoming connections ... \n");
	
	
	sin_size = sizeof(their_addr);
	pthread_t tid[2];	
	char ret[255];
	
	//continue as long as new connections are accepted
	while( (new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size) ) != -1){
		sleep(1);
		
		//assign to thread and call thread handler or send message if too many connections
		if(client1.cli_fd == -1){
			client1.cli_fd = -2;
			if(pthread_create(&tid[0], NULL, connect_client, (void*) &new_fd) == -1){
				perror("ERROR: thread\n");
			}
		}
		else if(client2.cli_fd == -1){
			client2.cli_fd = -2;
			if(pthread_create(&tid[1], NULL, connect_client, (void*) &new_fd) == -1){
				perror("ERROR: thread\n");
			}
		}
		else{
			bzero(ret, sizeof(ret));
			//send client message : too many clients
			strcpy(ret, "Too Many Clients Connected. Disconnecting ...");
			write(new_fd, ret, sizeof(ret));
			close(new_fd);
		}
				
		printf("Connection accepted\n");
		printf("Handler assigned\n");
	}
	
	close(new_fd);
	close(sockfd);

	return 0; 
}

int addNewNumber(int val, int sock, int client_no){
	//other_client = other client's number
	//strSize = size of message to send client
	//check = value returned by checkSum function
	int other_client = 0, strSize = 0, check = 0;
	
	//ret = message to return to this client
	//return2 = message to return to other client
	char ret[1025], return2[1025];

	bzero(ret, sizeof(ret));
	bzero(return2, sizeof(return2));
	
	//check the client number, add to total, and call checkSum()
	if(client_no == 1){
		client1.total += val;
		other_client = 2;
		printf("CLIENT %d:  %d - Total: %d\n", client_no, val, client1.total);
		check = checkSum(client1.total);
	}
	else{
		client2.total += val;
		other_client = 1;
		printf("CLIENT %d:  %d - Total: %d\n", client_no, val, client2.total);
		check = checkSum(client2.total);
	}
	
	//if total reaches desired value, send appropriate messages	
	if(check == 1){
	//total within range
		if(client_no == 1){
			strSize = snprintf( NULL, 0, "%d", client1.total);
			snprintf(ret, strSize+1, "%d", client1.total);
			ret[sizeof(ret)] = '\0';
			write(sock, ret, strSize+1);
		}
		else{
			strSize = snprintf( NULL, 0, "%d", client2.total);
			snprintf(ret, strSize+1, "%d", client2.total);
			ret[sizeof(ret)] = '\0';
			write(sock, ret, strSize+1);
		}
		if(client_no == 1){
			printf("Sending Client %d Port %d to Client %d, Reset Total\n", client_no, client1.total, other_client);
			bzero(ret, sizeof(ret));
			strSize = snprintf( NULL, 0, "%d", client1.total);
			snprintf(ret, strSize+1, "%d", client1.total);
			bzero(return2, sizeof(return2));
			ret[strSize+2] = '\0';
			strcpy(return2, "PORT ");
			strncat(return2, ret, strSize+1);
			send(sock, return2, strSize+6, 0);
			if(client2.cli_fd != -1){
				send(client2.cli_fd, return2, strSize+6, 0);
			}
		}
		else{
			printf("Sending Client %d Port %d to Client %d, Reset Total\n", client_no, client2.total, other_client);
			bzero(ret, sizeof(ret));
			strSize = snprintf( NULL, 0, "%d", client2.total);
			snprintf(ret, strSize+1, "%d", client2.total);
			bzero(return2, sizeof(return2));
			ret[strSize+2] = '\0';
			strcpy(return2, "PORT ");
			strncat(return2, ret, strSize+1);
			send(sock, return2, strSize+6, 0);
			if(client2.cli_fd != -1){
				send(client1.cli_fd, return2, strSize+6, 0);
			}
		}
		printf("Client %d Disconnected, Reset Total\n", other_client);
		client1.total=0;
		client2.total=0;
		return 0;
	}
	// just send client's current total
	else {
		// send total to client
		if(client_no == 1){
			strSize = snprintf( NULL, 0, "%d", client1.total);
			snprintf(ret, strSize+1, "%d", client1.total);
			ret[sizeof(ret)] = '\0';
			write(sock, ret, strSize+1);
		}
		else{
			strSize = snprintf( NULL, 0, "%d", client2.total);
			snprintf(ret, strSize+1, "%d", client2.total);
			ret[sizeof(ret)] = '\0';
			write(sock, ret, strSize+1);
		}
	}
	return 1;
}

int checkSum(int val){
	if( val >= 1024 && val <= 49151)
		return 1;
	else
		return 0;
}

void *connect_client(void *socket){
	int this_sock = *(int*)socket;
	int client_no = 0; 
	int msg_size = 0, num = 0;
	int return_val = 0;
	char msg[1025];
	
	if(client1.cli_fd == -2){
		client1.cli_fd = this_sock;
		client_no=1;
	}
	else {
		client2.cli_fd = this_sock;
		client_no=2;
	}
	
	while(1){

		msg_size = recv(this_sock, msg, sizeof(msg), 0);
		if(msg_size < 1){
			close(this_sock);
			pthread_exit(0);
		}
		msg[msg_size] = '\0';
		sscanf(msg, "%d", &num);
		if(num == 0){
			if(client_no == 1){
				client1.cli_fd = -1;
				client1.total = 0;
				close(client2.cli_fd);
				printf("Client %d Disconnected, Reset Total\n", client_no);
			}
			else if(client_no == 2){
				client2.cli_fd = -1;
				client2.total = 0;
				close(client1.cli_fd);
				printf("Client %d Disconnected, Reset Total\n", client_no);
			}
			pthread_exit(0); 
		}
		else{
			return_val = addNewNumber(num, this_sock, client_no);
			if(return_val == 666){
				printf("How...\n");
			}
		}
	}
	close(this_sock);
	pthread_exit(0);
}

