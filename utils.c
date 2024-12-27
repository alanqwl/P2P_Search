#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "network.h"
#include "utils.h"
#include "pthread.h"

pthread_mutex_t globalLock = PTHREAD_MUTEX_INITIALIZER; // global mutex (not necessarily used)


int generateIdentifier(){
  srand(time(NULL));
  return rand();
}


int generatePacket(packet_t *dataPacket, char *filename, int type, int TTL){
  char hostName[MAX_STRLEN];

  dataPacket->ID = generateIdentifier();
  dataPacket->TTL = (unsigned char)TTL;
  dataPacket->descriptor = (unsigned char)type;
  
  gethostname(hostName, MAX_STRLEN);

  strcpy(dataPacket->hostname, hostName);

  if (filename != NULL){
    strcpy(dataPacket->message, filename);
  }

  return dataPacket->ID;
}


int initializeList(IDlist_t *dataList){
  dataList->num = 0;
}


int addToDataList(IDlist_t *dataList, int identifier){
  pthread_mutex_lock(&globalLock); // lock

  // check duplicated ID
  for (int i = 0; i < dataList->num; ++i) {
      if (dataList->IDs[i] == identifier) {
          pthread_mutex_unlock(&globalLock); // unlock
          return -1; // ID already exists
      }
  }

  // add new ID to ID list
  int index = dataList->num;
  dataList->IDs[index] = identifier;
  ++dataList->num;

  pthread_mutex_unlock(&globalLock); // unlock

  return dataList->num;
}


int findInList(IDlist_t *dataList, int identifier){
  int num = dataList->num;
  int i;

  for (i = 0; i < num; ++i){
    if (dataList->IDs[i] == identifier){
      return 1;
    }
  }

  return 0;
}


void printList(IDlist_t *idList) {
    printf("[DEBUG] Current ID list: [");
    for (int i = 0; i < idList->num; ++i) {
        printf("%d", idList->IDs[i]);
        if (i != idList->num - 1) {
            printf(", ");
        }
    }
    printf("]\n");
}


int initializeNeighbors(neighbors_t *neighborData){
  neighborData->num_neighbors = 0;
  return 0;
}


int addNeighbor(neighbors_t *neighborData, unsigned long hostIPAddress, int port){
  int index = neighborData->num_neighbors;
  neighbor_t *newNeighbor = &(neighborData->neighbor_list[index]);

  neighborData->num_neighbors = neighborData->num_neighbors + 1;

  if (neighborData->num_neighbors >= MAX_NEIGHBOR){
    ERROR("ERROR: Exceeds maximum neighbor number\n");
    return -1;
  }

  // Assign the IP Address and Port to the new neighbor
  newNeighbor->inaddr = hostIPAddress;
  newNeighbor->port = port;

  return index;
}

// Contact neighbors and send CONNECT packet to all neighbors
int connectToNeighbors(neighbors_t *neighborData, int myPortNumber){
  printf("[DEBUG] Hello? I am trying to connect ... \n");
  int i;
  packet_t dataPacket;
  int num_neighbors = neighborData->num_neighbors;
  neighbor_t *neighbor;

  // connect to all neighbors stored
  for (i = 0; i < num_neighbors; ++i){
    dataPacket.port = myPortNumber;

    // generate packet and returns the packet ID
    generatePacket(&dataPacket, NULL, CONNECT, 0);
               
    // send to the neighbor
    neighbor = &neighborData->neighbor_list[i];
    sendToSocket(neighbor->inaddr, neighbor->port, 
                (void *)&dataPacket, sizeof(dataPacket));

  }
  
  return 0;
}


int isNeighborFound(unsigned long hostIPAddress, int port, neighbors_t *neighborData){
  int i;

  for (i = 0; i < neighborData->num_neighbors; ++i){
    if (neighborData->neighbor_list[i].inaddr == hostIPAddress &&
        neighborData->neighbor_list[i].port == port){
      return 1;
    }
  }

  return 0;
} 


void printNeighborData(neighbors_t *neighborData) {
    printf("[NEIGHBOR] Current neighbors\n");
    printf("[NEIGHBOR] Number of neighbors: %d\n", neighborData->num_neighbors);
    printf("[NEIGHBOR] Neighbor list:\n");
    for (int i = 0; i < neighborData->num_neighbors; ++i) {
        printf("[NEIGHBOR] Neighbor %d: IP %s, Port %d\n", i + 1, 
               getHostIp(neighborData->neighbor_list[i].inaddr),
               neighborData->neighbor_list[i].port);
    }
}


int findInDirectory(char *directory, char *file){
  DIR *dp;
  struct dirent *directoryStruct;
  if ( (dp = opendir(directory)) == NULL ){
    return -1;
  }
  // loop through directory, search for the file
  while ( (directoryStruct = readdir(dp)) != NULL ){
    if ( strcmp(file, directoryStruct->d_name) == 0 ){
      return 1;
    }
  }
  closedir(dp);
  return 0;
}


int floodRequest(neighbors_t *neighbors, int portno, packet_t *packet, int size){
  int i;
  int num_neighbors = neighbors->num_neighbors;
  unsigned long inaddr;
    
  for (i = 0; i < num_neighbors; ++ i){
    inaddr = neighbors->neighbor_list[i].inaddr;

    if (sendToSocket(inaddr, neighbors->neighbor_list[i].port, packet, size) < 0) {
      return -1;
    }
  }
  return 0;
}