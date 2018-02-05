/**
 * @shaoming_assignment1
 * @author  shaoming Xu <shaoming@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "../include/global.h"
#include "../include/logger.h"

#define TRUE 1
#define MSG_SIZE 256
#define BUFFER_SIZE 256
#define BACKLOG 5
#define STDIN 0
#define CMD_SIZE 100
#define MY_UBIT_NAME "shaoming"
#define UDPSERVERPORT "53"
#define UDPSERVERIP "8.8.8.8"
#define HOSTNAME_SIZE 1024

struct clientinfo
{
	char 				ci_hostname[HOSTNAME_SIZE];
	struct in_addr 		ci_addr;		//Internet address
	unsigned int 		ci_port;
	int 				ci_num_msg_sent;
	int      			ci_num_msg_rcv;
	bool 				ci_status;

	struct clientinfo 	*ci_next;
};

struct serverblocklist
{
	struct in_addr 		bs_src;
	char 				bs_des_hostname[HOSTNAME_SIZE];
	struct in_addr 		bs_des;
	unsigned int 		bs_des_port;

	struct 	serverblocklist *bs_next;
};

struct blockofclient
{
	struct in_addr 		bc_src;
	struct in_addr 		bc_des;

	struct blockofclient *bc_next;
};




int connect_to_host(char *server_ip, int server_port);
int Find_local_ip(struct sockaddr_in *clientaddr);
void Client_insert(struct clientinfo *L_head, struct clientinfo *elem);
int Clients_print(struct clientinfo *L_head, int opt);  //opt:0 list in common style.  opt:1 list of statistics
struct clientinfo Client_initiate(struct sockaddr_in client_addr);
int Client_location(struct clientinfo *L_head, struct sockaddr_in client_addr);
int Client_delete(struct clientinfo *L_head, int loc);
int Client_logout(struct clientinfo *L_head, char ip[]); //-1:fail 0:success
int Client_exit(struct clientinfo *L_head, char ip[]);	//-1:fail 0:success
int Clients_clear(struct clientinfo *L_head);
bool isValidIpAddress(char *ipAddress);
unsigned int s2i(char *str);
unsigned int string2uint(char *st);
int Client_list2string(struct clientinfo *L_head, char buf[]);
int Client_set_recv(struct clientinfo *L_head, char ip[]);
int Client_set_send(struct clientinfo *L_head, char ip[]);
int Client_set_recv_b(struct clientinfo *L_head, char ip[]);
int Client_login(struct clientinfo *L_head, char ip[]);
int Client_location_stringip(struct clientinfo *L_head, char ip[]);

//for client block list
void Cblock_insert(struct blockofclient *B_head, struct blockofclient *elem);
int Cblock_location(struct blockofclient *B_head, char des[]);
int Cblock_delete(struct blockofclient *B_head, char des[]);

//for server block
int Sblock_initiate(struct clientinfo *L_head, char ip[], struct serverblocklist *b_customer);
void Sblock_insert(struct serverblocklist *Bs_head, struct serverblocklist *elem);
void Sblock_print(struct serverblocklist *Bs_head, char ip[]);
int Sblock_delete(struct serverblocklist *Bs_head, char src[], char des[]);
int Sblock_location(struct serverblocklist *Bs_head, char src[], char des[]);
/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
int main(int argc, char **argv)
{
	/*Init. Logger*/
	cse4589_init_log(argv[2]);

	// /*Clear LOGFILE*/
	fclose(fopen(LOGFILE, "w"));

	/*Start Here*/
	 int si_port;
	 si_port = atoi(argv[2]);  // make string to int 

	 struct sockaddr_in clientaddr;  // use to save clients' information host address
	 Find_local_ip(&clientaddr);


///////////////////Server code start//////////////////////////////////
	if(strcmp(argv[1],"s") == 0){	
		//MAKE A EMPTY LINK LIST FOR LATER USE//////////
		struct clientinfo *L_head;
		L_head = (struct clientinfo *) malloc(sizeof(struct clientinfo));
		L_head->ci_next = NULL;	

		//MAKE A EMPTY LINK LIST FOR SERVER BLOCK LIST
		struct serverblocklist *Bs_head;
		Bs_head =(struct serverblocklist *) malloc(sizeof(struct serverblocklist));
		Bs_head->bs_next = NULL;

	    // Declare a few variable
		int server_socket, head_socket, selret, sock_index, fdaccept=0, caddr_len;
		struct sockaddr_in server_addr, client_addr;
	    //file descriptor list
		fd_set master_list, watch_list;

		/* Socket 
	     * AF_INET: Internet Protocol v4 addresses
	     * SOCK_STREAM: TCP protocol
	     * 0 designates the specific protocol
	     * server_socket is the file descriptor (handle) used to access a file or
	     other input/outp resources*/

		server_socket = socket(AF_INET, SOCK_STREAM, 0);   //creat a server socket
	    if(server_socket < 0)
			perror("Cannot create socket");

		/* Fill up sockaddr_in struct 
	     * atoi convert string to integer 
	    */
		bzero(&server_addr, sizeof(server_addr));
	    server_addr.sin_family = AF_INET;
	    /* INADDR_ANY: allowed your program to work without knowing the IP address...
	    */
	    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	    server_addr.sin_port = htons(si_port);  // make int to network readable

	    /* Bind */
	    if(bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0 )  
	    	perror("Bind failed");

	    /* Listen */
	    /*BACKLOG specifies the maximum number of connections*/
	    if(listen(server_socket, BACKLOG) < 0)
	    	perror("Unable to listen on server port");

	    /* ---------------------------------------------------------------------------- */
	    /* FD_ZERO() clears a set.
	     * FD_SET() and FD_CLR() respectively add and remove a given file descriptor from
	     * a set. 
	     * FD_ISSET() tests to see if a file descriptor is part of the set  
	    */

	    /* Zero select FD sets */
	    FD_ZERO(&master_list);
	    FD_ZERO(&watch_list);
	    
	    /* Register the listening socket */
	    FD_SET(server_socket, &master_list);
	    /* Register STDIN */
	    FD_SET(STDIN, &master_list);

	    head_socket = server_socket;

	    while(TRUE){
	        // copy master_list to watch_list
	        memcpy(&watch_list, &master_list, sizeof(master_list));
	        printf("\n[PA1-Server@CSE489/589]$ ");
			fflush(stdout);

	        /* select() system call. This will BLOCK */
	        /* select examinines the status of file descriptors of open input/output channels.
	         * it return the total number of file descriptors that are ready.
	         * nfds [in] Igonred. The nfds parameter is included only for copativility with 
	         * Berkeley sockets.
	         * readfds [in, out] a set of sockets to be checked for readability.
	         * writefds [in, out] a set of sockets to be checked fo writability.
	         * exceptfds [in, out] a set fo sockets to be checked for errors
	         * timeout [in] The maximum time for select to wait.
	        */
	        selret = select(head_socket + 1, &watch_list, NULL, NULL, NULL);
	        if(selret < 0)
	            perror("select failed.");

	        /* Check if we have sockets/STDIN to process */
	        if(selret > 0){
	            /* Loop through socket descriptors to check which ones are ready */
	            for(sock_index=0; sock_index<=head_socket; sock_index+=1){
	                if(FD_ISSET(sock_index, &watch_list)){
	                    /* Check if new command on STDIN */
	                    if (sock_index == STDIN){
	                    	char *cmd = (char*) malloc(sizeof(char)*CMD_SIZE);

	                        //copy '\0' to cmd in the first CMD_SIZE characters
	                    	memset(cmd, '\0', CMD_SIZE);
							if(fgets(cmd, CMD_SIZE-1, stdin) == NULL) {
	                            exit(-1);                          
	                        }	                        
	                        // from https://stackoverflow.com/questions/2693776/removing-trailing-newline-character-from-fgets-input
	                        // delete the 'enter' in the end of msg  
	                        char *pos;
	                        if((pos = strchr(cmd, '\n')) != NULL){
	                            *pos = '\0';
	                        }
							printf("\nI got: %s\n", cmd);							
							// learn from https://stackoverflow.com/questions/4513316/split-string-in-c-every-white-space
							//divide the message bases on space and then store them into cmdv array
							int i;
							char cmdv[3][MSG_SIZE];  // cmdv is used to store User's command and message
							for(i=0; i < 3; i++)	//memset the cmdv array
							{
								memset(cmdv[i],'\0',MSG_SIZE);
							}

							char *pch;
							pch = strtok (cmd," ");	
							i=0;  	//reset i to 0;
							//divide the original message and store it into msgv array
							while (pch != NULL)	
							{		  
							  if(i < 2){
							   	strcpy(cmdv[i], pch);
							  }else{							  	
							  	strcat(cmdv[2], pch);
							  	strcat(cmdv[2], " ");
							  }
							  pch = strtok (NULL, " ");
							  i++;
							}
							//delete the last space of msgv[2]
							cmdv[2][strlen(cmdv[2])-1] = '\0';							
							//Process PA1 commands here ...
	                        if(strcmp(cmdv[0],"AUTHOR") == 0){	                     
	                            cse4589_print_and_log("[%s:SUCCESS]\n", cmdv[0]);	                            
	                            cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", MY_UBIT_NAME);	                            
	                            cse4589_print_and_log("[%s:END]\n", cmdv[0]);	                            
	                        }
	                        else if(strcmp(cmdv[0],"IP") == 0){ //Print the external IP address of this process
								if(inet_ntoa(clientaddr.sin_addr) != NULL){	//How to know it success
									cse4589_print_and_log("[%s:SUCCESS]\n", cmdv[0]);
									cse4589_print_and_log("IP:%s\n", inet_ntoa(clientaddr.sin_addr));
									cse4589_print_and_log("[%s:END]\n", cmdv[0]);							
								}else{
									cse4589_print_and_log("[%s:ERROR]\n", cmdv[0]);
									cse4589_print_and_log("[%s:END]\n", cmdv[0]);									
								}
							}else if(strcmp(cmdv[0],"PORT") == 0){
								cse4589_print_and_log("[%s:SUCCESS]\n", cmdv[0]);				
								cse4589_print_and_log("PORT:%d\n", si_port);
								cse4589_print_and_log("[%s:END]\n", cmdv[0]);								
							}
							else if(strcmp(cmdv[0],"LIST") == 0){	
								//display current login in client, not include server
								//sorted by their listening port numbers, in increasing order							
							 	cse4589_print_and_log("[%s:SUCCESS]\n", cmdv[0]);
								Clients_print(L_head, 0);																							
								cse4589_print_and_log("[%s:END]\n", cmdv[0]);
							}else if(strcmp(cmdv[0],"STATISTICS") == 0){
	                            cse4589_print_and_log("[%s:SUCCESS]\n", cmdv[0]);
	                            Clients_print(L_head, 1);  
	                            //opt:0 list in common style.  opt:1 list of statistics
	                            // list of all the clients that have ever logged-in to the server
	                            // sorted by their listening port numbers, in increasing order
	                            cse4589_print_and_log("[%s:END]\n", cmdv[0]);
	                        }
	                        else if(strcmp(cmdv[0],"BLOCKED") == 0){ //cmdv[1] is the IP address of client
	 
	                            if(!isValidIpAddress(cmdv[1])){
	                                cse4589_print_and_log("[%s:ERROR]\n", cmdv[0]);
	                                cse4589_print_and_log("[%s:END]\n", cmdv[0]);
	                            }
	                            else if(Client_location_stringip(L_head, cmdv[1]) == -1){
	                            	// valid IP but non-existen IP
	                                cse4589_print_and_log("[%s:ERROR]\n", cmdv[0]);
	                                cse4589_print_and_log("[%s:END]\n", cmdv[0]);
	                            }
	                            else{
	                            	cse4589_print_and_log("[%s:SUCCESS]\n", cmdv[0]);
	                            	Sblock_print(Bs_head, cmdv[1]);  
	                            	cse4589_print_and_log("[%s:END]\n", cmdv[0]);
	                            }
	                        } // END BLOCKED
	                        //else{
	                        //     cse4589_print_and_log("[%s:ERROR]\n", msgv[0]);
	                        //     cse4589_print_and_log("[%s:END]\n", msgv[0]);
	                        //     // print input error. we may need to use 5.3 shell command output
	                        // }

							free(cmd);
	                    }
	                    /* Check if new client is requesting connection */
	                    else if(sock_index == server_socket){
	                        caddr_len = sizeof(client_addr);
	                        fdaccept = accept(server_socket, (struct sockaddr *)&client_addr, &caddr_len);
	                        if(fdaccept < 0)
	                            perror("Accept failed.");                      
	                        /* Add to watched socket list */
	                        FD_SET(fdaccept, &master_list);
	                        if(fdaccept > head_socket) head_socket = fdaccept;
	                    }
	                    /* Read from existing clients */
	                    else{
	                        //[EVENT]: Message Received
	                        /* Initialize buffer to receieve response */
	                        char *buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);
	                        memset(buffer, '\0', BUFFER_SIZE);
	                        //the sock_index is that return from accept()
	                        if(recv(sock_index, buffer, BUFFER_SIZE, 0) <= 0){  
	                            close(sock_index);
	                            printf("Remote Host terminated connection!\n");
	                            /* Remove from watched list */
	                            FD_CLR(sock_index, &master_list);
	                        }
	                        else {
	                        	//Process incoming data from existing clients here ...	                        	
	                        	struct sockaddr_in clientaddr_recv , clientaddr_send;  // get server's ip
	                        	int clientaddr_recv_len, clientaddr_send_len;	                 
	                        	clientaddr_recv_len = sizeof(struct sockaddr_in);
	                        	clientaddr_send_len = sizeof(struct sockaddr_in);
	                        	getpeername(sock_index, (struct sockaddr*)&clientaddr_recv, &clientaddr_recv_len);
	                        	printf("clientaddr_recv ip:%s\n",inet_ntoa(clientaddr_recv.sin_addr) );  
	                        		int i;
									char bufv[3][MSG_SIZE];  // cmdv is used to store User's command and message
									for(i=0; i < 3; i++)	//memset the cmdv array
									{
										memset(bufv[i],'\0',MSG_SIZE);
									}

									char *pch;
									pch = strtok (buffer," ");	
									i=0;  						//reset i to 0;
									//divide the original message and store it into msgv array
									while (pch != NULL)			
									{		  
									  if(i < 2){
									   	strcpy(bufv[i], pch);
									  }else{							  	
									  	strcat(bufv[2], pch);
									  	strcat(bufv[2], " ");
									  }
									  pch = strtok (NULL, " ");
									  i++;
									}
									bufv[2][strlen(bufv[2])-1] = '\0'; //buf[0], bufv[1], bufv[2]
 									for(i=0; i<=2; i++){
										printf("%s\n", bufv[i]);
									}                       	                 	

								char linkbuffer[MSG_SIZE];
								memset(linkbuffer, '\0', MSG_SIZE);
	                        	if( strcmp(bufv[0],"LOGOUT")==0 ){
	                        		Client_logout(L_head, bufv[1]); 
	                        		printf("LOGOUT\n");
	                        	}else if(strcmp(bufv[0], "EXIT")==0){
	                        		Client_exit(L_head, bufv[1]);
	                        		printf("EXIT\n");
	                        	}else if(strcmp(bufv[0], "REFRESH") == 0){
	                        		strcat(linkbuffer,bufv[0]);
	                        		strcat(linkbuffer," ");
	                        		strcat(linkbuffer,bufv[0]);
	                        		Client_list2string(L_head, linkbuffer);
	                        		printf("logined clients information:%s\n", linkbuffer);
	                        		printf("Sending it to the remote host ... ");
									if(send(sock_index, linkbuffer, strlen(linkbuffer), 0) == strlen(linkbuffer))
										printf("Done!\n");
									fflush(stdout);
	                        	}else if(strcmp(bufv[0], "BROADCAST") == 0 ){
	                        		cse4589_print_and_log("[%s:SUCCESS]\n", "RELAYED");
	                        		char *str_b_1;
	                        		str_b_1 = inet_ntoa(clientaddr_recv.sin_addr);
	                        		char *str_b_2 = (char*) malloc(sizeof(char)* strlen(str_b_1));
	                        		memcpy(str_b_2, str_b_1, strlen(str_b_1));	                        		
	                        		strcat(bufv[0], " ");
	                        		strcat(bufv[0], inet_ntoa(clientaddr_recv.sin_addr));
	                        		strcat(bufv[0], " ");
	                        		strcat(bufv[0], bufv[1]);
	                        		strcat(bufv[0], " ");
	                        		strcat(bufv[0], bufv[2]);
	                        		strcat(bufv[1], " ");
	                        		strcat(bufv[1], bufv[2]);
	                        		cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", str_b_2, "255.255.255.255", bufv[1]);	
	                        		Client_set_send(L_head, str_b_2);
	                        		Client_set_recv_b(L_head, str_b_2);

	                        		for(i = 0; i <= head_socket; i++) {
			                            // send to everyone!
			                            if (FD_ISSET(i, &master_list)) {
			                                // except the listener and ourselves
			                                if (i != server_socket && i != STDIN && i != sock_index) {
			                                    if (send(i, bufv[0], strlen(bufv[0]), 0) == -1) {
			                                        perror("send");
			                                    }
			                                }
			                            }
			                        } // END FOR
			                        cse4589_print_and_log("[%s:END]\n", "RELAYED");
	                        	} //END BROADCAST

	                        	else if(strcmp(bufv[0], "SEND") == 0){ //bufv[0] SEND  bufv[1] desIp bufv[2] data
	                        		cse4589_print_and_log("[%s:SUCCESS]\n", "RELAYED");
	                        		char *str_s_1;
	                        		str_s_1 = inet_ntoa(clientaddr_recv.sin_addr);
	                        		char *str_s_2 = (char*) malloc(sizeof(char)* strlen(str_s_1));
	                        		memcpy(str_s_2, str_s_1, strlen(str_s_1));
	                        		strcat(bufv[0], " ");
	                        		strcat(bufv[0], str_s_2);
	                        		strcat(bufv[0], " ");
	                        		strcat(bufv[0], bufv[2]);
	                        		cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", str_s_2, bufv[1], bufv[2]);	
	                        		Client_set_send(L_head, str_s_2);

	                        		if(Sblock_location(Bs_head, bufv[1], str_s_2) == -1){
	                        			Client_set_recv(L_head, bufv[1]);

		                        		for(i = 0; i <= head_socket; i++) {
				                            // send to everyone!
				                            if (FD_ISSET(i, &master_list)) { 
				                                // except the listener and ourselves
				                                if (i != server_socket && i != STDIN && i != sock_index) {
				                                	getpeername(i, (struct sockaddr*)&clientaddr_send, &clientaddr_send_len);
				                                	if( strcmp(bufv[1], inet_ntoa(clientaddr_send.sin_addr)) ==0 ){
				                                		if (send(i, bufv[0], strlen(bufv[0]), 0) == -1) {
				                                        	perror("send");
				                                		}			                                    
				                                    }
				                                } //END IF
				                            }// END IF
				                        } // END FOR
	                        		}
		                        		
			                        cse4589_print_and_log("[%s:END]\n", "RELAYED");
	                        	} // END ELSE IF SEND

	                        	else if(strcmp(bufv[0], "LOGIN") == 0){
	                        		struct clientinfo *customer_l;	
	                        		struct sockaddr_in src_client;
	                        		char service_l[20];
	                        		
			                        int loc = -1;
			                        inet_aton(bufv[1], &(src_client.sin_addr));
			                        loc = Client_location(L_head, src_client);// find client location in list
			                        printf("loc: %d\n", loc);
			                        if(loc !=-1){   //already in list
			                        	Client_login(L_head, bufv[1]);
			                        }else{			  			                        	                      	
				                        customer_l =  (struct clientinfo *) malloc(sizeof(struct clientinfo));

				                        getnameinfo(&client_addr, sizeof client_addr, customer_l->ci_hostname,
									                        	sizeof customer_l->ci_hostname, service_l, sizeof service_l, 0 );
				                        customer_l->ci_addr=src_client.sin_addr;
				                        customer_l->ci_port =  s2i(bufv[2]);
				                        customer_l->ci_num_msg_sent = 0;
				                        customer_l->ci_num_msg_rcv = 0;
				                        customer_l->ci_status = true;
			                        	Client_insert(L_head, customer_l);
			                        }

	                        	} //END ELSE IF LOGIN
	                        	else if(strcmp(bufv[0], "BLOCK") == 0){
	                        		struct serverblocklist *b_customer;
	                        		b_customer =(struct serverblocklist *) malloc(sizeof(struct serverblocklist));
	                        		inet_aton(bufv[1], &(b_customer->bs_src));
	                        		Sblock_initiate(L_head, bufv[2], b_customer);
	                        		Sblock_insert(Bs_head, b_customer);
	                        	}else if(strcmp(bufv[0], "UNBLOCK") == 0){
	                        		Sblock_delete(Bs_head, bufv[1], bufv[2]);
	                        	}

	        //                 	if(strlen(linkbuffer)!=0){
	        //                 		printf("\nClients information:%s\n", linkbuffer);
									// printf("Sending it to the remote host ... ");
									// if(send(fdaccept, linkbuffer, strlen(linkbuffer), 0) == strlen(linkbuffer))
									// 	printf("Done!\n");
									// fflush(stdout);
	        //                 	}else{
	        //                 		printf("\nClient sent me:%s\n", buffer);
									// printf("ECHOing it back to the remote host ... ");
									// if(send(fdaccept, buffer, strlen(buffer), 0) == strlen(buffer))
									// 	printf("Done!\n");
									// fflush(stdout);
	        //                 	}	                        	
	                        }

	                        free(buffer);
	                    }
	                }
	            }
	        }
	    }

	    return 0;
	}
/////////////////////////////////server code stop/////////////

//////////////////client code start here//////////////////////
/////////////////////////////////////////////////////////////
	if(strcmp(argv[1],"c") == 0){	
//////////////////////initiate client linklists////////////////////
		struct clientinfo *customer, *L_head;
		char hostname[HOSTNAME_SIZE];
		gethostname(hostname, HOSTNAME_SIZE);
		int login_success = 0;

		L_head = (struct clientinfo *) malloc(sizeof(struct clientinfo));
		customer =  (struct clientinfo *) malloc(sizeof(struct clientinfo));
		L_head->ci_next = NULL;

		strcpy(customer->ci_hostname, hostname);
		customer->ci_addr = clientaddr.sin_addr;
		customer->ci_port = clientaddr.sin_port;
		customer->ci_num_msg_sent = 0;
		customer->ci_num_msg_rcv = 0;
		customer->ci_status = false;

		// initiate link list
		customer->ci_next = L_head->ci_next;
		L_head->ci_next = customer;

/////////////MAKE A EMPTY LINK LIST FOR client block list//////////
		struct blockofclient *B_head;
		B_head = (struct blockofclient *) malloc(sizeof(struct blockofclient));
		B_head->bc_next = NULL;	

//////////////////Declare a few variable///////
		// server use to send message to server. server_socket use to listen mssage from other clients
		int server, len, server_socket, head_socket, selret, sock_index, fdaccept=0, caddr_len;
		struct sockaddr_in remote_server_addr,server_addr, client_addr;
		//file descriptor list
		fd_set master_list, watch_list;
		////USE TO STORE LOGIN SOCKET INFORMATION////
		int clientin_len;
		clientin_len = sizeof(struct sockaddr_in);
		struct sockaddr_in clientin;

////////////////////begin select//////////////
       	/* Zero select FD sets */
	    FD_ZERO(&master_list);
	    FD_ZERO(&watch_list);
	    

	   	/* Register the server  socket */
	    //FD_SET(server_socket, &master_list);
	    /* Register STDIN */
	    FD_SET(STDIN, &master_list);
		
	    head_socket = STDIN;
//////////////////////////begin while loop/////
	    while(TRUE)
	    {
	    	memcpy(&watch_list, &master_list, sizeof(master_list));
	        printf("\n[PA1-Client@CSE489/589]$ ");
			fflush(stdout);
			selret = select(head_socket + 1, &watch_list, NULL, NULL, NULL);
	        if(selret < 0)
	            perror("select failed.");
	        if(selret > 0){
	        	for(sock_index=0; sock_index<=head_socket; sock_index+=1){
	        		if(FD_ISSET(sock_index, &watch_list)){
	        			/* Check if new command on STDIN */
	                    if(sock_index == STDIN){
	                    	char *msg = (char*) malloc(sizeof(char)*MSG_SIZE);
					    	memset(msg, '\0', MSG_SIZE);
							if(fgets(msg, MSG_SIZE-1, stdin) == NULL) //Mind the newline character that will be written to msg
								exit(-1);
							// delete the 'enter' in the end of msg  
							// form https://stackoverflow.com/questions/2693776/removing-trailing-newline-character-from-fgets-input
							char *pos;
							if((pos = strchr(msg, '\n')) != NULL){
								*pos = '\0';
							}
							printf("I got: %s(size:%d chars)\n", msg, strlen(msg));
							////////////divide user input msg and store snippets into msgv array///////////////////////////////////
							// learn from https://stackoverflow.com/questions/4513316/split-string-in-c-every-white-space
							int i;
							char msgv[3][MSG_SIZE];  // msgv is used to store User's command and message
							for(i=0; i < 3; i++)	//memset the msgv array
							{
								memset(msgv[i],'\0',MSG_SIZE);
							}
							char *pch;
							pch = strtok (msg," ");	
							i=0;  	//reset i to 0;
							while (pch != NULL)	// this will divide the original message and store it into msgv array
							{		  
							  if(i < 2){
							   	strcpy(msgv[i], pch);
							  }else{			  	
							  	strcat(msgv[2], pch);
							  	strcat(msgv[2], " ");   // FOR THE SUCCESS OF s2i();
							  }
							  pch = strtok (NULL, " ");
							  i++;
							}
							msgv[2][strlen(msgv[2])-1] = '\0'; //delete the last space of msgv[2]
							//printf("msgv[0]:%s\n", msgv[0]);

							for(i=0; i<=2; i++){
								printf("%s\n", msgv[i]);
							}
							// use strcmp() to compare two strings. 
							if(strcmp(msgv[0],"AUTHOR") == 0){
								cse4589_print_and_log("[%s:SUCCESS]\n", msgv[0]);
								cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", MY_UBIT_NAME);
								cse4589_print_and_log("[%s:END]\n", msgv[0]);				
							}
							else if(strcmp(msgv[0],"IP") == 0){ //Print the external IP address of this process
								if(inet_ntoa(clientaddr.sin_addr) != NULL){	//***! to-do  How to know it success
									cse4589_print_and_log("[%s:SUCCESS]\n", msgv[0]);
									cse4589_print_and_log("IP:%s\n", inet_ntoa(clientaddr.sin_addr));
									cse4589_print_and_log("[%s:END]\n", msgv[0]);	
								}else{
									cse4589_print_and_log("[%s:ERROR]\n", msgv[0]);
									cse4589_print_and_log("[%s:END]\n", msgv[0]);	
								}
							}else if(strcmp(msgv[0],"PORT") == 0){				
								cse4589_print_and_log("[%s:SUCCESS]\n", msgv[0]);
								cse4589_print_and_log("PORT:%d\n", atoi(argv[2]));									
								cse4589_print_and_log("[%s:END]\n", msgv[0]);			
							}else if(strcmp(msgv[0],"LIST") == 0){	
							 	cse4589_print_and_log("[%s:SUCCESS]\n", msgv[0]);	
							 	Clients_print(L_head, 0);							
								cse4589_print_and_log("[%s:END]\n", msgv[0]);

							}else if(strcmp(msgv[0], "LOGIN") == 0){ //msgv[1] is IP address, msgv[2] is port number						
								if( isValidIpAddress(msgv[1]) && (1<=s2i(msgv[2]) && s2i(msgv[2])<=65535) ){

									cse4589_print_and_log("[%s:SUCCESS]\n", msgv[0]);

									char msg_login[MSG_SIZE];
									memset(msg_login, '\0', MSG_SIZE);
									/////////////make socket for remote server////
									server = socket(AF_INET, SOCK_STREAM, 0);		
									if(server < 0)
							       		perror("Failed to create socket");
							       	bzero(&remote_server_addr, sizeof(remote_server_addr));
									//////////////////////////////////////////////											
									si_port = s2i(msgv[2]);
									login_success = 1; //declare login_success = 0 before.
									printf("si_port is %d\n", si_port);

									remote_server_addr.sin_family = AF_INET;
								    // converts a human readable IPv4 address into its sin_addr representation.
								    inet_pton(AF_INET, msgv[1], &remote_server_addr.sin_addr);	
								    remote_server_addr.sin_port = htons(si_port); 	//make int to network readable

								    
								    if(connect(server, (struct sockaddr*)&remote_server_addr, sizeof(remote_server_addr)) < 0){  //connect
								        perror("Connect failed");
								    }
								    printf("\nI am giving server my ip and port!\n");
								    strcat(msg_login, msgv[0]);
								    strcat(msg_login," ");
								    strcat(msg_login, inet_ntoa(clientaddr.sin_addr));
								    strcat(msg_login," ");
								    strcat(msg_login, argv[2]);
								    printf("msg_login%s\n", msg_login);
								    if(send(server, msg_login, strlen(msg_login), 0) == strlen(msg_login)){    
										printf("Done!\n");	
									}								
									fflush(stdout);	

								    ///////////////////////GET LIST////////////////////////////////////////////////////
								    printf("\n I am telling server i want to the clients list!\n");
									if(send(server, "REFRESH", strlen("REFRESH"), 0) == strlen("REFRESH")){    
										printf("Done!\n");	
									}								
									fflush(stdout);																		
										getsockname(server, (struct sockaddr*)&clientin, &clientin_len);
									//Store List of all currently logged-in clients responded by server.
									cse4589_print_and_log("[%s:END]\n", msgv[0]);
									FD_SET(server, &master_list);
									if(server > head_socket){
										head_socket = server;
									}
								}else{
									cse4589_print_and_log("[%s:ERROR]\n", msgv[0]);
									cse4589_print_and_log("[%s:END]\n", msgv[0]);						
								}  
							}
							else if(strcmp(msgv[0], "REFRESH") == 0){			
								if(login_success == 0){
									// print please login first
									cse4589_print_and_log("[%s:ERROR]\n", msgv[0]);
									cse4589_print_and_log("[%s:END]\n", msgv[0]);
								}else{
									cse4589_print_and_log("[%s:SUCCESS]\n", msgv[0]);
									//printf("\n I am telling server i want to REFRESH!");
									if(send(server, msgv[0], strlen(msgv[0]), 0) == strlen(msgv[0])){    
										printf("Done!\n");	
									}								
									fflush(stdout);	
									//Clients_clear(L_head);
									// send request to server then get the currently logged-in clients
									cse4589_print_and_log("[%s:END]\n", msgv[0]);
								}
							}
							else if(strcmp(msgv[0], "SEND") == 0){  //msgv[1] is dest ip  msgv[2] is message
								if(login_success == 0){
									// print please login first
									cse4589_print_and_log("[%s:ERROR]\n", msgv[0]);
									cse4589_print_and_log("[%s:END]\n", msgv[0]);
								}else{
									// CONVERT IP TO NETWORK IP AND STORE IT IN des_client
									struct sockaddr_in des_client;
									inet_aton(msgv[1], &(des_client.sin_addr));
									char smsg[MSG_SIZE];
									memset(smsg, '\0', MSG_SIZE);

									if( !(isValidIpAddress(msgv[1])) ){
										cse4589_print_and_log("[%s:ERROR]\n", msgv[0]);
										printf("invalid IP ADDR");
										cse4589_print_and_log("[%s:END]\n", msgv[0]);
									}else if(Client_location(L_head, des_client) == -1){
										cse4589_print_and_log("[%s:ERROR]\n", msgv[0]);
										printf("Client not in list");
										cse4589_print_and_log("[%s:END]\n", msgv[0]);
									}
									else{						
										cse4589_print_and_log("[%s:SUCCESS]\n", msgv[0]);
										strcat(smsg, msgv[0]);
										strcat(smsg, " ");
										strcat(smsg, msgv[1]);
										strcat(smsg, " ");
										strcat(smsg, msgv[2]);
										printf("SENDing:%s",smsg);
										if(send(server, smsg, strlen(smsg), 0) == strlen(smsg))    //server is socket.meg here is a point which point to a sting.
											printf("Done!\n");									// when login we can send login to server for it to provide specific service
										fflush(stdout);			//fflush the stdout
										// send the message to server
										cse4589_print_and_log("[%s:END]\n", msgv[0]);
									}
								 }
							}
							else if(strcmp(msgv[0], "BROADCAST") == 0){	//msgv[1] is message.
		
								if(login_success == 0){
									cse4589_print_and_log("[%s:ERROR]\n", msgv[0]);
									cse4589_print_and_log("[%s:END]\n", msgv[0]);
									// print please login first
								}else{
									cse4589_print_and_log("[%s:SUCCESS]\n", msgv[0]);
									char bmsg[MSG_SIZE];
									memset(bmsg, '\0', MSG_SIZE);
									strcat(bmsg, msgv[0]);	
									strcat(bmsg, " ");							
									strcat(bmsg, msgv[1]);
									strcat(bmsg," ");
									strcat(bmsg,msgv[2]);
									printf("\n I will broadcast:%s\n", bmsg);
									if(send(server, bmsg, strlen(bmsg), 0) == strlen(bmsg)){    
										printf("Done!\n");	
									}
									fflush(stdout);	
									cse4589_print_and_log("[%s:END]\n", msgv[0]);
								}
							}
							else if(strcmp(msgv[0], "BLOCK") == 0){  //msgv[1] is the ip of client been blocked			
								if(login_success == 0){
									// print please login first
									// cse4589_print_and_log("[%s:ERROR]\n", msgv[0]);
									// cse4589_print_and_log("[%s:END]\n", msgv[0]);
								}else{
									if(!isValidIpAddress(msgv[1])){
										//invalid ip
										cse4589_print_and_log("[%s:ERROR]\n", msgv[0]);
										cse4589_print_and_log("[%s:END]\n", msgv[0]);
									}else if(Client_location_stringip(L_head, msgv[1]) == -1){
										// not in the logged in list
										cse4589_print_and_log("[%s:ERROR]\n", msgv[0]);
										cse4589_print_and_log("[%s:END]\n", msgv[0]);
									}	
									else if( Cblock_location(B_head, msgv[1]) != -1 ){ 
										//client ip is already blocked 	
										cse4589_print_and_log("[%s:ERROR]\n", msgv[0]);
										cse4589_print_and_log("[%s:END]\n", msgv[0]);
									}
									else{
										cse4589_print_and_log("[%s:SUCCESS]\n", msgv[0]);					
										
										char b_msg[MSG_SIZE];
										memset(b_msg, '\0', MSG_SIZE);

										//Set local block list;
										struct blockofclient *btemp;
										btemp = (struct blockofclient *) malloc(sizeof(struct blockofclient));
										inet_aton(inet_ntoa(clientaddr.sin_addr),&(btemp->bc_src));
										inet_aton(msgv[1], &(btemp->bc_des));
										Cblock_insert(B_head, btemp);

										// send a request to server ask server to block specific client.
										strcat(b_msg, msgv[0]);
										strcat(b_msg, " ");
										strcat(b_msg, inet_ntoa(clientaddr.sin_addr)); //src ip
										strcat(b_msg, " ");
										strcat(b_msg, msgv[1]);

										printf("\n I will send %s to server to block:%s\n", b_msg, msgv[1]);
										if(send(server, b_msg, strlen(b_msg), 0) == strlen(b_msg)){    
											printf("Done!\n");	
										}
										fflush(stdout);	

										cse4589_print_and_log("[%s:END]\n", msgv[0]);					
									}
								}// end login succuess
							}// EDN BLOCK
							else if(strcmp(msgv[0], "UNBLOCK") == 0){ //msgv[1] client-ip			
								if(login_success == 0){
									// print please login first
									// cse4589_print_and_log("[%s:ERROR]\n", msgv[0]);
									// cse4589_print_and_log("[%s:END]\n", msgv[0]);
								}else{
									if(!isValidIpAddress(msgv[1])){
										// invalid ip
										cse4589_print_and_log("[%s:ERROR]\n", msgv[0]);
										cse4589_print_and_log("[%s:END]\n", msgv[0]);
									}else if(Client_location_stringip(L_head, msgv[1]) == -1){
										// not in the logged in list
										cse4589_print_and_log("[%s:ERROR]\n", msgv[0]);
										cse4589_print_and_log("[%s:END]\n", msgv[0]);
									}
									else if(Cblock_location(B_head, msgv[1]) == -1 ){
										//client with ip is not blocked
										cse4589_print_and_log("[%s:ERROR]\n", msgv[0]);
										cse4589_print_and_log("[%s:END]\n", msgv[0]);
									}
									else{
										cse4589_print_and_log("[%s:SUCCESS]\n", msgv[0]);	
										char ub_msg[MSG_SIZE];
										memset(ub_msg, '\0', MSG_SIZE);
										//unblock client in local block link.	
										Cblock_delete(B_head, msgv[1]);
										// send a request to ask server to ublock specific client
										strcat(ub_msg, msgv[0]);
										strcat(ub_msg, " ");
										strcat(ub_msg, inet_ntoa(clientaddr.sin_addr));
										strcat(ub_msg, " ");
										strcat(ub_msg, msgv[1]);
										printf("\n I will send %s to server to unblock:%s\n", ub_msg, msgv[1]);
										if(send(server, ub_msg, strlen(ub_msg), 0) == strlen(ub_msg)){    
											printf("Done!\n");	
										}
										fflush(stdout);	

										cse4589_print_and_log("[%s:END]\n", msgv[0]);			
									}				
								} // login success
							} //END UNBLOCK

							else if(strcmp(msgv[0], "LOGOUT") == 0){	
								if(login_success == 0){
									cse4589_print_and_log("[%s:ERROR]\n", msgv[0]);
									cse4589_print_and_log("[%s:END]\n", msgv[0]);
									// print you are already LOGOUT
								}else{
									// send request to server then get the currently logged-in clients
									cse4589_print_and_log("[%s:SUCCESS]\n", msgv[0]);
									printf("\n I am telling server i will log out!");

									strcat(msgv[0], " ");
									strcat(msgv[0], inet_ntoa(clientaddr.sin_addr));
									printf("msgv[0]:%s\n", msgv[0]);
									if(send(server, msgv[0], strlen(msgv[0]), 0) == strlen(msgv[0])){    
										printf("Done!\n");	
									}								
									fflush(stdout);	
									//notify server the logout
									close(server);
									FD_CLR(server, &master_list);
									login_success = 0;
									cse4589_print_and_log("[%s:END]\n", msgv[0]);
								}
							}
							else if(strcmp(msgv[0], "EXIT") == 0){
								cse4589_print_and_log("[%s:SUCCESS]\n", msgv[0]);
								if(login_success == 0){
									cse4589_print_and_log("[%s:END]\n", msgv[0]);
									exit(0);
								}else{
									printf("\n I am telling server i will EXIT!");
									strcat(msgv[0], " ");
									strcat(msgv[0], inet_ntoa(clientaddr.sin_addr));
									printf("msgv[0]:%s\n", msgv[0]);
									if(send(server, msgv[0], strlen(msgv[0]), 0) == strlen(msgv[0])){    
										printf("Done!\n");	
									}								
									fflush(stdout);	
									//notify server the logout
									close(server);
									FD_CLR(server, &master_list);
									login_success = 0;
									cse4589_print_and_log("[%s:END]\n", msgv[0]);
									exit(0); // terminate the application with exit code 0.	
								}							
							}
							else{
								printf("ERROR");
							}
							free(msg);
							//FD_SET(server, &master_list);
	                    } //end if(sock_index == STDIN)
	                    // if(sock_index == server)
	                    else if(sock_index == server){	 // receive from server 	                    	
					        char *buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);	//create a buffer to get server's message
					        memset(buffer, '\0', BUFFER_SIZE);
							if(recv(server, buffer, BUFFER_SIZE, 0) >= 0){				//get buffer's message.
								////////////divide user input msg and store snippets into msgv array///////////////////////////////////
								// learn from https://stackoverflow.com/questions/4513316/split-string-in-c-every-white-space
								int i;
								char cbufv[3][BUFFER_SIZE];  // msgv is used to store User's command and message
								for(i=0; i < 3; i++)	//memset the msgv array
								{
									memset(cbufv[i],'\0',BUFFER_SIZE);
								}

								char *pch;
								pch = strtok (buffer," ");	
								i=0;  	//reset i to 0;
								while (pch != NULL)	// this will divide the original message and store it into msgv array
								{		  
								  if(i < 2){
								   	strcpy(cbufv[i], pch);
								  }else{			  	
								  	strcat(cbufv[2], pch);
								  	strcat(cbufv[2], " ");   // FOR THE SUCCESS OF s2i();
								  }
								  pch = strtok (NULL, " ");
								  i++;
								}
								cbufv[2][strlen(cbufv[2])-1] = '\0'; //delete the last space of msgv[2]
								if(strcmp(cbufv[0], "REFRESH") == 0){
									//cse4589_print_and_log("[%s:SUCCESS]\n", "REFRESH");
									struct clientinfo *temp, *ptr_ref;
									char *pch_ref;
									Clients_clear(L_head);
									ptr_ref = L_head;
									pch_ref = strtok (cbufv[2]," "); 
									i=0;  	//reset i to 0;
									while (pch_ref != NULL)	// this will divide the original message and store it into msgv array
									{		
										switch(i%6){
											case 0: 
												temp = NULL;
												temp =  (struct clientinfo *) malloc(sizeof(struct clientinfo));
												printf("0 pch_ref:%s\n", pch_ref);
												strcpy(temp->ci_hostname, pch_ref);
												printf("temp->ci_hostname%s\n", temp->ci_hostname);
												break;
											case 1:
												printf("1 pch_ref:%s", pch_ref);
												inet_aton(pch_ref, &(temp->ci_addr));
												break;
											case 2: 
												temp->ci_port = string2uint(pch_ref);
												break;
											case 3: 
												temp->ci_num_msg_sent = atoi(pch_ref);
												break;
											case 4: 
												temp->ci_num_msg_rcv = atoi(pch_ref);
												break;
											case 5: 
												if(atoi(pch_ref) == 0){
													temp->ci_status = false;
												}else{
													temp->ci_status = true;
												}
												ptr_ref->ci_next = temp;
												ptr_ref = ptr_ref->ci_next;
												break;
											default:
												printf("\nerror\n");
										} // end swithch
									  	pch_ref = strtok (NULL, " ");
									  	i++;
									}	// end while
								} //END REFRESH
								else if(strcmp(cbufv[0], "BROADCAST") == 0){
									cse4589_print_and_log("[%s:SUCCESS]\n", "RECEIVED");
									cse4589_print_and_log("msg from:%s\n[msg]:%s\n", cbufv[1], cbufv[2]);
									cse4589_print_and_log("[%s:END]\n", "RECEIVED");	
								}else if(strcmp(cbufv[0], "SEND") == 0){
									cse4589_print_and_log("[%s:SUCCESS]\n", "RECEIVED");
									cse4589_print_and_log("msg from:%s\n[msg]:%s\n", cbufv[1], cbufv[2]);
									cse4589_print_and_log("[%s:END]\n", "RECEIVED");	
								}
							} //END RECV
							  free(buffer);            		                        
	                    }// end else if(sock_index == server_socket)             
	        		}// end if(FD_ISSET(sock_index, &watch_list))       		
	        	} // end for(sock_index=0; sock_index<=head_socket; sock_index+=1)
	        } // end if(selret > 0)
	    }
	    return 0;
	}

//////////////////////////////client code stop here////////////////////////////////////////////
 	return 0;
}


////////////////////////////Function Declaration start////////////////////////////////////////////
int Find_local_ip(struct sockaddr_in *clientaddr)
{
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	//struct sockaddr_in clientaddr;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(UDPSERVERIP, UDPSERVERPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
			close(sockfd);
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
		return 2;
	}

	int clientaddr_len;
	clientaddr_len = sizeof(struct sockaddr_in);							//get clientaddr_len
	getsockname(sockfd, (struct sockaddr*)clientaddr, &clientaddr_len);		//store client information in clientaddr

	freeaddrinfo(servinfo);
	close(sockfd);
}

//future we may need to make the message number not change, but port change.When client log in in second time.
struct clientinfo Client_initiate(struct sockaddr_in client_addr)    
{
	struct clientinfo customer;											
	char service[20];
	getnameinfo(&client_addr, sizeof client_addr, customer.ci_hostname,
	                        	sizeof customer.ci_hostname, service, sizeof service, 0 );
	customer.ci_addr = client_addr.sin_addr;
	customer.ci_port = client_addr.sin_port;
	customer.ci_num_msg_sent = 0;
	customer.ci_num_msg_rcv = 0;
	customer.ci_status = true;
	return customer;
}

int Client_logout(struct clientinfo *L_head, char ip[]){
	struct clientinfo *p;
	p=L_head->ci_next;
	char *str1;

	while(p){
		str1 = inet_ntoa(p->ci_addr);			// might be the danger code
		printf("\nstr1 client ip %s\n",ip);
		printf("IP in list:%s\n", str1);	
		if( strcmp(ip, str1) == 0){
			p->ci_status =false;
			return 0;
		}
			p=p->ci_next;
	}
	return -1;
}

int Client_login(struct clientinfo *L_head, char ip[]){
	struct clientinfo *p;
	p=L_head->ci_next;
	char *str1;

	while(p){
		str1 = inet_ntoa(p->ci_addr);
		printf("\nstr1 client ip %s\n",ip);
		printf("IP in list:%s\n", str1);	
		if( strcmp(ip, str1) == 0){
			p->ci_status =true;
			return 0;
		}
			p=p->ci_next;
	}
	return -1;
}

int Client_location_stringip(struct clientinfo *L_head, char ip[]){
	struct clientinfo *p;
	p=L_head->ci_next;
	char *str1;
	int i = 1;

	while(p){
		str1 = inet_ntoa(p->ci_addr);
		printf("\nstr1 client ip %s\n",ip);
		printf("IP in list:%s\n", str1);	
		if( strcmp(ip, str1) == 0){
			return i;
		}
			p=p->ci_next;
			i++;
	}
	return -1;
}

int Client_set_send(struct clientinfo *L_head, char ip[]){
	struct clientinfo *p;
	p=L_head->ci_next;
	char *str1;

	while(p){
		str1 = inet_ntoa(p->ci_addr);
		printf("\nstr1 client ip %s\n",ip);
		printf("IP in list:%s\n", str1);	
		if( strcmp(ip, str1) == 0){
			p->ci_num_msg_sent +=1;
			return 0;
		}
			p=p->ci_next;
	}
	return -1;
}

int Client_set_recv(struct clientinfo *L_head, char ip[]){
	struct clientinfo *p;
	p=L_head->ci_next;
	char *str1;

	while(p){
		str1 = inet_ntoa(p->ci_addr);
		printf("\nstr1 client ip %s\n",ip);
		printf("IP in list:%s\n", str1);	
		if( strcmp(ip, str1) == 0){
			p->ci_num_msg_rcv += 1;
			return 0;
		}
			p=p->ci_next;
	}
	return -1;
}

int Client_set_recv_b(struct clientinfo *L_head, char ip[]){
	struct clientinfo *p;
	p=L_head->ci_next;
	char *str1;

	while(p){
		str1 = inet_ntoa(p->ci_addr);
		if( strcmp(ip, str1) != 0){
			p->ci_num_msg_rcv += 1;
		}
			p=p->ci_next;
	}
	return -1;
}

int Client_list2string(struct clientinfo *L_head, char buf[]){  //the first in buf is the commands
	struct clientinfo *p;
	p=L_head->ci_next;
	char *str1;
	char temp[10];

	while(p){
		if(p->ci_status == true){
			strcat(buf, " ");
			strcat(buf, p->ci_hostname);

			strcat(buf, " ");
			str1 = inet_ntoa(p->ci_addr);
			strcat(buf, str1);

			strcat(buf, " ");
			sprintf(temp, "%u", p->ci_port);
			strcat(buf,temp);

			strcat(buf, " ");
			sprintf(temp, "%d", p->ci_num_msg_sent);
			strcat(buf, temp);

			strcat(buf, " ");
			printf(temp, "%d", p->ci_num_msg_rcv);
			strcat(buf, temp);

			strcat(buf, " ");
			if(p->ci_status){
				sprintf(temp, "%d", 1);
			}else{
				sprintf(temp, "%d", 0);
			}
			strcat(buf, temp);
		}		
		p=p->ci_next;		
	}	
	printf("\n%s\n", buf);
	return 1;
}

//learn from https://stackoverflow.com/questions/34206446/how-to-convert-string-into-unsigned-int-c
unsigned int string2uint(char *st) {
  char *x;
  for (x = st ; *x ; x++) {
    if (!isdigit(*x))
      return 0L;
  }
  return (strtoul(st, 0L, 10));
}

// Learn how to clear linklist from http://blog.csdn.net/bzhxuexi/article/details/41721429
int Clients_clear(struct clientinfo *L_head){
	struct clientinfo *p, *q;
	if(L_head == NULL){
		return 0;
	}
	p=L_head->ci_next;
	while(p!=NULL){
		q=p->ci_next;
		free(p);
		p=q;
	}
	L_head->ci_next = NULL;
	return 1;
}


int Client_exit(struct clientinfo *L_head, char ip[]){

	struct clientinfo *p, *q;
	q= L_head;
	p=L_head->ci_next;
	char *str1;

	while(p){
		str1 = inet_ntoa(p->ci_addr);	  // might be the danger code
		printf("\nstr1 client ip %s\n",ip);
		printf("IP in list:%s\n", str1);	
		if( strcmp(ip, str1) == 0){
			q->ci_next = p->ci_next;
			p->ci_next = NULL;
			free(p);
			return 0;
		}
			p=p->ci_next;
			q=q->ci_next;
	}
	return -1;
}

int Client_location(struct clientinfo *L_head, struct sockaddr_in client_addr){
	struct clientinfo *p;
	p=L_head->ci_next;
	int i = 1;
	char *str1, *str2;
	str1 = inet_ntoa(client_addr.sin_addr);
	char *str3 = (char*) malloc(sizeof(char)* strlen(str1));
	memcpy(str3, str1, strlen(str1));

	while(p){
		str2 = inet_ntoa(p->ci_addr);	
		if( strcmp(str3, str2) == 0){
			return i;
		}
			p=p->ci_next;
			i++;
	}
	return -1;
}

int Client_delete(struct clientinfo *L_head, int loc){
	struct clientinfo *p, *q;
	q=L_head;
	p=L_head->ci_next;
	int i=1;
	while(p){
		if(i == loc){
			q->ci_next = p->ci_next;
			p->ci_next = NULL;
			free(p);
			return 0;
		}
		p=p->ci_next;
		q=q->ci_next;
		i++;	
	}
	return -1;
}


void Client_insert(struct clientinfo *L_head, struct clientinfo *elem)
{
	struct clientinfo *p, *q;
	p=L_head;
	q=L_head->ci_next;

	if(q == NULL){

	}else{
		while(q && q->ci_port < elem->ci_port){
			q=q->ci_next;
			p=p->ci_next;
		}
	}

	elem->ci_next = p->ci_next;
	p->ci_next = elem;
}


int Clients_print(struct clientinfo *L_head, int opt)  //opt:0 pirnt in list style.  opt:1 pirnt statistics style
{
	struct clientinfo *p;
	p=L_head->ci_next;
	int i = 1;
	if(opt == 0){			//print the list style
		while(p){
			if(p->ci_status == true){   // if the client is login, print it out
				cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", i, p->ci_hostname, inet_ntoa(p->ci_addr), p->ci_port);
				p=p->ci_next;
				i++;
			}else{
				p=p->ci_next;
			}
		}
		return 0;		
	}
	if(opt == 1){			//print the statistic style
		while(p){
			if(p->ci_status == false){  // client logged out
				cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8s\n", i, p->ci_hostname ,p->ci_num_msg_sent,p->ci_num_msg_rcv, "logged-out");
			}
			if(p->ci_status == true){	//client logged in
				cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8s\n", i, p->ci_hostname ,p->ci_num_msg_sent,p->ci_num_msg_rcv, "logged-in");
			}
			
			p=p->ci_next;
			i++;
		}
		return 1;
	}

}

// Determine if a string is a valid IP address in C
// from: https://stackoverflow.com/questions/791982/determine-if-a-string-is-a-valid-ip-address-in-c
bool isValidIpAddress(char *ipAddress)
{
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
    return result != 0;
}


unsigned int s2i(char *str)
{
	int len = strlen(str);
	int i = 0;
	for(i=0; i<len; i++){
		if(48<= str[i]&&str[i]<=57)
		{
		}else{
			return -1;
		}
	}
	return atoi(str);
}

int connect_to_host(char *server_ip, int server_port)
{
    int fdsocket, len;							//
    struct sockaddr_in remote_server_addr;		//save the server info?

    fdsocket = socket(AF_INET, SOCK_STREAM, 0);		//get socket
    if(fdsocket < 0)
       perror("Failed to create socket");

    bzero(&remote_server_addr, sizeof(remote_server_addr));
    remote_server_addr.sin_family = AF_INET;
    // converts a human readable IPv4 address into its sin_addr representation.
    inet_pton(AF_INET, server_ip, &remote_server_addr.sin_addr);	
    remote_server_addr.sin_port = htons(server_port); 	//make int to network readable

    if(connect(fdsocket, (struct sockaddr*)&remote_server_addr, sizeof(remote_server_addr)) < 0)  //connect
        perror("Connect failed");
    // can we get port here? use print to test
    return fdsocket;		// return socket
}


int Cblock_delete(struct blockofclient *B_head, char des[]){
	struct blockofclient *p, *q;
	q=B_head;
	p=B_head->bc_next;
	char *str1;

	while(p){
		str1 = inet_ntoa(p->bc_des);
		if(strcmp(des, str1) == 0){
			q->bc_next = p->bc_next;
			p->bc_next = NULL;
			free(p);
			return 1;
		}
		p=p->bc_next;
		q=q->bc_next;
	}
	return -1;
}


int Cblock_location(struct blockofclient *B_head, char des[]){
	struct blockofclient *p;
	p=B_head->bc_next;
	char *str1;
	int i = 1;

	while(p){
		str1 = inet_ntoa(p->bc_des);
		if(strcmp(des, str1) == 0){
			return i;
		}
		p=p->bc_next;
		i++;
	}
	return -1;
}

void Cblock_insert(struct blockofclient *B_head, struct blockofclient *elem){
	struct blockofclient *p,*q;
	p = B_head;
	q = B_head->bc_next;
	if(q == NULL){

	}else{
		while(q){
			q=q->bc_next;
			p=p->bc_next;
		}
	}
	elem->bc_next = p->bc_next;
	p->bc_next = elem;
}

/////////////server block/////////////////////////

int Sblock_location(struct serverblocklist *Bs_head, char src[], char des[]){
	struct serverblocklist *p, *q;
	q=Bs_head;
	p=Bs_head->bs_next;
	char *str1, *str2;
	char *str3 = (char*) malloc(sizeof(char)* MSG_SIZE);
	char *str4 =  (char*) malloc(sizeof(char)* MSG_SIZE);
	int i = 1;

	memset(str3, '\0', MSG_SIZE);
	memset(str4, '\0', MSG_SIZE);


	while(p){
		str1 = inet_ntoa(p->bs_src);
		memcpy(str3, str1, strlen(str1));
		str2 = inet_ntoa(p->bs_des);
		memcpy(str4, str2, strlen(str2));
		if( (strcmp(src, str3) == 0) && (strcmp(des, str4) == 0) ){
			return i;
		}
		p = p->bs_next;
		i++;
	}
	return -1;
}

int Sblock_delete(struct serverblocklist *Bs_head, char src[], char des[]){
	struct serverblocklist *p, *q;
	q=Bs_head;
	p=Bs_head->bs_next;
	char *str1, *str2;
	char *str3 = (char*) malloc(sizeof(char)* MSG_SIZE);
	char *str4 =  (char*) malloc(sizeof(char)* MSG_SIZE);

	memset(str3, '\0', MSG_SIZE);
	memset(str4, '\0', MSG_SIZE);


	while(p){
		str1 = inet_ntoa(p->bs_src);
		memcpy(str3, str1, strlen(str1));
		str2 = inet_ntoa(p->bs_des);
		memcpy(str4, str2, strlen(str2));
		if( (strcmp(src, str3) == 0) && (strcmp(des, str4) == 0) ){
			q->bs_next = p->bs_next;
			p->bs_next = NULL;
			free(p);
			return 1;
		}
		p = p->bs_next;
	}
	return -1;
}

void Sblock_print(struct serverblocklist *Bs_head, char ip[]){
	struct serverblocklist *p;
	p=Bs_head->bs_next;
	char *str1;
	int i = 1;
	while(p){
		str1 = inet_ntoa(p->bs_src);
		if(strcmp(ip, str1) == 0){
			cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", i, p->bs_des_hostname, inet_ntoa(p->bs_des), p->bs_des_port);
			i++;
		}
		p = p->bs_next;
	}
}

void Sblock_insert(struct serverblocklist *Bs_head, struct serverblocklist *elem){
	struct serverblocklist *p, *q;
	p=Bs_head;
	q=Bs_head->bs_next;
	if(q == NULL){

	}else{
		while(q && q->bs_des_port < elem->bs_des_port){
			q=q->bs_next;
			p=p->bs_next;
		}
	}
	elem->bs_next = p->bs_next;
	p->bs_next = elem;
}

int Sblock_initiate(struct clientinfo *L_head, char ip[], struct serverblocklist *b_customer){
	struct clientinfo *p;
	p=L_head->ci_next;
	char *str1;

	while(p){
		str1 = inet_ntoa(p->ci_addr);
		if( strcmp(ip, str1) == 0){
			strcat(b_customer->bs_des_hostname, p->ci_hostname);
			b_customer->bs_des = p->ci_addr;
			b_customer->bs_des_port = p->ci_port;
			return 1;
		}
		p=p->ci_next;
	}
	return -1;
}

