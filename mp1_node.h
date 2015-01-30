/**********************************************************
*
* Progam Name: MP1. Membership Protocol.
* 
* Code author: Manish Kumar Pandey (mpandey2@illinois.edu)
*
* Current file: mp2_node.h
* About this file: Header file.
*
***********************************************************/

#ifndef _NODE_H_
#define _NODE_H_

#include "stdincludes.h"
#include "params.h"
#include "queue.h"
#include "requests.h"
#include "emulnet.h"

/* Configuration Parameters */
char JOINADDR[30];                    /* address for introduction into the group. */
extern char *DEF_SERVADDR;            /* server address. */
extern short PORTNUM;                /* standard portnum of server to contact. */

/* Miscellaneous Parameters */
extern char *STDSTRING;

/* Node Membership structure template. */
typedef struct nodedetail{
    address addr;                    // node's address
    int heartbeat;                   // node's heartbeat
    int localtime;                   // node's local time
    int flag;                        // flag to test node's failure
} nodedetail;

/* Node structure for node details and node size */
typedef struct NodeMembershipList {
    struct nodedetail *detail;       // A pointer member of type nodedetail
    int nodesize;                    // Node Size
} NodeMemberList;

typedef struct member{            
        struct address addr;            // my address
        int inited;                     // boolean indicating if this member is up
        int ingroup;                    // boolean indicating if this member is in the group
        queue inmsgq;                   // queue for incoming messages
        int bfailed;                    // boolean indicating if this member has failed
        NodeMemberList *node_memlist;   // A pointer member of type NodeMemberList
} member;

/* Message types */
/* Meaning of different message types
  JOINREQ - request to join the group
  JOINREP - reply to JOINREQ
  HEARTBEAT - message for heartbeat
*/
enum Msgtypes{
		JOINREQ,			
		JOINREP,
        HEARTBEAT,
		DUMMYLASTMSGTYPE
};

/* Flag Types */
/* Meaning of different flag types
  NOFAIL - the node has not been marked as fail
  FAIL - the node is marked as having failed and is given a chance until its deleted
  CLEANUP - Tcleanup time is passed and the node's entry is deleted from the node membership list
*/
enum Flagtypes{
    NOFAIL,
    FAIL,
    CLEANUP
};

/* Generic message template. */
typedef struct messagehdr{ 	
	enum Msgtypes msgtype;
} messagehdr;


/* Functions in mp2_node.c */

/* Message processing routines. */
STDCLLBKRET Process_joinreq STDCLLBKARGS;
STDCLLBKRET Process_joinrep STDCLLBKARGS;

/*
int recv_callback(void *env, char *data, int size);
int init_thisnode(member *thisnode, address *joinaddr);
*/

/*
Other routines.
*/

void nodestart(member *node, char *servaddrstr, short servport);
void nodeloop(member *node);
int recvloop(member *node);
int finishup_thisnode(member *node);

#endif /* _NODE_H_ */

