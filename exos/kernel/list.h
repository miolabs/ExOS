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
	EXOS_NODE_PORT,
	EXOS_NODE_MESSAGE,
	EXOS_NODE_INITIALIZER,

	EXOS_NODE_TREE_NODE,
	EXOS_NODE_IO_DEVICE,
	EXOS_NODE_IO_ENTRY,
	EXOS_NODE_IO_BUFFER,
} node_type_t;

typedef struct __EXOS_NODE
{
	struct __EXOS_NODE *Succ;
	struct __EXOS_NODE *Pred;
#ifdef DEBUG
	node_type_t Type;
#endif
	int Priority;
} node_t;

typedef struct 
{
	node_t Node;
	void *Ref;
} node_ref_t;

typedef struct
{
	node_t *Head;
	node_t *Tail;
	node_t *TailPred;
} list_t;

#define LIST_HEAD(l) ((node_t *)&((l)->Head))
#define LIST_TAIL(l) ((node_t *)&((l)->Tail))
#define LIST_ISEMPTY(l) ((l)->Head == LIST_TAIL(l))
#define LIST_FIRST(l) (LIST_ISEMPTY(l) ? NULL : LIST_HEAD(l)->Succ)
#define FOREACH(n, l) for(node_t *n = LIST_HEAD(l)->Succ; n != LIST_TAIL(l); n = n->Succ)
#define NODE_PRED(l, n) (((n)->Pred != LIST_HEAD(l)) ? (n)->Pred : NULL)
#define NODE_SUCC(l, n) (((n)->Succ != LIST_TAIL(l)) ? (n)->Succ : NULL)

// prototypes
void list_initialize(list_t *list);
void list_insert(node_t *pred, node_t *node);
void list_add_head(list_t *list, node_t *node);
void list_add_tail(list_t *list, node_t *node);
void list_remove(node_t *node);
void list_enqueue(list_t *list, node_t *node);
node_t *list_find_node(list_t *list, node_t *node);
int list_get_count(list_t *list);
node_t *list_rem_head(list_t *list);

#ifdef EXOS_OLD
#define EXOS_NODE_TYPE node_type_t
#define EXOS_NODE node_t
#define EXOS_LIST list_t
#define EXOS_NODE_REF node_ref_t
#endif 

#endif // EXOS_LIST_H
