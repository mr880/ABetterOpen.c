#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

int server_socket;

int netopen(const char * pathname, int flags){

	int response;
	int tag = 1;
  char * retpath = malloc(sizeof(pathname));

	send(server_socket, &tag, sizeof(tag),0);
	send(server_socket, pathname, strlen(pathname)+1, 0);
    //recv echo
  recv(server_socket, retpath, strlen(pathname)+1,0);
  flags = htonl(flags);
 	send(server_socket, &flags, sizeof(int), 0);

	recv(server_socket, &response, sizeof(response), 0);
  response = ntohl(response);

	if(response == -1){
			//server will send an errno+1
			recv(server_socket, &response, sizeof(response), 0);
			errno = response;
			perror("Error");
			return -1;
	}
	
	//response = ntohl(response);

	return response;


}

ssize_t netread(int fd, char* buff, size_t nbytes){

		int response;
		int tag = 3;
		int mul_response;
		fd = htonl(fd);
		struct sockaddr_in serv_addr;

		bzero((char *) &serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
        int len = sizeof(serv_addr);
		getpeername(server_socket, (struct sockaddr*)&serv_addr, &len);

		send(server_socket, &tag, sizeof(tag),0);
    send(server_socket, &fd, sizeof(fd), 0);
		send(server_socket, &nbytes, sizeof(nbytes), 0);
	  recv(server_socket, &response, sizeof(response), 0);

		if(response == -1){
				//server will send an errno+1
				recv(server_socket, &response, sizeof(response), 0);
				errno = response;
				perror("Error");
				return -1;
		}

		recv(server_socket, &mul_response, sizeof(mul_response), 0);

		//create array of sockets
		int socket_arr[(response/4096)];
		int i = 0;

		while(mul_response != 0){
				serv_addr.sin_port = htons(mul_response);
				socket_arr[i] = socket(AF_INET, SOCK_STREAM, 0);
				connect(socket_arr[i], (struct sockaddr*)&serv_addr, sizeof(serv_addr));
				i++;

				recv(server_socket, &mul_response, sizeof(mul_response), 0);

		}
		recv(server_socket, buff, 4096, 0 );

		while(i > 0){
				i--;
				int x = socket_arr[i];
				recv(x, &buff[4096*(i+1)], 4096, 0);
				close(x);
		}

}

ssize_t netwrite(int fd, char *buff, size_t nbytes){

		int response;
		int tag = 4;
		int mul_response;
		fd = htonl(fd);
		struct sockaddr_in serv_addr;

		bzero((char *) &serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
        int len = sizeof(serv_addr);
		getpeername(server_socket, (struct sockaddr*)&serv_addr, &len);

		send(server_socket, &tag, sizeof(tag),0);
        send(server_socket, &fd, sizeof(fd), 0);
		send(server_socket, &nbytes, sizeof(nbytes), 0);

		recv(server_socket, &mul_response, sizeof(mul_response), 0);

		//create array of sockets
		int socket_arr[(response/4096)];
		int i = 0;

		while(mul_response != 0){
				serv_addr.sin_port = htons(mul_response);
				socket_arr[i] = socket(AF_INET, SOCK_STREAM, 0);
				connect(socket_arr[i], (struct sockaddr*)&serv_addr, sizeof(serv_addr));
				i++;

				recv(server_socket, &mul_response, sizeof(mul_response), 0);

		}
		send(server_socket, buff, 4096, 0 );

		while(i > 0){
				i--;
				int x = socket_arr[i];
				send(x, &buff[4096*(i+1)], 4096, 0);

		}

		recv(server_socket, &response, sizeof(response), 0);

		if(response == -1){
				//server will send an errno+1
				recv(server_socket, &response, sizeof(response), 0);
				errno = response;
				perror("Error");
				return -1;
		}

		while(i < sizeof(socket_arr)){
				close(socket_arr[i]);
				i++;
		}




}

int netclose(int filedesc){

    int response;
		int tag = 2;
		filedesc = htonl(filedesc);
		send(server_socket, &tag, sizeof(tag),0);
    	send(server_socket, &filedesc, sizeof(filedesc), 0);
    	recv(server_socket, &response, sizeof(response), 0);

    if(response == -1){
	      recv(server_socket, &response, sizeof(response), 0);
	      errno = response;
	      perror("Error");
	      return -1;
    }

    return response;


}

int netserverinit(char* hostname, int filemode) {

	   int portno = 8080;
	   struct sockaddr_in serv_addr;
	   struct hostent *server;

	   /* Create a socket point */
	   server_socket = socket(AF_INET, SOCK_STREAM, 0);

	   if (server_socket < 0) {
		      perror("ERROR opening socket");
		      exit(1);
	   }

	   server = gethostbyname(hostname);

	   if (server == NULL) {
		      fprintf(stderr,"ERROR, no such host: %s\n",hostname);
		      exit(0);
	   }

	   bzero((char *) &serv_addr, sizeof(serv_addr));
	   serv_addr.sin_family = AF_INET;
	   bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	   serv_addr.sin_port = htons(portno);

	   /* Now connect to the server */
	   if (connect(server_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		      perror("ERROR connecting");
		      exit(1);
	   }

	   send(server_socket,&filemode,sizeof(filemode),0);
	   /* Now ask for a message from the user, this message
	      * will be read by server
	   */
	   return 0;
}

int prompt_user(){
		char* comm = malloc(sizeof(char)*2);
		printf("Enter command: \n 0 : netopen()\n 1 : netclose() \n 2 : netread() \n 3: netwrite() \n 4: exit\n");
		fgets(comm, sizeof(int), stdin);
		char buffer[256];
		int flag;
		char* buff;
		char flags[3];
		int fd, x;
		char bytes[4];
		char input[5000];
        ssize_t bytes_read;
		switch (comm[0])
		{
		         case '0':
						printf("-----netopen-----\n");
						printf("Enter Pathname:");

						bzero(buffer,256);
						fgets(buffer,255,stdin);
						char* pos;

						if((pos = strchr(buffer,'\n'))!= NULL){
								*pos = '\0';
						}
						printf("Enter Flag: ");
						fgets(flags,sizeof(int),stdin);

						if((pos = strchr(flags,'\n'))!= NULL){
								*pos = '\0';
						}

						flag = atoi(flags);

						fd = netopen(buffer, flag);

						printf("File Descriptor: %d\n", fd);

						return 0;

		        case '1':
						printf("-----netclose-----\n");

						int close = netclose(fd);

						return 1;

				case '2':
						printf("-----netread------\n");


						printf("Enter Size (in bytes): ");
						fgets(bytes,sizeof(int),stdin);
						char* pos2;
						if((pos2 = strchr(bytes,'\n'))!= NULL){
								*pos2 = '\0';
						}

						x = atoi(bytes);
						buff = malloc(x);
						bytes_read = netread(fd, buff, x);
						printf("file contained: %s",buff);
						return 2;

				case '3':
						printf("-----netwrite-----\n");
						char* pos3;
						bzero(input, 5000);
						printf("Input: ");
						fgets(input,sizeof(int),stdin);

						if((pos3 = strchr(input,'\n'))!= NULL){
								*pos3 = '\0';
						}

						x = strlen(input);

						bytes_read = netwrite(fd, input, x);

						return 3;

				case '4':
						printf("Exit - Goodbye\n");
						return -1;

		    default:
						printf("Enter a digit 0-4");
						return 4;
		}

}



int main(int argc, char** argv){

	char buffer[256];
	int filemode;
	char fm[3];

	printf("Please enter the hostname: ");
	bzero(buffer,256);
	fgets(buffer,255,stdin);
	char* pos;

	if((pos = strchr(buffer,'\n'))!= NULL){
			*pos = '\0';
	}

	int command = 0;



	printf("Please enter the filemode: ");
	fgets(fm,sizeof(int),stdin);

	if((pos = strchr(fm,'\n'))!= NULL){
			*pos = '\0';
	}

	filemode = atoi(fm);
	netserverinit(buffer,filemode);


	while(command != -1){
			command = prompt_user();
	}
	//printf("netopen\n");

	// int fd = netopen("test.txt",3);
  int end = -1;
  send(server_socket,&end,sizeof(end),0);
	// printf("%d\n",fd);

}
