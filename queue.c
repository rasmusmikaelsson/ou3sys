#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

typedef struct Node {
	void *value;
	struct Node *next;
} Node;

struct Queue{
    Node *head;
    Node *tail;
	int size;
};

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

void *peek(Queue *q) {
	if(is_empty(q)) {
		return NULL;
	}
	
	return q->head->value;
}

int enqueue(Queue *q, void *value) {
	Node *new_node = malloc(sizeof(Node));
	if(new_node == NULL) {
		perror("malloc");
		return -1;
	}
	
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
	return 1;
}

void *dequeue(Queue *q) {
	if(is_empty(q)) {
		return NULL;
	}

	void *value = q->head->value;
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
    while (current_node != NULL) {
        Node *temp = current_node;
        current_node = current_node->next;
        free(temp);
    }
    free(q);
}

