/* SSOO-P3 2022-2023
 * David Roldán Nogal				100451289
 * Ilse Mariana Córdova Sánchez		100501460
 *
 * Fichero fuente C donde se implementan las funciones
 * que permiten gestionar la cola circular. */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "queue.h"

// Para crear la queue y reservar el tamaño especificado como parámetro.
queue* queue_init(int size){
	// Creae la queue
	queue * q = (queue *)malloc(sizeof(queue));

	// Para asignar los valores.
	q -> elements = malloc(size * sizeof(struct element));
  	q -> length = 0;
  	q -> head = 0;
  	q -> tail = 0;
	q -> size = size;

	return q;
}

// Para "enqueue" un elemento si es posible o esperar en caso contrario.
int queue_put(queue *q, struct element* x) {
	if (queue_full(q) == 0) {
		// La operación enqueue se hace en la cabeza de la queue.
    	q -> elements[q -> head] = *x;

        // Para el tamaño.
    	q -> head = (q -> head + 1) % q -> size;  
    	q -> length = q -> length + 1;

    	return 0;
  	}

  	return -1;
}

// Para "dequeue" un elemento si es posible o esperar en caso contrario.
struct element* queue_get(queue *q) {
	struct element* element;
	
	if (queue_empty(q) == 0) {
		// La operación dequeue se hace en la cola de la queue.
    	element = &(q -> elements[q -> tail]);

		// Para el tamaño.
    	q -> tail = (q -> tail + 1) % q -> size;
    	q -> length = q -> length - 1;
  	}

	return element;
}

// Para checar si la queue está vacía (return 1) o no (return 0).
int queue_empty(queue *q){
	// Si está vacía. 
	if (q -> length == 0){
    	return 1;
	}

	// Si no está vacía.
	return 0;
}

// Para checar si la queue está llena (return 1) o no (return 0).
int queue_full(queue *q){
	// Si está llena.
	if (q -> length == q -> size){
    	return 1;
	}

	// Si no está llena.
  	return 0;
}

// Para destruir la queue y liberar los recursos.
int queue_destroy(queue *q){
	free(q -> elements);
	free(q);
	
    return 0;
}
