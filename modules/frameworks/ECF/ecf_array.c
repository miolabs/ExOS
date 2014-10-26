//
//  ecf_array.c
//  ExOS Core Foundation
//
//  Created by GodShadow on 26/10/14.
//  Copyright (c) 2014 MIO Research Labs. All rights reserved.
//

#include "ecf_array.h"
#include <stdlib.h>

struct ecf_array_node_struct
{
    struct ecf_array_node_struct *prev_node;
    struct ecf_array_node_struct *next_node;
    void *object;
};

typedef struct ecf_array_node_struct ecf_array_node;

struct ecf_array_struct
{
    struct ecf_array_node_struct *start_node;
    struct ecf_array_node_struct *end_node;
    UInt32 count;
};

// Private functions
void _ecf_array_add_node(ecf_array *array, ecf_array_node *node);
void _ecf_array_remove_node(ecf_array *array, ecf_array_node *node);

ecf_array_node *_ecf_array_get_node(ecf_array* array);
ecf_array_node *_ecf_array_get_node_at_index(ecf_array *array, UInt32 index);
ecf_array_node *_ecf_array_move_node_forward(ecf_array_node *node);
ecf_array_node *_ecf_array_move_node_backwards(ecf_array_node *node);

#pragma mark - Functions

ecf_array *ecf_array_create()
{
    ecf_array *array = malloc (sizeof(struct ecf_array_struct));
    if (array == NULL)
        return NULL;

    array->start_node = NULL;
    array->end_node = NULL;
    array->count = 0;
    
    return array;
}

void ecf_array_destroy(ecf_array *array)
{
    //TODO: Memory leak!
}

UInt32 ecf_array_get_count(ecf_array *array)
{
    if (array == NULL)
        return 0;
    
    return array->count;
}

void *ecf_array_get_object_at_index(ecf_array *array, UInt32 index)
{
    if (array == NULL)
        return NULL;
    
    ecf_array_node *node = _ecf_array_get_node_at_index(array, index);
    if (node == NULL)
        return NULL;
    
    return node->object;
}

void ecf_array_add_object(ecf_array *array, void *object)
{
    if (array == NULL)
        return;
    
    ecf_array_node *node = malloc(sizeof(ecf_array_node));
    if (node == NULL)
        return; //TODO: Make excepction. Handle the error!
    
    node->next_node = NULL;
    node->prev_node = NULL;
    node->object = object;
    
    _ecf_array_add_node(array, node);
}

void ecf_array_remove_object(ecf_array *array, void *object)
{
    if(object == NULL)
        return;
    
    if (array == NULL)
        return;
    
    ecf_array_node *node = _ecf_array_get_node(array);
    while (node != NULL)
    {
        if (node->object == object)
        {
            _ecf_array_remove_node(array, node);
            node = NULL;
        }
        else
            node = _ecf_array_move_node_forward(node);
    }
}

void ecf_array_remove_object_at_index(ecf_array *array, UInt32 index)
{
    if (array == NULL)
        return;
    
    ecf_array_node *node = _ecf_array_get_node_at_index(array, index);
    _ecf_array_remove_node(array, node);
}

#pragma mark - Private functions

void _ecf_array_add_node(ecf_array *array, ecf_array_node *node)
{
    if (array->end_node == NULL)
    {
        node->prev_node = NULL;
        array->start_node = node;
        array->end_node = node;
    }
    else
    {
        node->prev_node = array->end_node;
        array->end_node->next_node = node;
        array->end_node = node;
    }
    
    array->count++;
}

void _ecf_array_remove_node(ecf_array *array, ecf_array_node *node)
{
    ecf_array_node *prev_node = node->prev_node;
    ecf_array_node *next_node = node->next_node;
    
    if(prev_node != NULL)
        prev_node->next_node = next_node;
    
    if(next_node != NULL)
        next_node->prev_node = prev_node;
    
    if(node == array->start_node)
        array->start_node = next_node;
    
    if(node == array->end_node)
        array->end_node = prev_node;
    
    free(node);
    array->count--;
}

#pragma mark - Iterator functions

ecf_array_node *_ecf_array_get_node(ecf_array* array)
{
    if (array == NULL)
        return NULL;
    
    return array->start_node;
}

ecf_array_node *_ecf_array_get_node_at_index(ecf_array *array, UInt32 index)
{
    ecf_array_node *node = NULL;
    int middle;
    
    if (array == NULL)
        return node;
    
    if(index >= array->count)
        return node;
    
    middle = array->count / 2;
    
    if(index < middle)
    {
        node = array->start_node;
        int count;
        for(count = 0; count < index; count++)
            node = node->next_node;
    }
    else
    {
        node = array->end_node;
        int count;
        for(count = array->count - 1; count > index; count--)
            node = node->prev_node;
    }
    
    return node;
}

ecf_array_node *_ecf_array_move_node_forward(ecf_array_node *node)
{
    if (node != NULL)
        return node->next_node;
    
    return NULL;
}

ecf_array_node *_ecf_array_move_node_backwards(ecf_array_node *node)
{
    if (node != NULL)
        return node->prev_node;
    
    return NULL;
}
