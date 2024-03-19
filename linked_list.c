#include "linked_list.h"

// Creates and returns a new list
list_t* list_create()
{
    list_t* new_list = (list_t*)malloc(sizeof(list_t));
    new_list->count = 0;
    new_list->head = NULL;

    return new_list;

}

// Destroys a list
void list_destroy(list_t* list)
{
    list_node_t* current_node = list->head;
    while (current_node != NULL)
    {
        list_node_t* next_node = current_node->next;
        free(current_node);
        current_node = next_node;
    }
    free(list);
}

// Returns beginning of the list
list_node_t* list_begin(list_t* list)
{
    return list->head;

}

// Returns next element in the list
list_node_t* list_next(list_node_t* node)
{
    return node->next;

}

// Returns data in the given list node
void* list_data(list_node_t* node)
{
    
    return node->data;

}

// Returns the number of elements in the list
size_t list_count(list_t* list)
{
    
    return list->count;

}

// Finds the first node in the list with the given data
// Returns NULL if data could not be found
list_node_t* list_find(list_t* list, void* data)
{
    list_node_t* current_node = list->head;
    while (current_node!= NULL)
    {
        if (current_node-> data == data)
        {
            return current_node;
        }
        current_node = current_node->next;
    }
    return NULL;
}

// Inserts a new node in the list with the given data
void list_insert(list_t* list, void* data)
{
    list_node_t* new_node = (list_node_t*)malloc(sizeof(list_node_t));

    new_node->prev = NULL;
    new_node->data = data;
    new_node->next = list->head;


    if (list->head == NULL)
    {
        list->head = new_node;
    }
    else 
    {
        list->head->prev = new_node;
        list->head = new_node;
    }


    list->count++;






}

// Removes a node from the list and frees the node resources
void list_remove(list_t* list, list_node_t* node)
{
    if (node != NULL) {

        
        if (node->prev == NULL)
        {
            list->head = node->next;

        }
        else if (node->prev != NULL)
        {
            node->prev->next = node->next;
    

		}
		else if (node->next != NULL)
		{

			node->next->prev = node->prev;
		}
	
        free(node);
        list->count--;
    }

}

// Executes a function for each element in the list
void list_foreach(list_t* list, void (*func)(void* data))
{
    list_node_t* current_node = list->head;
    while (current_node!= NULL)
    {
        func(current_node -> data);
        current_node = current_node->next;
    }

}
