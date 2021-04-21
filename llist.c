#include <stdlib.h>
#include "llist.h"

void llistInit(llist_t *list){
	list->next = list;
	list->prev = list;
}

bool llistIsEmpty(llist_t *list){
	if(list->next == list && list->prev == list){
		return(true);
	}
	return(false);
}

unsigned int llistGetLength(llist_t *list){
	llist_node_t *current;
	unsigned int length = 0;
	llistForEach(list, current){
		length++;
	}
	return(length);
}

llist_node_t* _llistGetNode(llist_t *list, llist_node_t *node){
	if(node != list){
		return(node);
	}
	return(NULL);
}

llist_node_t* llistGetHead(llist_t *list){
	return(_llistGetNode(list, list->next));
}

llist_node_t* llistGetTail(llist_t *list){
	return(_llistGetNode(list, list->prev));
}

llist_node_t* llistGetNext(llist_t *list, llist_node_t *node){
	return(_llistGetNode(list, node->next));
}

llist_node_t* llistGetPrev(llist_t *list, llist_node_t *node){
	return(_llistGetNode(list, node->prev));
}

llist_node_t* llistGetIdx(llist_t *list, unsigned int index){
	llist_node_t *current;
	llistForEach(list, current){
		if(index == 0){
			return(current);
		}
		index--;
	}
	return(NULL);
}

void _llistRemoveNode(llist_node_t *node){
	if(node != NULL){
		node->next->prev = node->prev;
		node->prev->next = node->next;
		node->next = NULL;
		node->prev = NULL;
	}
}

void llistRemoveNode(llist_t *list, llist_node_t *node){
	_llistRemoveNode(node);
}

void llistRemoveHead(llist_t *list){
	_llistRemoveNode(llistGetHead(list));
}

void llistRemoveTail(llist_t *list){
	_llistRemoveNode(llistGetTail(list));
}

void llistRemoveAll(llist_t *list){
	llistInit(list);
}

void llistRemoveIdx(llist_t *list, unsigned int index){
	_llistRemoveNode(llistGetIdx(list, index));
}

void _llistDeleteNode(llist_node_t *node, llist_delete_callback deleteCallback){
	if(node != NULL){
		node->prev->next = node->next;
		node->next->prev = node->prev;
		deleteCallback(node);
	}
}

void llistDeleteNode(llist_t *list, llist_node_t *node, llist_delete_callback deleteCallback){
	_llistDeleteNode(node, deleteCallback);
}

void llistDeleteHead(llist_t *list, llist_delete_callback deleteCallback){
	_llistDeleteNode(llistGetHead(list), deleteCallback);
}

void llistDeleteTail(llist_t *list, llist_delete_callback deleteCallback){
	_llistDeleteNode(llistGetTail(list), deleteCallback);
}

void llistDeleteAll(llist_t *list, llist_delete_callback *deleteCallback){
	while(llistIsEmpty(list) == false){
		llistDeleteHead(list, deleteCallback);
	}
	llistInit(list);
}

void llistDeleteIdx(llist_t *list, unsigned int index, llist_delete_callback deleteCallback){
	_llistDeleteNode(llistGetIdx(list, index), deleteCallback);
}

void llistFree(llist_t *list, llist_delete_callback *deleteCallback){
	llistDeleteAll(list, deleteCallback);
}

void _llistInsertNode(llist_node_t *prev, llist_node_t *next, llist_node_t *node){
	prev->next = node;
	next->prev = node;
	node->prev = prev;
	node->next = next;
}

void llistInsertHead(llist_t *list, llist_node_t *node){
	_llistInsertNode(list, list->next, node);
}

void llistInsertTail(llist_t *list, llist_node_t *node){
	_llistInsertNode(list->prev, list, node);
}

void llistInsertIdx(llist_t *list, unsigned int index, llist_node_t *node){
	if(index == 0){
		llistInsertHead(list, node);
	}else{
		llist_node_t *prev = llistGetIdx(list, index-1);
		if(prev == NULL){
			return;
		}
		_llistInsertNode(prev, prev->next, node);
	}
}

void llistInsertInOrder(llist_t *list, llist_node_t *node, llist_compare_callback *compareCallback){
	if(llistIsEmpty(list)){
		llistInsertHead(list, node);
	}else{
		llist_node_t *current;
		llistForEach(list, current){
			if(compareCallback(node, current) <= 0){
				_llistInsertNode(current->prev, current, node);
				return;
			}
		}
		llistInsertTail(list, node);
	}
}

void llistConcat(llist_t *list1, llist_t *list2){
	if(llistIsEmpty(list2)){
		return;
	}
	list1->prev->next = list2->next;
	list2->next->prev = list1->prev;
	list1->prev = list2->prev;
	list2->prev->next = list1;
	llistInit(list2);
}

void llistSplitAtNode(llist_t *list1, llist_t *list2, llist_node_t *node){
	if(_llistGetNode(list1, node->next) == NULL){
		llistInit(list2);
	}else{
		list2->next = node->next;
		list2->prev = list1->prev;
		node->next->prev = list2;
		list1->prev->next = list2;
		node->next = list1;
		list1->prev = node;
	}
}

void llistSplitAtIdx(llist_t *list1, llist_t *list2, unsigned int index){
	llist_node_t *node = llistGetIdx(list1, index);
	if(node != NULL){
		llistSplitAtNode(list1, list2, node);
	}else{
		llistInit(list2);
	}
}