#include <stdbool.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct Node {
	int value;
	struct Node *next;
} Node;

typedef struct {
    Node *head;
    Node *tail;
	int size;
} Queue;

Queue *create_queue() {
	Queue *q = malloc(sizeof(Queue));
	q->head = NULL;
	q->tail = NULL;
	q->size = 0;
	
	return q;
}

int size(Queue *q) {
	return q->size;
}

bool is_empty(Queue *q) {
	return (q->size == 0);
}

int peek(Queue *q, bool *status) {
	if(is_empty(q)) {
		*status = false;
		return NULL;
	}
	
	*status = true;
	return q->head->value;
}

int enqueue(Queue *q, int value) {
	Node *new_node = malloc(sizeof(Node));
	
	new_node->value = value;
	new_node->next = NULL;

	if(is_empty(q)) {
		q->head = new_node;
		q->tail = new_node;
	} else {
		q->tail->next = new_node;
		q->tail = new_node;
	}

	q->size++;
}

int dequeue(Queue *q, bool *status) {
	if(is_empty(q)) {
		*status = false;
		return NULL;
	}
	*status = true;

	int value = q->head->value;
	Node *old_head = q->head;

	// If last element, reset the queue
	if(q->size == 1) {
		q->head = NULL;
		q->tail = NULL;
	} else {
		q->head = q->head->next;
	}
	
	free(old_head);
	q->size--;
	return value;
}

void free_queue(Queue *q) {
	Node *current_node = q->head;

	// Remove elements one by one
	while(current_node == NULL) {
		Node *temp = current_node;
		current_node = current_node->next;
		free(temp);
	}

	free(q);
}
