#include <string.h>
#include <netinet/in.h>
#include "../include/global.h"
#include "../include/control_header_lib.h"
#include "../include/network_util.h"
#include "../include/table.h"


void init_response(int sock_index, char *cntrl_payload){
	
	//send response back controller
	uint16_t response_payload_len, response_len;
	char *cntrl_response_header, *cntrl_response;
	response_payload_len = 0;
	cntrl_response_header = create_response_header(sock_index, 1, 0, response_payload_len);
	response_len = CNTRL_RESP_HEADER_SIZE + response_payload_len;
	cntrl_response = (char *) malloc(response_len);
	/* Copy Header */
	memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
	free(cntrl_response_header);
	sendALL(sock_index, cntrl_response, response_len);
	free(cntrl_response);
	//end send response back controller
}

int init_table(char *cntrl_payload){
	if(init_routing_table(cntrl_payload) == 1){
		printf("init_routing_table Success\n");
		return 1;
	}else{
		printf("init_routing_table Failure\n");
		return 0;
	}
}

void init_list(){
	initTimeOutList();
}

void print_list(){
	printTimeOutList();
}








