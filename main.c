#include "utils.h"
#include "server.h"
#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
pthread_t server_thread_id;


int main(int argc, char **argv) {
    // Read in port number, TTL, directory, and neighbors from arguments
    int portNumber, myPortNumber; // neighbor port number; host port number
    int ttl; // host ttl
    char directory[128]; // host directory

    neighbors_t neighbors;
    pthread_mutex_t lock;

    myPortNumber = atoi(argv[1]);
    ttl = atoi(argv[2]);
    strcpy(directory, argv[3]);

    if (argc < 4) {
        ERROR("ERROR: Insufficient arguments provided. Usage: ./p2p_server [PORT_NUM] [TTL] [DIR] [NEIGHBOR_HOST, ...]\n");
    }

    if (argc >= 4) {
        // Initialize neighbors
        initializeNeighbors(&neighbors);
        int i;
        for (i = 4; i < argc - 1; i += 2) {  // Increment by 2 to process both IP and port at the same time
            unsigned long hostIpAddress = getHostAddr(argv[i]);
            printf("[NEIGHBOR] HostIPaddr: %lu\n", hostIpAddress);
            portNumber = atoi(argv[i+1]);
            if (hostIpAddress < 0) {
                ERROR("ERROR: Could not find host\n");
            }
            // Save to neighbors data structure
            addNeighbor(&neighbors, hostIpAddress, portNumber);
            
            //[DEBUG] printNeighborData(&neighbors);
            //[DEBUG] printf("[NEIGHBOR] Sent connection request to neighbor with address %s:%lu \n", argv[i], portNumber);
        }
        connectToNeighbors(&neighbors, myPortNumber);
    }

    IDlist_t idList;
    serverArg_t serverArg;

    // Add values to server arguments
    serverArg.idList = &idList;
    serverArg.port = myPortNumber;
    serverArg.directory = directory;
    serverArg.lock = &lock;
    serverArg.neighbors = &neighbors;


    /* 
     * TODO: handle server operation that you implements from server.c . 
     *       You may need a thread mutex to manage server operation.
     */

    int err = pthread_create(&server_thread_id, NULL, &server, &serverArg);
    if (err != 0) {
        ERROR("fail to create server thread");
    }

    // Main thread while loop: host sender
    while (1) {

        // Read input from user
        char input[MAX_STRLEN];

        // Input filename for search
        printf("[FILE] Enter the File Name for Search: \n");
        fgets(input, MAX_STRLEN, stdin);
        int len = strlen(input);
        if (input[0] != '\0' && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }
      
        // First search local server directory
        if (findInDirectory(directory, input)) {
            printf("[FILE] Found file '%s' in local directory \n", input);
            continue;
        } else {
            // if the file is not found, go to the neighbours
            neighbor_t neighbor;
            packet_t query_packet;
            query_packet.port = myPortNumber;
            int packet_id = generatePacket(&query_packet, input, QUERY, ttl);

            // forward the same packet to different neighbors
            for (int i = 0; i < neighbors.num_neighbors; ++i) {
                neighbor = neighbors.neighbor_list[i];
                sendToSocket(neighbor.inaddr, neighbor.port, &query_packet, sizeof(query_packet));
            }
        }
        sleep(5);
    }

    return 0;
}