#include <string.h>
#include <netinet/in.h>
#include "../include/global.h"
#include "../include/control_header_lib.h"
#include "../include/network_util.h"
#include "../include/table.h"
#include "../include/init.h"

#include <sys/socket.h>
#include <arpa/inet.h>

char *make_routing_table_response_element(struct ROUTING_TABLE element){
  char *buffer;
  BUILD_BUG_ON(sizeof(struct ROUTING_TABLE_RESPONSE_ELEMENT) != ROUTING_TABLE_RESPONSE_ELEMENT_SIZE);

  struct ROUTING_TABLE_RESPONSE_ELEMENT *response_element;
  buffer = (char *) malloc(sizeof(char)*ROUTING_TABLE_RESPONSE_ELEMENT_SIZE);
  response_element = (struct ROUTING_TABLE_RESPONSE_ELEMENT *) buffer;

  response_element->router_id = htons(element.router_id);
  response_element->padding = htons(0);
  response_element->next_hop_id = htons(element.next_hop_id);
  response_element->cost = htons(element.cost);

  return buffer;
}

void update_response(int sock_index){
  //send response back controller
  uint16_t response_payload_len, response_len;
  char *cntrl_response_header, *cntrl_response;
  response_payload_len = 0;
  cntrl_response_header = create_response_header(sock_index, 3, 0, response_payload_len);
  response_len = CNTRL_RESP_HEADER_SIZE + response_payload_len;
  cntrl_response = (char *) malloc(response_len);
  /* Copy Header */
  memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
  free(cntrl_response_header);
  sendALL(sock_index, cntrl_response, response_len);
  free(cntrl_response);
  //end send response back controller
}

void routing_table_response(int sock_index){
  uint16_t payload_len, response_len;
  char *cntrl_response_header, *cntrl_response_element, *cntrl_response_payload;
  char *cntrl_response;
  int i, loc;

  payload_len = num_of_routers * ROUTING_TABLE_RESPONSE_ELEMENT_SIZE;
  cntrl_response_payload = (char *) malloc(payload_len);

  for(i=0; i < num_of_routers; i++){
    cntrl_response_element = make_routing_table_response_element(Routing_table[i]);
    loc = ROUTING_TABLE_RESPONSE_ELEMENT_SIZE * i;
    memcpy(cntrl_response_payload + loc, cntrl_response_element, ROUTING_TABLE_RESPONSE_ELEMENT_SIZE);
  }
  free(cntrl_response_element);

  cntrl_response_header = create_response_header(sock_index, 2, 0, payload_len);

  response_len = CNTRL_RESP_HEADER_SIZE + payload_len;
  cntrl_response = (char *) malloc(response_len);

  /* Copy Header */
  memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
  free(cntrl_response_header);

  /* Copy Payload */
  memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, cntrl_response_payload, payload_len);
  free(cntrl_response_payload);

  sendALL(sock_index, cntrl_response, response_len);
  free(cntrl_response);
}

int init_routing_table(char *cntrl_payload){
	// get number of routers and updates Periodic Interval
	char *ptr;
	ptr = cntrl_payload;

	char *payload_head_buffer;
	char *payload_router_buffer;
	struct INIT_PAYLOAD_HEAD *payload_head;
	struct INIT_PAYLOAD_CONTENT *payload_router;

    payload_head_buffer = (char *) malloc(sizeof(char)*INIT_PAYLOAD_HEAD_SIZE);
    payload_router_buffer = (char *) malloc(sizeof(char)*INIT_PAYLOAD_CONTENT_SIZE);

    payload_head = (struct INIT_PAYLOAD_HEAD *) payload_head_buffer;
    payload_router = (struct INIT_PAYLOAD_CONTENT *) payload_router_buffer;

    //copy head into payload_head_buffer.
    memcpy(payload_head_buffer, ptr, INIT_PAYLOAD_HEAD_SIZE);
    num_of_routers =  ntohs(payload_head->num_of_routers);
    updates_periodic_interval = ntohs(payload_head->updates_periodic_interval);
    printf("num_of_routers: %d\n", num_of_routers);
    printf("updates_periodic_interval: %d\n", updates_periodic_interval);
    if(num_of_routers > 5){
    	printf("THE MAXIMUM NUMBER OF ROUTERS NOT OVER FIVE\n");
    	return 0;
    }
    ptr += INIT_PAYLOAD_HEAD_SIZE;

    int i;
    for(i = 0; i < num_of_routers; i++){
    	memcpy(payload_router_buffer, ptr, INIT_PAYLOAD_CONTENT_SIZE);
    	Routing_table[i].router_id =  ntohs(payload_router->router_id);
    	Routing_table[i].router_port = ntohs(payload_router->router_port);
    	Routing_table[i].data_port = ntohs(payload_router->data_port);
    	Routing_table[i].cost = ntohs(payload_router->cost);
    	Routing_table[i].ip_addr = ntohl(payload_router->ip_addr);

      Routing_topology[i].router_id =  ntohs(payload_router->router_id);
      Routing_topology[i].router_port = ntohs(payload_router->router_port);
      Routing_topology[i].data_port = ntohs(payload_router->data_port);
      Routing_topology[i].cost = ntohs(payload_router->cost);
      Routing_topology[i].ip_addr = ntohl(payload_router->ip_addr);

      
    	//its neightbor or myself
    	if(Routing_table[i].cost < INF){
    		Routing_table[i].next_hop_id = Routing_table[i].router_id;
        Routing_topology[i].next_hop_id = Routing_topology[i].router_id;

        if(Routing_table[i].cost == 0){  // myself
          my_id =Routing_table[i].router_id;
          my_port = Routing_table[i].router_port;
          my_ip = Routing_table[i].ip_addr;
          ROUTER_PORT = Routing_table[i].router_port;
          DATA_PORT = Routing_table[i].data_port;
        }
    	}else{
    		Routing_table[i].next_hop_id = INF;
        Routing_topology[i].next_hop_id = INF;
    	}
      printf("*************check Router element**************************\n");
    	printf("ROUTING_table[%d] info:\n", i);
    	printf("router_id: %d\n", Routing_table[i].router_id);
    	printf("router_port: %d\n", Routing_table[i].router_port);
    	printf("data_port: %d\n", Routing_table[i].data_port);
    	printf("cost: %d\n", Routing_table[i].cost);
    	printf("ip_addr: %u\n", Routing_table[i].ip_addr);
    	printf("next_hop_id: %d\n", Routing_table[i].next_hop_id);
      printf("*************end Router element**************************\n");

    	ptr = ptr + INIT_PAYLOAD_CONTENT_SIZE;
    }
	return 1;
}

void update_topology_table(char *cntrl_payload){
    printf("*************into update_topology_table**********\n");
    char *ptr;
    ptr = cntrl_payload;
    char *controller_update_buffer;
    struct CONTROLLER_UPDATE_CONTENT *controller_update;
    uint16_t update_id;
    uint16_t update_cost;
    int32_t difference; 

    controller_update_buffer = (char *) malloc(sizeof(char)*CONTROLLER_UPDATE_CONTENT_SIZE);
    controller_update = (struct CONTROLLER_UPDATE_CONTENT *) controller_update_buffer;

    memcpy(controller_update_buffer, ptr, CONTROLLER_UPDATE_CONTENT_SIZE);

    update_id = ntohs(controller_update->router_id);
    update_cost = ntohs(controller_update->cost);
    free(controller_update_buffer);

    difference = controller_update_topology(update_id, update_cost);
    controller_update_routing_table(update_id, difference);
    printf("*************out update_topology_table**********\n");
}

void controller_update_routing_table(uint16_t update_id, int32_t difference){
  printf("*************into controller_update_routing_table**********\n");
  int i;
  for(i = 0; i < num_of_routers ; i++){
    if(Routing_table[i].next_hop_id == update_id){
      if(difference == INF){
        Routing_table[i].cost = INF;
        printf("set cost to be INF\n");
      }else{
        printf("to router %u's old cost is %u\n", Routing_table[i].router_id, Routing_table[i].cost);
        Routing_table[i].cost = Routing_table[i].cost - difference;
        printf("new cost is %u\n", Routing_table[i].cost);
      }
    }//end if(Routing_table[i].next_hop_id == update_id)
  }//end for
  printf("*************out controller_update_routing_table**********\n");
}

int32_t controller_update_topology(uint16_t update_id, uint16_t update_cost){
  printf("*************into controller_update_topology**********\n");
  int i;
  int32_t difference;
  printf("router_id %u\nupdate_cost %u\n", update_id, update_cost);
  if(update_cost < 0){
    printf("cost need to be larger than 0\n");
    return 0;
  }

  for(i = 0; i< num_of_routers; i++){
    if(update_id == Routing_topology[i].router_id){
      printf("router id %u original cost is %u\n",Routing_topology[i].router_id, Routing_topology[i].cost);
      if(update_cost == INF){
        difference = INF; 
        Routing_topology[i].cost = INF;
        printf("return difference to be INF");  
      }else if(update_cost < INF){
        difference = Routing_topology[i].cost - update_cost;
        Routing_topology[i].cost = update_cost;
        printf("return difference to be %d\n", difference);
      }else{
        printf("error\nmaximum cost is 65535\n");
        printf("remain topology unchanged, set difference to be 0\n");
        difference = 0;
      }
      printf("*************out controller_update_topology**********\n");
      return difference;
    }
  }
    printf("can't find router id %u\n", update_id);
    printf("remain topology unchanged, set difference to be 0\n");
    printf("*************out controller_update_topology**********\n");
    return 0;
}

void update_routing_table(char *update_packet_content, struct sockaddr_in *their_addr){
  printf("*************into update_routing_table**********\n");
  char *ptr;
  ptr = update_packet_content;
  char *update_router_buffer;
  struct UPDATE_PACKET_CONTENT *update_router;
  uint16_t v_id, c_xv_cost;
  uint32_t y_ip;
  uint16_t y_port, y_id, d_vy_cost;  // x-v-y, v send y info to x
  uint16_t d_xy_cost, xy_next_hop_id;
  int i,j;

  find_id_cost_in_topo(&v_id, &c_xv_cost, their_addr);
  printf("v_id: %u, c_xv_cost: %u\n", v_id, c_xv_cost);

  update_router_buffer = (char *) malloc(sizeof(char)*UPDATE_PACKET_CONTENT_SIZE);
  update_router = (struct UPDATE_PACKET_CONTENT *) update_router_buffer;

  for(i = 0; i < num_of_routers; i++){ // get info from buffer
    memcpy(update_router_buffer, ptr, UPDATE_PACKET_CONTENT_SIZE);
    y_ip = ntohl(update_router->ip_addr);
    y_port = ntohs(update_router->router_port);
    y_id = ntohs(update_router->router_id);
    d_vy_cost = ntohs(update_router->cost);

    for(j=0; j<num_of_routers; j++){  //locate z in routing table by z_id
      if(Routing_table[i].router_id == y_id){
        d_xy_cost = Routing_table[i].cost;
        xy_next_hop_id = Routing_table[i].next_hop_id;
        if(xy_next_hop_id == v_id){ // next_hop is v
          Routing_table[i].cost = c_xv_cost + d_vy_cost;
        }else{  //next-hop is not v
          if(c_xv_cost + d_vy_cost < d_xy_cost){
            Routing_table[i].cost = c_xv_cost + d_vy_cost;
            Routing_table[i].next_hop_id = v_id;
          }
        }
        break;
      }
    }

    if(j >= num_of_routers){
      printf("error, cannot find y_id %u in routing table\n", y_id);
    }

    printf("***inspect element's vaule***\n");
    printf("ip_addr: %u\n", ntohl(update_router->ip_addr));
    printf("router_port: %d\n", ntohs(update_router->router_port));
    printf("router_id: %d\n", ntohs(update_router->router_id));
    printf("cost: %d\n", ntohs(update_router->cost));

    ptr = ptr + INIT_PAYLOAD_CONTENT_SIZE;
  }
  printf("*************out update_routing_table**********\n");
}

int find_id_cost_in_topo(uint16_t *src_id, uint16_t *src_cost, struct sockaddr_in *their_addr){
  int i;
  uint32_t src_ip;
  src_ip = ntohl(their_addr->sin_addr.s_addr);
  for(i=0 ; i<num_of_routers; i++){
    if(Routing_topology[i].ip_addr == src_ip){
      *src_id = Routing_topology[i].router_id;
      *src_cost = Routing_topology[i].cost;
      return 1;
    }
  }
  printf("can't find src_id and src_cost\n");
  return 0;
}

char *make_update_packet(){
  printf("**********in *make_update_packet()*********\n");
  uint16_t content_length, packet_length;
  char *update_packet_header, *update_packet_element, *update_packet_content;
  char *update_packet;
  int i, loc;

  content_length = num_of_routers * UPDATE_PACKET_CONTENT_SIZE;
  update_packet_content = (char *) malloc(content_length);

  for(i = 0; i < num_of_routers ; i++){
    update_packet_element = make_update_packet_element(Routing_table[i]);
    loc = UPDATE_PACKET_CONTENT_SIZE * i;
    memcpy(update_packet_content + loc, update_packet_element, UPDATE_PACKET_CONTENT_SIZE);
  }
  free(update_packet_element);

  update_packet_header = make_update_packet_head();
  packet_length = UPDATE_PACKET_HEAD_SIZE + content_length;
  update_packet = (char *) malloc(packet_length);
  // copy header
  memcpy(update_packet, update_packet_header, UPDATE_PACKET_HEAD_SIZE);
  free(update_packet_header);
  // copy content
  memcpy(update_packet+UPDATE_PACKET_HEAD_SIZE, update_packet_content, content_length);
  free(update_packet_content);

  // we may use sendALL directly, do research about udp sender first. then come back
  printf("**********out *make_update_packet()*********\n");
  return update_packet;
}

char *make_update_packet_element(struct ROUTING_TABLE element){
  char *buffer;
  BUILD_BUG_ON(sizeof(struct UPDATE_PACKET_CONTENT) != UPDATE_PACKET_CONTENT_SIZE);

  struct UPDATE_PACKET_CONTENT *update_packet_element;
  buffer = (char *) malloc(sizeof(char)*UPDATE_PACKET_CONTENT_SIZE);

  update_packet_element = (struct UPDATE_PACKET_CONTENT *) buffer;

  update_packet_element->ip_addr = htonl(element.ip_addr);
  update_packet_element->router_port = htons(element.router_port);
  update_packet_element->padding = htons(0);
  update_packet_element->router_id = htons(element.router_id);
  update_packet_element->cost = htons(element.cost);

  return buffer;
}

char *make_update_packet_head(){
  printf("************* in make_update_packet_head***\n");
  char *buffer;
  BUILD_BUG_ON(sizeof(struct UPDATE_PACKET_HEAD) != UPDATE_PACKET_HEAD_SIZE); 
  struct UPDATE_PACKET_HEAD *update_packet_header;

  buffer = (char *) malloc(sizeof(char)*UPDATE_PACKET_HEAD_SIZE);
  update_packet_header = (struct UPDATE_PACKET_HEAD *) buffer;
  
  update_packet_header->num_of_update_fields = htons(num_of_routers);
  update_packet_header->src_router_port = htons(my_port);
  update_packet_header->src_router_ip = htonl(my_ip);

  printf("update_packet_header->num_of_update_fields %d\n", ntohs(update_packet_header->num_of_update_fields) );
  printf("update_packet_header->src_router_port %d\n", ntohs(update_packet_header->src_router_port) );
  printf("buffer %s strlen() %lu\n", buffer, strlen(buffer));
  printf("sizeof %lu\n", sizeof(buffer));
  printf("************* out make_update_packet_head***\n");

  return buffer;
}

////////////////End function for routing table////////////////////
////////////////function for timer////////////////////////////////

struct ROUTER_TIMEOUT *make_timeout_node(struct ROUTING_TABLE Routing_table_entry ){
  struct ROUTER_TIMEOUT *timeout_node;
  timeout_node = (struct ROUTER_TIMEOUT *)malloc(sizeof(struct ROUTER_TIMEOUT));
  timeout_node->router_id = Routing_table_entry.router_id;
  timeout_node->count = 0;
  timeout_node->recv = 0;

  bzero(&timeout_node->router_addr, sizeof(timeout_node->router_addr));
  timeout_node->router_addr.sin_family = AF_INET;
  timeout_node->router_addr.sin_port = htons(Routing_table_entry.router_port);
  timeout_node->router_addr.sin_addr.s_addr = htonl(Routing_table_entry.ip_addr);
  timeout_node->next = NULL;
  return timeout_node;
}
// init linklist
void initTimeOutList(){
  printf("****************into initTimeOutList************\n");
	RT_head = (struct ROUTER_TIMEOUT *) malloc(sizeof(struct ROUTER_TIMEOUT));
	RT_head->next = NULL;	
	int i;
	for(i=0 ; i < num_of_routers; i++){
		if(Routing_table[i].cost < INF){  // is neighbor or myself
			if(Routing_table[i].router_id != my_id){  //neighbor
				// !** be careful, if the linklist is empty in future, we need to check this.
				RT_node = *make_timeout_node(Routing_table[i]);
				enQueue(RT_head, RT_node);
				printf("append neighbor to the rear of list\n");
			}else{		//myself
				RT_node = *make_timeout_node(Routing_table[i]);				
				// calculate timer
				gettimeofday(&RT_node.timeout,NULL);
				RT_node.timeout.tv_sec += updates_periodic_interval;
				// myself's recv must be 1
				RT_node.recv = 1;
				// insert to the head of linklist
				enQueue2Head(RT_head, RT_node);
				printf("insert myself into list head\n");
			}
		}
	}
  printf("****************out initTimeOutList************\n");
}

uint32_t update_neighbor_node(char *buffer){
  printf("********in update_neighbor_node***************\n");
  struct UPDATE_PACKET_HEAD *update_packet_header;
  uint16_t src_fields;
  uint16_t src_port;
  struct in_addr addr;
  struct ROUTER_TIMEOUT *temp;


  update_packet_header = (struct UPDATE_PACKET_HEAD *) buffer;
  src_fields = ntohs(update_packet_header->num_of_update_fields);
  src_port = ntohs(update_packet_header->src_router_port);
  addr.s_addr = update_packet_header->src_router_ip;

  temp = deQueue_router_ip(addr.s_addr);
  if(temp == NULL){
    return 0;
  }
  gettimeofday(&temp->timeout, NULL);
  temp->timeout.tv_sec += updates_periodic_interval;
  temp->count = 0;
  temp->recv = 1;
  temp->next = NULL;

  enQueueByRecv(RT_head, temp);

  printf("src_field: %d\n", src_fields);
  printf("src_port: %d\n", src_port);
  printf("src_ip: %s\n", inet_ntoa(addr));
  printf("*********out update_neighbor_node***************\n");
  return addr.s_addr; // network style
}

int enQueueByRecv(struct ROUTER_TIMEOUT *RT_head, struct ROUTER_TIMEOUT *element){

  struct ROUTER_TIMEOUT *p, *q;
  p = RT_head;
  q = RT_head->next;
  while(q && q->recv == 1){
    p=p->next;
    q=q->next;
  }
  element->next = p->next;
  p->next = element;
  return 1;
}

void printTimeOutList(){
  struct ROUTER_TIMEOUT *q;
  q = RT_head->next;
  printf("********check timeoutlist elements***************\n");
  while(q){
    printf("router_id: %d\n", q->router_id);
    printf("count: %d\n", q->count);
    if(q->recv == 1){
      printf("recv: %d\n", q->recv);
      printf("tv_sec%ld tv_usec%d\n", q->timeout.tv_sec, q->timeout.tv_usec);
    }else{
      printf("recv: %d\n",q->recv);
    }
    printf("\n\n");
    q = q->next;
  }
  printf("********End check timeoutlist elements**************\n");
}

int list_initialized()
{
  struct ROUTER_TIMEOUT *q;
  if(RT_head == NULL){
    return 0;
  }else{
    q = RT_head->next;  // get head
    if(q == NULL){
      printf("Error! List at least includes itself\n");
      return 0;
    }else{
      return 1;
    }
  }
}

int is_myself(){
  struct ROUTER_TIMEOUT *q;
  q = RT_head->next;
  if(q->router_id == my_id){
    return 1;
  }else{
    return 0;
  }
}

int get_timer(struct timeval *tv){
  struct ROUTER_TIMEOUT *q;
  struct timeval current_time;
  long int usec_temp;
  //
  if(RT_head == NULL){  // List not been init yet
    return 0;
  }else{
    q = RT_head->next;  // get head
    if(q == NULL){   // error
      printf("Error! list at least include itself\n");
      return 0;
    }else{
      gettimeofday(&current_time,NULL);
      usec_temp = q->timeout.tv_usec - current_time.tv_usec;
      if(usec_temp <0){
        tv->tv_usec = 1000000 + usec_temp;
        q->timeout.tv_sec = q->timeout.tv_sec - 1;
      }else{
        tv->tv_usec = usec_temp;
      }
      tv->tv_sec = q->timeout.tv_sec - current_time.tv_sec;
      if(tv->tv_sec < 0){
        printf("Has timeout for a while. call timeout immediately\n");
        tv->tv_sec = 0;
        tv->tv_usec = 0;
      }
      return 1;
    }   
  }
}

int enQueue2Head(struct ROUTER_TIMEOUT *RT_head, struct ROUTER_TIMEOUT element){
	struct ROUTER_TIMEOUT *temp = (struct ROUTER_TIMEOUT*)malloc(sizeof(struct ROUTER_TIMEOUT));
  	*temp = element;
  	temp->next = NULL;

  	struct ROUTER_TIMEOUT *p;
  	p = RT_head;
  	temp->next = p->next;
  	p->next = temp;

  	return 1;
}


int enQueue(struct ROUTER_TIMEOUT *RT_head, struct ROUTER_TIMEOUT element){
  struct ROUTER_TIMEOUT *temp = (struct ROUTER_TIMEOUT*)malloc(sizeof(struct ROUTER_TIMEOUT));
  *temp = element;
  temp->next = NULL;

  struct ROUTER_TIMEOUT *p, *q;
  p = RT_head;
  q = RT_head->next;
  while(q){
    p=p->next;
    q=q->next;
  }
  temp->next = p->next;
  p->next = temp;
  return 1;
}

int queueLength(struct ROUTER_TIMEOUT *RT_head){
  struct ROUTER_TIMEOUT *p;
  p = RT_head->next;
  int i;
  while(p){
    p = p->next;
    i++;
  }
  return i;
}


struct ROUTER_TIMEOUT *deQueue_router_id(struct ROUTER_TIMEOUT *RT_head, uint16_t router_id){
  struct ROUTER_TIMEOUT *p, *q;
  p = RT_head;
  q = RT_head->next;

  while(q && q->router_id != router_id){
    p = p->next;
    q = q->next;
  }
  if(q == NULL){
    printf("Do not have router_id equal to %d\n", router_id);
    return q;
  }else{
    p->next = q->next;
    q->next = NULL;
    return q;
  }
}

struct ROUTER_TIMEOUT *deQueue_router_ip(uint32_t router_ip){
  struct ROUTER_TIMEOUT *p, *q;
  p = RT_head;
  q = RT_head->next;

  while(q && q->router_addr.sin_addr.s_addr != router_ip){
    p = p->next;
    q = q->next;
  }
  if(q == NULL){
    printf("Do not have router_id equal to %u\n", router_ip);
    return q;
  }else{
    p->next = q->next;
    q->next = NULL;
    return q;
  }
}

struct ROUTER_TIMEOUT *deQueue(struct ROUTER_TIMEOUT *RT_head){
  struct ROUTER_TIMEOUT *p, *q;
  p = RT_head;
  q = RT_head->next;

  if(q == NULL){
    printf("Linklist is empty, nothing to delete\n");
    return q;
  }
  p->next = q->next;
  q->next = NULL;
  return q;
}

int rotate_head(){
  struct ROUTER_TIMEOUT *temp;
  temp = deQueue(RT_head);

  if(temp != NULL){
    if(temp->router_id == my_id){

      gettimeofday(&temp->timeout,NULL);
      temp->timeout.tv_sec += updates_periodic_interval;
      temp->next = NULL;
      enQueueByRecv(RT_head, temp);
    }else{  // is neighbor
      temp->count += 1;
      printf("neighbor %d timeout count: %d\n",temp->router_id, temp->count);
      if(temp->count < 3){
        gettimeofday(&temp->timeout,NULL);
        temp->timeout.tv_sec += updates_periodic_interval;
        temp->next = NULL;
        enQueueByRecv(RT_head, temp);
      }else{  // exceed three consecutive update intervals                
        // set temp->router_id topology to be INF     
        controller_update_topology(temp->router_id, INF);
        // update routing table    
        controller_update_routing_table(temp->router_id, INF);
        printf("neighbor %d been removed\n", temp->router_id);
        free(temp);
        printf("*******End removed*******\n");
      }
    }
    return 1;
  }else{
    printf("rotate_head ERROR!\n");
    return 0;
  }

}

int first_node_count(){
  struct ROUTER_TIMEOUT *q;
  q = RT_head->next;
  return q->count;
}






