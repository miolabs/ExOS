#include "list.h"
#include "panic.h"

void list_initialize(list_t *list)
{
	LIST_HEAD(list)->Succ = LIST_TAIL(list);
	LIST_TAIL(list)->Pred = LIST_HEAD(list);
	list->Tail = NULL;
}

void list_insert(node_t *pred, node_t *node)
{
	node->Pred = pred;
	node->Succ = pred->Succ;
	node->Succ->Pred = node;
	node->Pred->Succ = node;
}

void list_add_head(list_t *list, node_t *node)
{
	list_insert(LIST_HEAD(list), node);
}

void list_add_tail(list_t *list, node_t *node)
{
	list_insert(LIST_TAIL(list)->Pred, node);
}

void list_enqueue(list_t *list, node_t *node)
{
	node_t *pred = LIST_TAIL(list)->Pred;
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

void list_remove(node_t *node)
{
	ASSERT(node != NULL && node->Pred != NULL && node->Succ != NULL, KERNEL_ERROR_LIST_CORRUPTED);
	node->Pred->Succ = node->Succ;
	node->Succ->Pred = node->Pred;
#ifdef DEBUG
	node->Pred = node->Succ = NULL;
#endif
}

node_t *list_find_node(list_t *list, node_t *node)
{
	node_t *found = NULL;
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

int list_get_count(list_t *list)
{
	int count = 0;
	FOREACH(node_i, list) count++;
	return count;
}

node_t *list_rem_head(list_t *list)
{
	node_t *node;
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
