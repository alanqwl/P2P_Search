#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "network.h"
#include "utils.h"
#include "server.h"
#include <stdio.h>
#include <dirent.h>
#include <pthread.h>


void *server(void *arg){
  serverArg_t *_arg = (serverArg_t *)arg;
  IDlist_t *idList = _arg->idList;
  int portNum = _arg->port;
  char *directory = _arg->directory;
  neighbors_t *neighbors = _arg->neighbors;
  pthread_mutex_t *mutex = _arg->lock;

  int socketfd = createNewSock();
  packet_t packet;
  unsigned long clientIpAddress;

  if (bindSocket(socketfd, portNum) == -1){
    ERROR("Error: wrong port binding. \n");
  }
  printf("[CONNECT] Successfully bind with the port: %d\n", portNum);

  while (1) {
    clientIpAddress = recvFromSocket(socketfd, &packet, sizeof(packet));
    unsigned char type = packet.descriptor;
    printf("[INFO] Received new packet: ");

    switch(type){
    case 0: printf("CONNECT packet\n"); break;
    case 1: printf("QUERY packet\n"); break;
    case 2: printf("RESPONSE packet\n"); break;
    }

    if (type == CONNECT){
      //TODO: handle server connect
      serverHandleConnect(neighbors, &packet, mutex);
    }
    else if (type == QUERY){
      //TODO: handle server query
      serverQuery(_arg, &packet);
    }
    else if (type == RESPONSE){
      //TODO: handle server response
      serverHandleResponse(&packet);
    }
  }
}

/*
 * You may implement the following functions to deal with TODOs. 
 */
int serverHandleConnect(neighbors_t *neighbors, packet_t *packet, pthread_mutex_t *mutex){
  //connect host to neighbors
  //update neighbor list
  unsigned long hostIPAddress = getHostAddr(packet->hostname);
  pthread_mutex_lock(mutex);
  addNeighbor(neighbors, hostIPAddress, packet->port);
  pthread_mutex_unlock(mutex);
  return 0;
}


int serverQuery(serverArg_t *args, packet_t *packet) {
  // if found in ID list
  if (findInList(args->idList, packet->ID)){
    printf("[DUPLICATE ID] Duplicate packet with ID %d detected. Ignoring and discarding.\n", packet->ID);
    return 0;
  }
  
  // if file found
  if (findInDirectory(args->directory, packet->message)){
    printf("[RESPON] File %s found, responding to query.\n", packet->message);

    // response to the query packet
    packet_t response_packet;
    response_packet.port = args->port;
    unsigned long host_address = getHostAddr(packet->hostname);
    generatePacket(&response_packet, packet->message, RESPONSE, packet->TTL);
    sendToSocket(host_address, packet->port, &response_packet, sizeof(response_packet));
    addToDataList(args->idList, packet->ID);              // record packet id
  } else {
    // if file not found
    if (packet->TTL > 0) {
      // if ttl > 0, continue to forward the query packet
      // if ttl <= 0, packet expires.
      packet->TTL -= 1;
      char* neighbor_addr;
      int port_num;
      neighbors_t forward_neighbors;
      initializeNeighbors(&forward_neighbors);
      for (int i = 0; i < args->neighbors->num_neighbors; ++i) {
        if (args->neighbors->neighbor_list[i].port == packet->port) {
          continue;
        }
        neighbor_addr = getHostIp(args->neighbors->neighbor_list[i].inaddr);
        port_num = args->neighbors->neighbor_list[i].port;    
        printf("[INFO] File Not found, Forward to next neighbor: %s with port %d\n", neighbor_addr, port_num);
        addNeighbor(&forward_neighbors, args->neighbors->neighbor_list[i].inaddr, port_num);
      }
      // else not found, flood to neighbors
      floodRequest(&forward_neighbors, args->port, packet, sizeof(*packet));
    }
  }
  return 0;
}



int serverHandleResponse(packet_t *packet){

  printf("\n[FILE] File %s found on address %s, with port %d\n\n", 
         packet->message,  
         getHostIp(getHostAddr(packet->hostname)),
          packet->port);

  return 0;
}
