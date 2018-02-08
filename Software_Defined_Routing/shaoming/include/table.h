#ifndef TABLE_H_
#define TABLE_H_

#define INIT_PAYLOAD_HEAD_SIZE 4
#define INIT_PAYLOAD_CONTENT_SIZE 12
#define ROUTER_NUMBER 5
#define INF 65535
#define UPDATE_PACKET_HEAD_SIZE 8
#define UPDATE_PACKET_CONTENT_SIZE 12
#define ROUTING_TABLE_RESPONSE_ELEMENT_SIZE 8
#define CONTROLLER_UPDATE_CONTENT_SIZE 4

struct __attribute__((__packed__)) ROUTING_TABLE
{
  uint16_t router_id;
  uint16_t router_port;
  uint16_t data_port;
  uint16_t cost;
  uint32_t ip_addr;
  uint16_t next_hop_id;
};

struct __attribute__((__packed__)) INIT_PAYLOAD_HEAD
{
  uint16_t num_of_routers;
  uint16_t updates_periodic_interval;	
};

struct __attribute__((__packed__)) INIT_PAYLOAD_CONTENT
{
  uint16_t router_id;
  uint16_t router_port;
  uint16_t data_port;
  uint16_t cost;
  uint32_t ip_addr;
};

struct __attribute__((__packed__)) UPDATE_PACKET_HEAD
{
  uint16_t num_of_update_fields;
  uint16_t src_router_port;
  uint32_t src_router_ip;
};

struct __attribute__((__packed__)) UPDATE_PACKET_CONTENT
{
  uint32_t ip_addr;
  uint16_t router_port;
  uint16_t padding;
  uint16_t router_id;
  uint16_t cost;
};

struct __attribute__((__packed__)) ROUTING_TABLE_RESPONSE_ELEMENT
{
  uint16_t router_id;
  uint16_t padding;
  uint16_t next_hop_id;
  uint16_t cost;
};

struct __attribute__((__packed__)) CONTROLLER_UPDATE_CONTENT
{
  uint16_t router_id;
  uint16_t cost;
};


// this struct for timer
struct ROUTER_TIMEOUT
{
  uint16_t router_id;
  struct timeval timeout;
  int count;
  int recv; 
  //0: never get router_id udp meesage 
  //1: has get router_id udp meesage 
  struct sockaddr_in router_addr;
  struct ROUTER_TIMEOUT *next;
};
/////////////////////////////////////////////////////

/////////////////////////////////////////////////////
//for table
struct ROUTING_TABLE Routing_table[ROUTER_NUMBER];
struct ROUTING_TABLE Routing_topology[ROUTER_NUMBER];

uint16_t num_of_routers;
uint16_t updates_periodic_interval;
uint16_t my_id;
uint16_t my_port;
uint32_t my_ip;
//for timeout linklist
struct ROUTER_TIMEOUT *RT_head;
struct ROUTER_TIMEOUT RT_node;

////////////////////////////////////////////////////
void update_response(int sock_index);
void update_topology_table(char *cntrl_payload);
int32_t controller_update_topology(uint16_t update_id, uint16_t update_cost);
void controller_update_routing_table(uint16_t update_id, int32_t difference);


void routing_table_response(int sock_index);
int init_routing_table(char *cntrl_payload);
struct ROUTER_TIMEOUT *make_timeout_node(struct ROUTING_TABLE Routing_table_entry );
int enQueue2Head(struct ROUTER_TIMEOUT *RT_head, struct ROUTER_TIMEOUT element);
int enQueue(struct ROUTER_TIMEOUT *RT_head, struct ROUTER_TIMEOUT element);
char *make_update_packet_head();
char *make_update_packet_element(struct ROUTING_TABLE element);
char *make_update_packet();
void update_routing_table(char *update_packet_content, struct sockaddr_in *their_addr);
int find_id_cost_in_topo(uint16_t *src_id, uint16_t *src_cost, struct sockaddr_in *their_addr);

////////for time linklist
void initTimeOutList();
void printTimeOutList();
int get_timer(struct timeval *tv);
int list_initialized();
int is_myself();
int rotate_head();
int first_node_count();
uint32_t update_neighbor_node(char *update_packet_header);
int enQueueByRecv(struct ROUTER_TIMEOUT *RT_head, struct ROUTER_TIMEOUT *element);
struct ROUTER_TIMEOUT *deQueue_router_id(struct ROUTER_TIMEOUT *RT_head, uint16_t router_id);
struct ROUTER_TIMEOUT *deQueue_router_ip(uint32_t router_ip);

#endif








