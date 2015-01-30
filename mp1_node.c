/**********************************************************
*
* Progam Name: MP1. Membership Protocol
* 
* Code author: Manish Kumar Pandey (mpandey2@illinois.edu)
*
* Current file: mp1_node.c
* About this file: Member Node Implementation
* 
***********************************************************/

#include "mp1_node.h"
#include "emulnet.h"
#include "MPtemplate.h"
#include "log.h"
#include <string.h>

/*
 *
 * Routines for introducer and current time.
 *
 */

char NULLADDR[] = {0,0,0,0,0,0};
int isnulladdr( address *addr){
    return (memcmp(addr, NULLADDR, 6)==0?1:0);
}

/*
 *
 * Constants defined for Tfail and Tcleanup
 *
 */

int FAILURE_TIME = 10;
int CLEANUP_TIME = 20;

/* 
Return the address of the introducer member. 
*/
address getjoinaddr(void){

    address joinaddr;

    memset(&joinaddr, 0, sizeof(address));
    *(int *)(&joinaddr.addr)=1;
    *(short *)(&joinaddr.addr[4])=0;

    return joinaddr;
}

/*
 *
 * Message Processing routines.
 *
 */

/*
 *
 * The function inserts the node details
 *
 */
void InsertNodeDetails(member *node, address *nodeaddress, int member_heartbeat, int node_local_time){
    nodedetail *memdata = (nodedetail *) malloc(sizeof(nodedetail));
    memcpy(&memdata->addr, nodeaddress, sizeof(address));
    memdata->heartbeat = member_heartbeat;
    memdata->localtime = node_local_time;
    memdata->flag = NOFAIL;
    node->node_memlist->detail[node->node_memlist->nodesize++] = *memdata;
    return;
}

/*
 *
 * The function searches the node address in the Node Membership List
 *
 */
int SearchNodeDetail(member *node, address *nodeaddress){
    for (int counter = 0; counter < node->node_memlist->nodesize; counter++){
        nodedetail memdata = *(node->node_memlist->detail + counter);
        if (memcmp(&memdata.addr, nodeaddress, sizeof(address)) == 0)
            return counter;
    }
    return -1;
}

/*
 *
 * The function updates the Node Membership List
 *
 */
void UpdateNodeMemberList(member *node, NodeMemberList *memlist, int current_time){
    int matched_location = -1;
    for (int counter = 0; counter < memlist->nodesize; counter++){
        nodedetail memdata = *(memlist->detail + counter);
        matched_location = SearchNodeDetail(node, &memdata.addr);
        if (matched_location == -1 && memdata.flag == NOFAIL){
            // If its the first time and the node is not found
            // in the node membership list then add this node
            InsertNodeDetails(node, &memdata.addr, memdata.heartbeat, current_time);
            logNodeAdd(&node->addr, &memdata.addr);
        }else{
            // If the node has the greater heartbeat
            // and its non faulty, then update with higher heartbeat
            nodedetail *matched_node_memdata = (node->node_memlist->detail + matched_location);
            if (memdata.heartbeat > matched_node_memdata->heartbeat && memdata.flag == NOFAIL){
                matched_node_memdata->heartbeat = memdata.heartbeat;
                matched_node_memdata->localtime = current_time;
                if (matched_node_memdata->flag == FAIL && memdata.flag == NOFAIL)
                    matched_node_memdata->flag = NOFAIL;
            }
        }
    }
}

/*
 *
 * The function makes a copy of the node details
 *
 */
NodeMemberList* CopyNodeDetails(member *node){
    NodeMemberList *memlist = (NodeMemberList *) malloc(sizeof(NodeMemberList));
    memlist->detail = (nodedetail *) malloc(node->node_memlist->nodesize * sizeof(nodedetail));
    memlist->nodesize = 0;
    for (int counter = 0; counter < node->node_memlist->nodesize; counter++){
        nodedetail memdata = *(node->node_memlist->detail + counter);
        if (memdata.flag == FAIL || memdata.flag == CLEANUP)
            continue;
        nodedetail new_memdata;
        new_memdata.heartbeat = memdata.heartbeat;
        new_memdata.localtime = memdata.localtime;
        new_memdata.flag = memdata.flag;
        memcpy(&new_memdata.addr, &memdata.addr, sizeof(address));

        memlist->detail[memlist->nodesize++] = new_memdata;
    }
    return memlist;
}

/*
 *
 * The function deletes the entry of a node in the Node Membership List based on the Tcleanup time
 *
 */
void DeleteMemberDetails(member *node, int position){
    nodedetail *memdata = node->node_memlist->detail + position;
    if (memdata->flag == CLEANUP)
        return;
    memdata->flag = CLEANUP;
    logNodeRemove(&node->addr, &memdata->addr);
    if (node->node_memlist->nodesize == 1 && position == 1){
        node->node_memlist = NULL;
        node->node_memlist = 0;
        return;
    } else if (position == (node->node_memlist->nodesize - 1)){
        node->node_memlist->nodesize--;
    } else {
        nodedetail memdata = node->node_memlist->detail[position];
        node->node_memlist->detail[position] = node->node_memlist->detail[node->node_memlist->nodesize - 1];
        node->node_memlist->detail[node->node_memlist->nodesize - 1] = memdata;
        node->node_memlist->nodesize--;
    }
}

/* 
Received a JOINREQ (joinrequest) message.
*/
// Purpose: The node's entry is added by the Introducer to the Node Membership List.
//          Also the Introducer sends the JOIN REPLY message to the node who requested
//          the Introducer
void Process_joinreq(void *env, char *data, int size)
{
    member *node = (member *) env;
    address *target_node;
    target_node = (address *) malloc(sizeof(address));
    messagehdr *msghdr;
    address jaddress = getjoinaddr();
    memset(target_node, 0, sizeof(address));
    strcpy(target_node->addr, data);

    if(memcmp(&node->addr, &jaddress, 4 * sizeof(char)) != 0) return;

    if (node->node_memlist == NULL){
        node->node_memlist = (NodeMemberList *) malloc(sizeof(NodeMemberList));
        node->node_memlist->nodesize = 0;
        node->node_memlist->detail = (nodedetail *) malloc(MAX_NNB * sizeof(nodedetail));
        InsertNodeDetails(node, &jaddress, 0, globaltime);
        logNodeAdd(&node->addr, &node->addr);
    }

    InsertNodeDetails(node, target_node, 0, globaltime);

    if (SearchNodeDetail(node, target_node) == -1);
        logNodeAdd(&node->addr, target_node);

    NodeMemberList *copy_memlist = CopyNodeDetails(node);
    size_t message_size = sizeof(messagehdr) + sizeof(NodeMemberList **);
    msghdr=malloc(message_size);
    msghdr->msgtype=JOINREP;
    memcpy((char *)(msghdr + 1), &copy_memlist, sizeof(NodeMemberList **));

    // Introducer sends the JOIN REPLY message to the node
    // who requested the introducer to join

    MPp2psend(&jaddress, target_node, (char *)msghdr, message_size);
    free(msghdr);
}

/* 
Received a JOINREP (joinreply) message. 
*/
// Purpose: The JOIN REPLY message send by the Introducer is recieved by the node
//          The node updates its Node Membership List

void Process_joinrep(void *env, char *data, int size)
{
    member *node = (member *) env;

    NodeMemberList **reply_memlist = (NodeMemberList **) data;
    NodeMemberList *copy_memlist = *reply_memlist;

    if (node->node_memlist == NULL){
        node->node_memlist = (NodeMemberList *) malloc(sizeof(NodeMemberList));
        node->node_memlist->nodesize = 0;
        node->node_memlist->detail = (nodedetail *) malloc(MAX_NNB * sizeof(nodedetail));
    }

    UpdateNodeMemberList(node, copy_memlist, globaltime);

    node->ingroup = 1;
    return;
}

/*
 *
 * The function to process gossip protocol messages
 *
 */
void Process_gossip_style_heartbeating(void *env, char *data, int size){

    member *node = (member *) env;
    // Check if the node is in the group or not
    // If the node is not in the group, disregard the gossip protocol request
    if (node->ingroup == 0){
        printf("This Node is not in the group so cannot process gossip protocol for this node\n");
        return;
    }

    NodeMemberList **reply_memlist = (NodeMemberList **) data;
    NodeMemberList *copy_memlist = *reply_memlist;

    if (node->node_memlist == NULL){
        node->node_memlist = (NodeMemberList *) malloc(sizeof(NodeMemberList));
        node->node_memlist->nodesize = 0;
        node->node_memlist->detail = (nodedetail *) malloc(MAX_NNB * sizeof(nodedetail));
    }

    UpdateNodeMemberList(node, copy_memlist, globaltime);

    return;
}
/* 
Array of Message handlers. 
*/
void ( ( * MsgHandler [20] ) STDCLLBKARGS )={
/* Message processing operations at the P2P layer. */
    Process_joinreq, 
    Process_joinrep,
    Process_gossip_style_heartbeating
};

/* 
Called from nodeloop() on each received packet dequeue()-ed from node->inmsgq. 
Parse the packet, extract information and process. 
env is member *node, data is 'messagehdr'. 
*/
int recv_callback(void *env, char *data, int size){

    member *node = (member *) env;
    messagehdr *msghdr = (messagehdr *)data;
    char *pktdata = (char *)(msghdr+1);

    if(size < sizeof(messagehdr)){
#ifdef DEBUGLOG
        LOG(&((member *)env)->addr, "Faulty packet received - ignoring");
#endif
        return -1;
    }

#ifdef DEBUGLOG
    LOG(&((member *)env)->addr, "Received msg type %d with %d B payload", msghdr->msgtype, size - sizeof(messagehdr));
#endif

    if((node->ingroup && msghdr->msgtype >= 0 && msghdr->msgtype <= DUMMYLASTMSGTYPE)
        || (!node->ingroup && msghdr->msgtype==JOINREP))            
            /* if not yet in group, accept only JOINREPs */
        MsgHandler[msghdr->msgtype](env, pktdata, size-sizeof(messagehdr));
    /* else ignore (garbled message) */
    free(data);

    return 0;

}

/*
 *
 * Initialization and cleanup routines.
 *
 */

/* 
Find out who I am, and start up. 
*/
int init_thisnode(member *thisnode, address *joinaddr){
    
    if(MPinit(&thisnode->addr, PORTNUM, (char *)joinaddr)== NULL){ /* Calls ENInit */
#ifdef DEBUGLOG
        LOG(&thisnode->addr, "MPInit failed");
#endif
        exit(1);
    }
#ifdef DEBUGLOG
    else LOG(&thisnode->addr, "MPInit succeeded. Hello.");
#endif

    thisnode->bfailed=0;
    thisnode->inited=1;
    thisnode->ingroup=0;
    /* node is up! */

    return 0;
}


/* 
Clean up this node. 
*/
int finishup_thisnode(member *node){
// Freed the pointer to Node Membership List
    free(node->node_memlist);
    return 0;
}


/* 
 *
 * Main code for a node 
 *
 */

/* 
Introduce self to group. 
*/
int introduceselftogroup(member *node, address *joinaddr){
    
    messagehdr *msg;
#ifdef DEBUGLOG
    static char s[1024];
#endif

    if(memcmp(&node->addr, joinaddr, 4*sizeof(char)) == 0){
        /* I am the group booter (first process to join the group). Boot up the group. */
#ifdef DEBUGLOG
        LOG(&node->addr, "Starting up group...");
#endif

        node->ingroup = 1;
    }
    else{
        size_t msgsize = sizeof(messagehdr) + sizeof(address);
        msg=malloc(msgsize);

    /* create JOINREQ message: format of data is {struct address myaddr} */
        msg->msgtype=JOINREQ;
        memcpy((char *)(msg+1), &node->addr, sizeof(address));

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        LOG(&node->addr, s);
#endif

    /* send JOINREQ message to introducer member. */
        MPp2psend(&node->addr, joinaddr, (char *)msg, msgsize);
        
        free(msg);
    }

    return 1;

}

/* 
Called from nodeloop(). 
*/
void checkmsgs(member *node){
    void *data;
    int size;

    /* Dequeue waiting messages from node->inmsgq and process them. */
	
    while((data = dequeue(&node->inmsgq, &size)) != NULL) {
        recv_callback((void *)node, data, size); 
    }
    return;
}


/* 
Executed periodically for each member. 
Performs necessary periodic operations. 
Called by nodeloop(). 
*/
void nodeloopops(member *node){

    if (node->node_memlist == NULL || node->ingroup == 0)
        return;

    /* Pick a random node and process gossip protocol */

    messagehdr *msghdr;

    int node_position = SearchNodeDetail(node, &node->addr);
    node->node_memlist->detail[node_position].heartbeat++;
    node->node_memlist->detail[node_position].localtime = globaltime;

    int random_node = rand() % node->node_memlist->nodesize;
    address random_node_address = node->node_memlist->detail[random_node].addr;

    NodeMemberList *copy_memlist = CopyNodeDetails(node);

    size_t message_size = sizeof(messagehdr) + sizeof(NodeMemberList **);

    msghdr = malloc(message_size);

    msghdr->msgtype = HEARTBEAT;
    memcpy((char *)(msghdr + 1), &copy_memlist, sizeof(NodeMemberList **));

    /* Picked up a random node and sending the membership list message to that node */

    MPp2psend(&node->addr, &random_node_address, (char *)msghdr, message_size);
    free(msghdr);

    /* If globaltime minus sum of Tfail and Tcleanup exceeds the localtime
     of the node then delete the node's entry in the node membership list  */

    for (int counter = 0; counter < node->node_memlist->nodesize; counter++) {
        nodedetail *memdata = node->node_memlist->detail + counter;
        if (globaltime - FAILURE_TIME - CLEANUP_TIME > memdata->localtime) {
            DeleteMemberDetails(node, counter);
        } else if (globaltime - FAILURE_TIME > memdata->localtime) {
            memdata->flag = FAIL;
        }
    }

    return;
}

/* 
Executed periodically at each member. Called from app.c.
*/
void nodeloop(member *node){
    if (node->bfailed) return;

    checkmsgs(node);

    /* Wait until you're in the group... */
    if(!node->ingroup) return ;

    /* ...then jump in and share your responsibilites! */
    nodeloopops(node);
    
    return;
}

/* 
All initialization routines for a member. Called by app.c. 
*/
void nodestart(member *node, char *servaddrstr, short servport){

    address joinaddr=getjoinaddr();

    /* Self booting routines */
    if(init_thisnode(node, &joinaddr) == -1){

#ifdef DEBUGLOG
        LOG(&node->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if(!introduceselftogroup(node, &joinaddr)){
        finishup_thisnode(node);
#ifdef DEBUGLOG
        LOG(&node->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/* 
Enqueue a message (buff) onto the queue env. 
*/
int enqueue_wrppr(void *env, char *buff, int size){    return enqueue((queue *)env, buff, size);}

/* 
Called by a member to receive messages currently waiting for it. 
*/
int recvloop(member *node){
    if (node->bfailed) return -1;
    else return MPrecv(&(node->addr), enqueue_wrppr, NULL, 1, &node->inmsgq); 
    /* Fourth parameter specifies number of times to 'loop'. */
}

