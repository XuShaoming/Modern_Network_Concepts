#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <arpa/inet.h>

#include "../include/global.h"
#include "../include/network_util.h"
#include "../include/table.h"

int create_router_sock()
{
    int sock;
    struct sockaddr_in router_addr;
    socklen_t addrlen = sizeof(router_addr);	

    sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock < 0){
    	ERROR("socket() failed");
	}
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) < 0){
        ERROR("setsockopt() failed");
	}

	bzero(&router_addr, sizeof(router_addr));

	router_addr.sin_family = AF_INET;
    router_addr.sin_addr.s_addr = htonl(INADDR_ANY); // use my IPv4 address
    router_addr.sin_port = htons(ROUTER_PORT);
    if(bind(sock, (struct sockaddr *)&router_addr, sizeof(router_addr)) < 0){
        ERROR("bind() failed");
    }

    return sock;
}

int creat_router_client_socke()
{
	int sock;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock < 0){
    	ERROR("socket() failed");
	}
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) < 0){
        ERROR("setsockopt() failed");
	}

    return sock;
}

void routing_update(int router_client_socket)
{
	printf("*******in routing_update*************\n");
	char *update_packet;
	update_packet = make_update_packet();

	struct ROUTER_TIMEOUT *q;
	q = RT_head->next;
	int numbytes;
	uint16_t packet_length;
	packet_length = UPDATE_PACKET_HEAD_SIZE + num_of_routers * UPDATE_PACKET_CONTENT_SIZE;
	printf("packet_length %d\n", packet_length);
	while(q){
		if(q->router_id != my_id){
			if ((numbytes = sendto(router_client_socket, update_packet, packet_length, 0, 
				(struct sockaddr *)&q->router_addr, sizeof(q->router_addr))) == -1) {

				perror("talker: sendto");
				exit(1);
			}
			printf("send to router_id : %d numbytes: %d\n", q->router_id, numbytes);
		}
		q = q->next;
	}
	printf("*******out routing_update*************\n");
}

int router_recv_hook(int sock)
{
	printf("**********into router_recv_hook***************\n");
	char *update_packet;
	char *update_packet_header, *update_packet_content;
	uint16_t update_packet_length;
	ssize_t numbytes = 0;
	struct sockaddr_in their_addr;

    socklen_t addrlen;
    addrlen = sizeof(their_addr);
    bzero(&their_addr, sizeof(their_addr));

	update_packet_length = num_of_routers * UPDATE_PACKET_CONTENT_SIZE + UPDATE_PACKET_HEAD_SIZE;
	update_packet = (char *) malloc(sizeof(char)*update_packet_length);
	bzero(update_packet, update_packet_length);

	numbytes = recvfrom(sock, update_packet, update_packet_length, 0, 
        (struct sockaddr *)&their_addr, &addrlen);

	if(numbytes == -1){
        perror("recvfrom");
        return -1;
    }

    update_packet_header = (char *) malloc(sizeof(char)*UPDATE_PACKET_HEAD_SIZE);

    bzero(update_packet_header, UPDATE_PACKET_HEAD_SIZE);
    memcpy(update_packet_header, update_packet, UPDATE_PACKET_HEAD_SIZE);
    update_neighbor_node(update_packet_header);
    update_packet += UPDATE_PACKET_HEAD_SIZE;

   	update_packet_content = (char *) malloc(sizeof(char)*(num_of_routers * UPDATE_PACKET_CONTENT_SIZE));
   	bzero(update_packet_content, (num_of_routers * UPDATE_PACKET_CONTENT_SIZE));
   	memcpy(update_packet_content, update_packet, (num_of_routers * UPDATE_PACKET_CONTENT_SIZE));
   	update_routing_table(update_packet_content, &their_addr);

   	// free(update_packet);
   	// free(update_packet_header);
   	// free(update_packet_content);

   	printf("Datagram size: %ld.\n", numbytes);
    printf("Datagram's IP address is: %s\n", inet_ntoa(their_addr.sin_addr));
    printf("Datagram's port is: %d\n", (int) ntohs(their_addr.sin_port));
    printf("**********out router_recv_hook***************\n");
    return numbytes;
}


