#ifndef LLIST_H
#define LLIST_H

#include <stdbool.h>


typedef struct llist_node_s{
	struct llist_node_s *next;
	struct llist_node_s *prev;
	unsigned char data[0];
}llist_node_t;


typedef llist_node_t llist_t;

#define llistForEach(list, current) for((current) = (list)->next; (current) != list; (current) = (current)->next)

typedef void(llist_delete_callback)(llist_node_t *node);
typedef int(llist_compare_callback)(llist_node_t *node1, llist_node_t *node2);


void llistInit(llist_t *list);
void llistFree(llist_t *list, llist_delete_callback deleteCallback);

bool llistIsEmpty(llist_t *list);
unsigned int llistGetLength(llist_t *list);

llist_node_t* llistGetHead(llist_t *list);
llist_node_t* llistGetTail(llist_t *list);
llist_node_t* llistGetNext(llist_t *list, llist_node_t *node);
llist_node_t* llistGetPrev(llist_t *list, llist_node_t *node);
llist_node_t* llistGetIdx(llist_t *list, unsigned int index);

void llistRemoveNode(llist_t *list, llist_node_t *node);
void llistRemoveHead(llist_t *list);
void llistRemoveTail(llist_t *list);
void llistRemoveAll(llist_t *list);
void llistRemoveIdx(llist_t *list, unsigned int index);

void llistDeleteNode(llist_t *list, llist_node_t *node, llist_delete_callback deleteCallback);
void llistDeleteHead(llist_t *list, llist_delete_callback deleteCallback);
void llistDeleteTail(llist_t *list, llist_delete_callback deleteCallback);
void llistDeleteAll(llist_t *list, llist_delete_callback deleteCallback);
void llistDeleteIdx(llist_t *list, unsigned int index, llist_delete_callback deleteCallback);

void llistInsertHead(llist_t *list, llist_node_t *node);
void llistInsertTail(llist_t *list, llist_node_t *node);
void llistInsertIdx(llist_t *list, unsigned int index, llist_node_t *node);
void llistInsertInOrder(llist_t *list, llist_node_t *node, llist_compare_callback compareCallback);

void llistConcat(llist_t *list1, llist_t *list2);
void llistSplitNode(llist_t *list1, llist_t *list2, llist_node_t *node);
void llistSplitIdx(llist_t *list1, llist_t *list2, unsigned int index);


#endif