#include <stdio.h>
#include "list.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "packet.h"
#include <pthread.h>


list_t *init_list(int sockfd,unsigned long r_timer)
{
    list_t *list = malloc(sizeof(list_t));
    if (list == NULL)
    {
        return NULL;
    }
    list->head = NULL;
    list->last = NULL;
    list->sockfd = sockfd;
    list->r_timer = r_timer;
    pthread_mutex_init(&(list->lock), NULL);
    return list;
}

void printList(list_t *list)
{
    node_t *ptr = list->head;
    printf("\n[ ");
    //start from the beginning
    while (ptr != NULL)
    {
        printf("%u\n", (ptr->pkt.msgid));
        ptr = ptr->next;
    }
    printf(" ]");
}

//insert link at the first location
void insertFirst(pkt_t pkt, list_t *list,struct sockaddr_in6 *addr,unsigned long timer)
{
    pthread_mutex_lock(&(list->lock));
    node_t *head = list->head;
    //create a link
    node_t *link = (node_t *)malloc(sizeof(node_t));

    link->pkt = pkt;

    link-> addr = addr; 

    //point it to old first node
    link->next = head;

    link->timer = timer;

    //point first to new first node
    list->head = link;
    pthread_mutex_unlock(&(list->lock));
}
//is list empty
bool isEmpty(list_t *list)
{
    node_t *head = list->head;
    return head == NULL;
}

int length(list_t *list)
{
    int length = 0;
    node_t *current;

    for (current = list->head; current != NULL; current = current->next)
    {
        length++;
    }

    return length;
}

//find a link with given key
node_t *find(uint8_t msgid, list_t *list)
{

    node_t *head = list->head;

    //start from the first link
    node_t *current = head;

    //if list is empty
    if (head == NULL)
    {
        return NULL;
    }

    //navigate through list
    while (current->pkt.msgid != msgid)
    {

        //if it is last node
        if (current->next == NULL)
        {
            return NULL;
        }
        else
        {
            //go to next link
            current = current->next;
        }
    }
    //if data found, return the current Link
    return current;
}

int reTransmit(list_t *list,unsigned long timer)
{
    node_t *current = list->head;
    //if list is empty
    if (current == NULL)
    {
        return -1;
    }
    //navigate through list
    while (current != NULL)
    {
        // printf("r_timer : %lu\n",list->r_timer);
        // printf("diff : %lu\n",timer-(current -> timer));

        if((timer-(current -> timer)) > list->r_timer){
            
            printf("in timer\n");

            char buf[10];
            pkt_t pkt = current->pkt;
            pkt_encode(&pkt, buf);
            int err = sendto(list->sockfd, buf, 5,
                         MSG_CONFIRM, (const struct sockaddr *)current->addr,
                         sizeof(struct sockaddr_in6));
            if(err < 0){
                perror("send error : ");  
                return -1;
            }
            current->timer = timer;   
        }
        current = current->next;
    }
return 1;
}

int compare_ipv6(struct in6_addr *ipA, struct in6_addr *ipB)
{
    int i = 0;
    for(i = 0; i < 16; ++i)
    {
        if (ipA->s6_addr[i] < ipB->s6_addr[i])
            return -1;
        else if (ipA->s6_addr[i] > ipB->s6_addr[i])
            return 1;
    }
    return 0;
}


node_t *delete (uint8_t msgid, list_t *list,struct sockaddr_in6 *addr)
{

    pthread_mutex_lock(&(list->lock));

    node_t *head = list->head;
    node_t *current = head;
    node_t *previous = NULL;
    

    //if list is empty
    if (head == NULL)
    {
        return NULL;
    }

    //navigate through list
    while ((current->pkt.msgid != msgid) && compare_ipv6(&(current->addr->sin6_addr),&addr->sin6_addr) != 0)
    {
        
        
        //if it is last node
        if (current->next == NULL)
        {
            return NULL;
            pthread_mutex_unlock(&(list->lock));
        }
        else
        {
            //store reference to current link
            previous = current;
            //move to next link
            current = current->next;
        }
    }
    //found a match, update the link
    printf("inside delete : \n");
    node_t *tmp = current;
    if (current == head)
    {
        //change first to point to next link
        list->head = head->next;
    }
    else
    {
        //bypass the current link
        previous->next = current->next;
    }
    free(tmp->addr);
    free(tmp);
    pthread_mutex_unlock(&(list->lock));
    return NULL;
}

// void main()
// {
//     list_t *list = init_list();
//     pkt_t pkt1;
//     pkt_t pkt2;
//     uint16_t len = 2;
//     char message1[] = "a";
//     char message2[] = "b";
//     uint8_t msgid1 = 1;
//     uint8_t msgid2 = 2;
//     pkt_set_payload(&pkt1, (const char *)(&message1), 2);
//     pkt_set_payload(&pkt2, (const char *)(&message2), 2);
//     pkt_set_msgid(&pkt1,msgid1);
//     pkt_set_msgid(&pkt2,msgid2);

//     pkt_set_token(&pkt1,msgid1);

//     pkt_set_token(&pkt2,msgid2);
//     struct sockaddr_in6 addr;
//     insertFirst(pkt1, list,addr);
//     insertFirst(pkt2, list,addr);
//     printList(list);
//     delete(msgid1,msgid1,list);
//     printf("============\n");
//     printList(list);
//     printf("============\n");
//     delete(msgid2,msgid2,list);
//     printList(list);
    
// }