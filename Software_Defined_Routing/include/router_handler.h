#ifndef ROUTER_HANDLER_H_
#define ROUTER_HANDLER_H_

int create_router_sock();
int creat_router_client_socke();
void routing_update(int router_client_socket);
int router_recv_hook(int sock);

#endif