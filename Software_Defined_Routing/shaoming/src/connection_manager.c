/**
 * @connection_manager
 * @author  Shaoming Xu <shaoming@buffalo.edu>
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
 * Connection Manager listens for incoming connections/messages from the
 * controller and other routers and calls the desginated handlers.
 */

#include <sys/select.h>
#include <netinet/in.h>
#include "../include/connection_manager.h"
#include "../include/global.h"
#include "../include/control_handler.h"
#include "../include/router_handler.h"
#include "../include/table.h"
#include "../include/crash.h"

fd_set master_list, watch_list;
int head_fd;

void main_loop()
{
    int selret, sock_index, fdaccept;
    struct timeval tv;
    int first_init;
    first_init = 1;  // 1: true  0:false
    crashed = 0;  // 1 crushed  0 not crushed

    while(TRUE){
        if(crashed == 1){
            break;
        }

        tv.tv_sec = 20;
        tv.tv_usec = 0;
        if(get_timer(&tv) == 1){  // timeout link list has been initialized
            if(first_init == 1){    // first time init.
                router_socket = create_router_sock();
                router_client_socket = creat_router_client_socke();
                FD_SET(router_socket, &master_list);
                if(router_socket > head_fd){
                    head_fd = router_socket;
                } 
                printf("router_socket: %d\n", router_socket);
                printf("router_client_socket: %d\n", router_client_socket);
                first_init = 0;
            }
        }

        printf("tv.tv_sec: %ld\ntv.tv_usec: %d\n", tv.tv_sec, tv.tv_usec);

        watch_list = master_list;
        selret = select(head_fd+1, &watch_list, NULL, NULL, &tv);
        printf("out select\n");
        if(selret < 0){
            ERROR("select failed.");
        }else if(selret == 0){
            printf("timeout\n");
            if(list_initialized()){
                if( is_myself() ){
                    //send packet to all neighbor. 
                    routing_update(router_client_socket);
                    rotate_head();
                }else{
                    if(first_node_count() >= 2){
                        // exceed three consecutive update intervals
                        // set it cost to be INF in table
                        rotate_head();
                    }else{
                        rotate_head();
                    }

                }
                printTimeOutList();
            }
        }else{
            // when some sockets in select is active
            /* Loop through file descriptors to check which ones are ready */
            printf("someone ask me!\n");
            for(sock_index=0; sock_index<=head_fd; sock_index+=1){

                if(FD_ISSET(sock_index, &watch_list)){

                    /* control_socket */
                    if(sock_index == control_socket){
                        printf("control_socket ask me!\n");
                        fdaccept = new_control_conn(sock_index);

                        /* Add to watched socket list */
                        FD_SET(fdaccept, &master_list);
                        if(fdaccept > head_fd) head_fd = fdaccept;
                    }

                    /* router_socket */
                    else if(sock_index == router_socket){
                        printf("router_socket ask me!\n");
                        router_recv_hook(sock_index);
                    }

                    /* data_socket */
                    else if(sock_index == data_socket){
                        printf("data_socket ask me!\n");
                        //new_data_conn(sock_index);
                    }

                    /* Existing connection */
                    else{
                        if(isControl(sock_index)){
                            printf("isControl ask me!\n");
                            if(!control_recv_hook(sock_index)) FD_CLR(sock_index, &master_list);
                        }
                        //else if isData(sock_index);
                        else ERROR("Unknown socket index");
                    }
                }
            }
        }
    }
}

void init()
{
    control_socket = create_control_sock();

    FD_ZERO(&master_list);
    FD_ZERO(&watch_list);

    /* Register the control socket */
    FD_SET(control_socket, &master_list);
    head_fd = control_socket;

    main_loop();
}