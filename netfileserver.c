#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <linux/limits.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>
#include <math.h>
#include <errno.h>

#define PORT 8080
const int mports[10] = {8001, 8002, 8003, 8004, 8005, 8006, 8007, 8008, 8009, 8010};
//thread info
pthread_t *tid;
pthread_mutex_t lock,mlock;

struct node{
	struct node* next;
	int tag;
	int data;
	int size;
	time_t entertime;
	int connection;
};
struct queue{
	struct node* head;
	struct node* last;
	char* file;
	int mode[3];
	int write;
};
struct queue** files;
int count = 1;

void* queuemanagment(void * q){
    struct queue* queue = (struct queue*) q;
	struct node* current;
	int fd, result;
	int mplex;
	char* buff;
	do{
		while(queue->head == NULL);
		current = queue->head;
		queue->head = queue->head->next;
		switch(current->tag){
			//open
            case 1: ;
					int flags = current->data;
					//opening in unrestricted
					if(queue->mode[2] != 0){
						current->next = queue->last;
						queue->last = current;
                        break;
					}
					else if(flags == O_RDONLY){
				    	fd = (open(queue->file, flags));
				    	if(fd != -1){
				    		fd = htonl(-fd);
							send(current->connection, &fd, sizeof(int), 0);
                        }
						else{
							fd = htonl(fd);
							send(current->connection, &fd, sizeof(int), 0);
							send(current->connection, &errno, sizeof(int), 0);
						}
                        queue->mode[current->size]++;
						free(current);
                        break;
					}
                    else{
                        if(queue->write == 0){
						    queue->write++;

					    	fd = (open(queue->file, flags));
					    	if(fd != -1){
					    		fd = htonl(-fd);
								send(current->connection, &fd, sizeof(int), 0);
                            }
							else{
								fd = htonl(fd);
								send(current->connection, &fd, sizeof(int), 0);
								send(current->connection, &errno, sizeof(int), 0);
							}
							queue->mode[current->size]++;
							free(current);
                        }
                        else if(queue->mode[1] != 0){
                            current->next = queue->last;
						    queue->last = current;
                            break;
                        }
                        else{
					    	fd = (open(queue->file, flags));
					    	if(fd != -1){
					    		fd = htonl(-fd);
								send(current->connection, &fd, sizeof(int), 0);
                            }
							else{
								fd = htonl(fd);
								send(current->connection, &fd, sizeof(int), 0);
								send(current->connection, &errno, sizeof(int), 0);
							}
							queue->write++;
                            queue->mode[current->size]++;
							free(current);
                        }
					}
				break;
				
			//close
			case 2: ;
				result = 0;			
				result = close(current->data);
				if(result == -1){
					send(current->connection, &result, sizeof(int), 0);
					send(current->connection, &errno, sizeof(errno), 0);
				}
				else{
					send(current->connection, &result, sizeof(int), 0);
					pthread_mutex_lock(&lock);
					queue->mode[current->size]--;

					if(fcntl(current->data, F_GETFL) != O_RDONLY){
						queue->write--;
					}
					pthread_mutex_unlock(&lock);
				}
				free(current);
				break;
			//read
			case 3: ;
				buff = malloc(current->size);

				result = 0;
				result = read(current->data,buff,current->size);
				send(current->connection, &result, sizeof(int), 0);
				if(result == -1){
					send(current->connection, &errno, sizeof(errno), 0);
					break;
				}
				mplex = (int)ceil(((double)result)/4096)-1;
				if(mplex>0){
					int i = 0;
					int j = 0;
					struct sockaddr_in mplexaddr;
					bzero((char *) &mplexaddr, sizeof(mplexaddr));
					mplexaddr.sin_family = AF_INET;
					mplexaddr.sin_addr.s_addr = INADDR_ANY;
					int mplexsock[mplex];

					pthread_mutex_lock(&mlock);
					while(j<mplex && i<10){
						mplexaddr.sin_port = htons(mports[i]);
						mplexsock[j] = socket(AF_INET, SOCK_STREAM, 0);
						if(bind(mplexsock[j],(struct sockaddr*)&mplexaddr,sizeof(mplexaddr))<0){
							if(errno == EADDRINUSE){
								i++;
								close(mplexsock[j]); 
							}
						}
						else{
							send(current->connection, &mports[i], sizeof(int), 0);
							listen(mplexsock[j],1);
							mplexsock[j] = accept(mplexsock[j],NULL,NULL);
							j++;
							i++;
						}
					}
					pthread_mutex_unlock(&mlock);
					send(current->connection, 0, sizeof(int), 0);
					send(current->connection, buff, 4096, 0);
					for(i = 0; i<j;i++){
						send(mplexsock[i], &buff[4096*(i+1)],4096,0);
					}
					for(i = 0; i<j;i++){
						close(mplexsock[i]);
					}
					free(mplexsock);
				}
				else{
					send(current->connection, 0, sizeof(int),0);
					send(current->connection, buff, result, 0);
				}
				free(current);
				free(buff);
				break;
			///write	
			case 4: ;
				mplex = ceil((double)(current->size)/4096)-1;
				buff = malloc(current->size);
				int result;
				if(mplex >0){
					int i = 0;
					int j = 0;
					struct sockaddr_in mplexaddr;
					bzero((char *) &mplexaddr, sizeof(mplexaddr));
					mplexaddr.sin_family = AF_INET;
					mplexaddr.sin_addr.s_addr = INADDR_ANY;
					int mplexsock[mplex];

					pthread_mutex_lock(&mlock);
					while(j<mplex && i<10){
						mplexaddr.sin_port = htons(mports[i]);
						mplexsock[j] = socket(AF_INET, SOCK_STREAM, 0);
						if(bind(mplexsock[j],(struct sockaddr*)&mplexaddr,sizeof(mplexaddr))<0){
							if(errno == EADDRINUSE){
								i++;
								close(mplexsock[j]); 
							}
						}
						else{
							send(current->connection, &mports[i], sizeof(int), 0);
							listen(mplexsock[j],1);
							mplexsock[j] = accept(mplexsock[j],NULL,NULL);
							j++;
							i++;
						}
					}
					pthread_mutex_unlock(&mlock);
					send(current->connection, 0, sizeof(int), 0);
					recv(current->connection, buff, 4096, 0);
					for(i = 0; i<j;i++){
						recv(mplexsock[i], &buff[4096*(i+1)],4096,0);
					}
					for(i = 0; i<j;i++){
						close(mplexsock[i]);
					}
					free(mplexsock);
				}
				else{
					send(current->connection, 0, sizeof(int), 0);
					recv(current->connection, buff, current->size, 0);
				}
					result = write(current->data, buff, current->size);
					if(result != -1)

						send(current->connection, &result, sizeof(int), 0);
					else{
						send(current->connection, &result, sizeof(int), 0);
						send(current->connection, &errno, sizeof(errno), 0);
					}
				free(current);
				free(buff);
				break;
			
		}
		
	}while(queue->mode[0] != 0 || queue->mode[1] != 0 || queue->mode[2] != 0);
	
}

void* clientmanagement(void* conn){
	//connection vars
    printf("new thread");
	int connection = *((int*)conn);
	int filemode, tag;
	int openfile;
	char* path ;
	int flag;
	void *result;
	if(recv(connection, &filemode, sizeof(int),0)==-1){
		perror("failed to read");
		exit(1);
	}
	printf("filemode is %d",filemode);
	read(connection, &tag, sizeof(int));
	int i =0;
	int j;
	//tag -1 = close connection
	while(tag != -1){
		if(tag == 1){		
			//open
			
			if(recv(connection, path, PATH_MAX,0)==-1){
				perror("failed to read");
				exit(1);
			}
            //echo
            send(connection, path, strlen(path)+1,0);
			if(recv(connection, &flag, sizeof(int),0)==-1){
   				perror("failed to read");
				exit(1);
			}
			//send(connection, &flag,sizeof(int),0);
			//check each open queue to see if the file has a queue
			pthread_mutex_lock(&lock);
			for(i = 0; i < sizeof(files)/sizeof(struct queue*);i++){
                if(files[i] != NULL){
					if(strcmp(files[i]->file,path)){

						struct node* new = malloc(sizeof(struct node));
						//add the open cmd to the queue
						new->tag = 1;
						new->data = flag;
						new->size = filemode;
						new->entertime = clock();
						new->connection = connection;
						if(files[i]->head){
							struct node* tmp = files[i]->last;
							files[i]->last = new;
							tmp->next = new;
						}
						else{
							files[i]->last = new;							
							files[i]->head = new;
						}
						break;
					}
				}
			}
			//the file does not have a queue
			if(i == sizeof(files)/sizeof(struct queue*)){
				files = realloc(files, sizeof(files)/sizeof(struct queue*)+1);
				i;
				files[i] = malloc(sizeof(struct queue));
				files[i]->file = path;
				//put open on queue
				struct node* new = malloc(sizeof(struct node));
				new->tag = 1;
				new->data = flag;
				new->size = filemode;
				new->connection = connection;
				new->entertime = clock();
				files[i]->last = new;
				files[i]->head = new;
				//create thread for the queue
				for(j = 0; j < count;j++){
					if(tid[j] == 0){
						pthread_create(&tid[j], NULL, queuemanagment, files[i]);
						break;
					}
				}
				if(j == count -1){
					count++;
					tid = realloc(tid, count);
				}
				
			}
			pthread_mutex_unlock(&lock);
            //send(connection, *((int**)result), sizeof(int), 0);
		}
		else if(tag == 2){
			//close
			int fd;
			recv(connection, &fd, sizeof(int), 0);
			struct node* new = malloc(sizeof(struct node));
			new->data = -htonl(fd);
			new->tag = 2;
			new->connection = connection;
			new->size = filemode;
			new->entertime = clock();
			pthread_mutex_lock(&lock);
					if(files[i]->head){
						struct node* tmp = files[i]->last;
						files[i]->last = new;
						tmp->next = new;
					}
					else{
						files[i]->last = new;
						files[i]->head = new;
					}
			pthread_mutex_unlock(&lock);
			
		}
		else if(tag == 3){
			//read
			int size,fd;
			recv(connection, &fd, sizeof(int), 0);
			recv(connection, &size, sizeof(int), 0);
			struct node* new = malloc(sizeof(struct node));
			new->size = size;
			new->tag = 3;
			new->data = -ntohl(fd);
			new->entertime = clock();
			pthread_mutex_lock(&lock);			
			if(files[i]->head){
				struct node* tmp = files[i]->last;
				files[i]->last = new;
				tmp->next = new;
			}
			else{
				files[i]->last = new;
				files[i]->head = new;
			}
			pthread_mutex_unlock(&lock);
			
		}
		else if(tag == 4){
			//write
			struct node* new = malloc(sizeof(struct node));
			new->tag = 4;
			int fd;
			recv(connection, &fd, sizeof(int), 0);
			recv(connection, &(new->size), sizeof(int), 0);
			new->data = -htonl(fd);
			new->connection = connection;
			new->entertime = clock();
			pthread_mutex_lock(&lock);
			if(files[i]->head){
				struct node* tmp = files[i]->last;
				files[i]->last = new;
				tmp->next = new;
			}
			else{
				files[i]->last = new;
				files[i]->head = new;
			}
			pthread_mutex_unlock(&lock);
		}
		recv(connection, &tag, sizeof(int),0);
	}
	close(connection);
	return 0;
}



int main(void){
	//socket info
	int listensocket;
	int *connection = malloc(sizeof(int));
	struct sockaddr_in address, client;
	int opt = 1;
	int addrlen = sizeof(address);
	int clientlen = sizeof(client);
	pthread_mutex_init(&lock,NULL);
	pthread_mutex_init(&mlock,NULL);
	tid = malloc(sizeof(pthread_t));
    files = malloc(sizeof(struct queue*));

	int ccount = 1;
	//create the socket to list for connections
	if((listensocket = socket(AF_INET,SOCK_STREAM,0)) == 0){
		perror("listen socket failed");
		return 1;
	}
	printf("socket created\n");
	// set socket options
	if(setsockopt(listensocket,SOL_SOCKET,SO_REUSEADDR|SO_REUSEPORT,&opt,sizeof(opt))){
		perror("set socket option failed");
		return 1;
	}
	address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
	// bind socket to port 8080
	if (bind(listensocket, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
	//set socket to listen
	listen(listensocket,5);
	printf("socket listening\n");
	int i = 0;
	int j = 0;
	while(1){
		//set connection to the next socket on listen Q
		for(j = 0; j < sizeof(connection);j++){
            if(connection[j] == 0){
			    connection[j] = accept(listensocket, (struct sockaddr *)&client, &clientlen);
                printf("found connection\n");
                break;
            }
		}
		if(j == ccount -1){
			ccount++;
			connection = realloc(connection, ccount);
		}
		if(connection < 0){
			perror("accept failed\n");
			return 1;
		}
		
		//spawn thread for the connection
		pthread_mutex_lock(&lock);
		for(i = 0; i < count;i++){
			if(tid[i] == 0){
				printf("creating thread\n");
				pthread_create(&tid[i], NULL, &clientmanagement, &connection[j]);
				//pthread_detatch(&tid[i]);
                printf("created thread");
				break;
			}
		}
		if(i == count -1){
			count++;
			tid = realloc(tid, count);
		}
		pthread_mutex_unlock(&lock);
	}
}
