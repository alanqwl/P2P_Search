# Peer-to-Peer Search

## Overview
This is an implementation of a basic simulation of a peer-to-peer (P2P) file search protocol, akin to the GNUtella protocol. 

Helper functions are implemented in the helper function files: **`utils.c`**, **`utils.h`**, **`network.c`**, **`network.h`**, **`sender.c`**, and **`sender.h`**, with **`network.c`** offering UDP communication APIs. The operational procedure is implemented in **`main.c`** and **`server.c`**.

For **`main.c`**:

1. Parse the command-line arguments, record neighbor information, and send CONNECT requests to all neighbors to establish connections.
2. Handle server operations (you may consider using a mutex to safely share the neighbor structure between threads).
3. Make sure the query ID List and Neighbor List data structures are maintained.

For **`server.c`**:

1. Init a server that Loops and receive packets.
2. Read the packet and determine the packet type (there are three types: CONNECT, QUERY, and RESPONSE).
3. If the packet is a CONNECT type, establish the connection between the server and the packet sender; add the packet sender as a neighbor.
4. If the packet is a QUERY type:
a. Check if the query packet has a duplicated ID based on its ID.
b. Search the target directory for the queried file.
c. If the queried file is found, sends a response back to the server that initialized the query.
d. If file is not found and TTL (Time To Live) is greater than 0, flood the query to all neighbors except the sender.
e. The search fails silently if there are no search results from any reachable server, and the host may receive multiple responses if the query is flooded to multiple server. (you may consider sharing the query ID structure between threads for this purpose).
5. If the packet is a RESPONSE type, print out the search response.

## Objectives

These are the goals achieved in this project.

1. **Basic Query Flooding**
    
    Confirm that a query packet is distributed to all neighboring servers.
    
    * Procedure:
        1. Launch multiple server instances on different terminals, each with a unique port and directory.
        2. Initiate a query request from one server.
        3. Make sure that all other servers receive the query packet.
    * Expected Outcome: Each server receives the query packet.
2. **Loop Avoidance**
    
    Ensure the system prevents endless looping of query packets in a network with loops.
    
    * Procedure:
        1. Set up a network with at least one loop.
        2. Issue a query request from a server within the loop.
        3. Monitor query packet flow to ensure no server receives the same packet more than once (you may maintain an ID list; each time a query is issued, it is identified as a unique ID).
    * Expected Outcome: Query packets do not loop indefinitely; servers receive the query only once.
3. **TTL Control**
    
    Verify that the Time-to-Live (TTL) mechanism restricts query packet propagation.
    
    - Procedure:
        1. Assign a low TTL value (e.g., 1) to query packets.
        2. Send a query from one server.
        3. Check that the query packet is not forwarded beyond immediate neighbors as the TTL expires.
    - Expected ****Outcome: Query packets do not extend beyond their TTL scope, preventing excess network traffic and loop risks.

## Testing Environment Setup

The test environment consists of multiple directories, each representing a server with a unique file. Use multiple terminals to run the compiled "p2p_server" binary with necessary arguments to simulate the distributed network.

```
.
├── Makefile
├── dir1
│   └── file1
├── dir2
│   └── file2
├── dir3
│   └── file3
├── dir4
│   └── file4
├── dir5
│   └── file5
├── main.c
├── network.c
├── network.h
├── server.c
├── server.h
├── utils.c
└── utils.h
```

**Usage**:

```
$ ./p2p_server [PORT_NUM] [TTL] [DIR] [NEIGHBOR_HOST, ...]  

```

### Three-Party Example

***Test*: *Flooding query and TTL Control***

(Bob being a neighbor to Alice, Carol being a neighbor to Bob)

```
// Initialize Alice as the boostrap server
(terminal Alice) $ ./p2p_server 49151 1 dir1

// Bob being a neighbor connected to Alice
(terminal Bob)   $ ./p2p_server 49152 2 dir2 127.0.0.1 49151 

// Carol being a neighbor connected to Bob
(terminal Carol) $ ./p2p_server 49153 2 dir2 127.0.0.1 49152  
```  

Now Carol searches for ‘file1’ in terminal, then log:

```
file1
[CONNECT] Socket created successfully
[CONNECT] Packet sent. size: 140
[FILE] Enter the File Name for Search:
Message received, size: 140
RESPONSE packet

[FILE] File file1 found on 127.0.0.1, with port: 49151

```

Bob would forward packet to Alice since TTL is still alive. Alice log:

```
[CONNECT] Packet sent size: 140
[INFO] Received new packet: QUERY packet
[RESPON] File file1 found, responding to query.
```

Now if Alice searches for file3, it logs:

```
file3
[CONNECT] Packet sent size: 140

```

Bob would not forward the packet to Carol address due to the initial TTL being 1. As a result, the packet will expire at Bob. This proves the file flooding query and TTL control.  



***Test: Avoid Loops***

(Bob being a neighbor to Alice, Carol being a neighbor to Bob and Carol being a neighbor to Alice.)

Now all three parties are connected with each other as neighbors. 

```
// Initialize Alice as the boostrap server
(terminal Alice) $ ./p2p_server 49151 2 dir1

// Bob being a neighbor connected to Alice
(terminal Bob)   $ ./p2p_server 49152 2 dir2 127.0.0.1 49151 

// Carol being a neighbor connected to Bob
(terminal Carol) $ ./p2p_server 49153 2 dir2 127.0.0.1 49152 127.0.0.1 49151
```

If Alice now queries for ‘file3’, the query will be forwarded to both Bob and Carol. For example, Alice searches for 'file3', Bob does not find it, and would again forward to Carol:

```
[FILE] Enter the File Name for Search: 
file3
[CONNECT] Packet sent size: 140
[INFO] Received new packet: QUERY packet
[INFO] File Not found, Forward to next neighbor: 49152
```

But Carol already handled this query from Alice, and would not respond to the same query again. Therefore it would simply ignore:

```
[INFO] Received new packet: QUERY packet
[CONNECT] Packet sent size: 140
[INFO] File file3 found, responding to query.
Message received, size: 140
[INFO] Received new packet: QUERY packet
[DUPLICATE ID] Duplicate packet with ID 1165118814 detected. Ignoring and discarding.
```

Thus proving the avoid loop functionality.
