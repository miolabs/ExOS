#ifndef EXOS_LIST_H
#define EXOS_LIST_H

#include <kernel/types.h>

typedef enum
{
	EXOS_NODE_UNKNOWN = 0,
	EXOS_NODE_THREAD,
	EXOS_NODE_TIMER,
	EXOS_NODE_WAIT_HANDLE,
	EXOS_NODE_MEM_REGION,
	EXOS_NODE_MEM_NODE,
	EXOS_NODE_MONITOR,

	EXOS_NODE_TREE_NODE,
	EXOS_NODE_IO_DEVICE,
	EXOS_NODE_IO_ENTRY,
	EXOS_NODE_IO_BUFFER,
} EXOS_NODE_TYPE;

typedef struct __EXOS_NODE
{
	struct __EXOS_NODE *Succ;
	struct __EXOS_NODE *Pred;
#ifdef DEBUG
	EXOS_NODE_TYPE Type;
#endif
	int Priority;
} EXOS_NODE;

typedef struct 
{
	EXOS_NODE Node;
	void *Ref;
} EXOS_NODE_REF;

typedef struct
{
	EXOS_NODE *Head;
	EXOS_NODE *Tail;
	EXOS_NODE *TailPred;
} EXOS_LIST;

#define LIST_HEAD(l) ((EXOS_NODE *)&((l)->Head))
#define LIST_TAIL(l) ((EXOS_NODE *)&((l)->Tail))
#define LIST_ISEMPTY(l) ((l)->Head == LIST_TAIL(l))
#define LIST_FIRST(l) (LIST_ISEMPTY(l) ? NULL : LIST_HEAD(l)->Succ)
#define FOREACH(n, l) for(EXOS_NODE *n = LIST_HEAD(l)->Succ; n != LIST_TAIL(l); n = n->Succ)
#define NODE_PRED(l, n) (((n)->Pred != LIST_HEAD(l)) ? (n)->Pred : NULL)
#define NODE_SUCC(l, n) (((n)->Succ != LIST_TAIL(l)) ? (n)->Succ : NULL)

// prototypes
void list_initialize(EXOS_LIST *list);
void list_insert(EXOS_NODE *pred, EXOS_NODE *node);
void list_add_head(EXOS_LIST *list, EXOS_NODE *node);
void list_add_tail(EXOS_LIST *list, EXOS_NODE *node);
void list_remove(EXOS_NODE *node);
void list_enqueue(EXOS_LIST *list, EXOS_NODE *node);
EXOS_NODE *list_find_node(EXOS_LIST *list, EXOS_NODE *node);
int list_get_count(EXOS_LIST *list);
EXOS_NODE *list_rem_head(EXOS_LIST *list);

#endif // EXOS_LIST_H
