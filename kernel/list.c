#include "list.h"
#include "panic.h"

void list_initialize(EXOS_LIST *list)
{
	LIST_HEAD(list)->Succ = LIST_TAIL(list);
	LIST_TAIL(list)->Pred = LIST_HEAD(list);
	list->Tail = NULL;
}

void list_insert(EXOS_NODE *pred, EXOS_NODE *node)
{
	node->Pred = pred;
	node->Succ = pred->Succ;
	node->Succ->Pred = node;
	node->Pred->Succ = node;
}

void list_add_head(EXOS_LIST *list, EXOS_NODE *node)
{
	list_insert(LIST_HEAD(list), node);
}

void list_add_tail(EXOS_LIST *list, EXOS_NODE *node)
{
	list_insert(LIST_TAIL(list)->Pred, node);
}

void list_enqueue(EXOS_LIST *list, EXOS_NODE *node)
{
	EXOS_NODE *pred = LIST_HEAD(list);
	FOREACH(node_i, list)
	{
		if (node_i->Priority < node->Priority) 
		{
			pred = node_i->Pred;
			break;
		}
	}
	list_insert(pred, node);
}

void list_remove(EXOS_NODE *node)
{
#ifdef DEBUG
	if (node == NULL || node->Pred == NULL || node->Succ == NULL) kernel_panic(KERNEL_ERROR_LIST_CORRUPTED);
#endif
	node->Pred->Succ = node->Succ;
	node->Succ->Pred = node->Pred;
#ifdef DEBUG
	node->Pred = node->Succ = NULL;
#endif
}

EXOS_NODE *list_find_node(EXOS_LIST *list, EXOS_NODE *node)
{
	EXOS_NODE *found = NULL;
	FOREACH(node_i, list)
	{
		if (node_i == node)
		{
			found = node_i;
			break;
		}
	}
	return found;
}

int list_get_count(EXOS_LIST *list)
{
	int count = 0;
	FOREACH(node_i, list) count++;
	return count;
}

EXOS_NODE *list_rem_head(EXOS_LIST *list)
{
	EXOS_NODE *node;
	if (LIST_ISEMPTY(list))
	{
		node = NULL;
	}
	else
	{
		node = LIST_HEAD(list)->Succ;
		list_remove(node);
	}
	return node;
}
