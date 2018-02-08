#include <string.h>
#include <netinet/in.h>
#include "../include/global.h"
#include "../include/control_header_lib.h"
#include "../include/network_util.h"
#include "../include/crash.h"

void crash_response(int sock_index){
		//send response back controller
	uint16_t response_payload_len, response_len;
	char *cntrl_response_header, *cntrl_response;
	response_payload_len = 0;
	cntrl_response_header = create_response_header(sock_index, 4, 0, response_payload_len);
	response_len = CNTRL_RESP_HEADER_SIZE + response_payload_len;
	cntrl_response = (char *) malloc(response_len);
	/* Copy Header */
	memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
	free(cntrl_response_header);
	sendALL(sock_index, cntrl_response, response_len);
	free(cntrl_response);
	//end send response back controller
}

void crash_router(){
	crashed = 1;
}