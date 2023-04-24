/* David Roldán Nogal				100451289
 * Ilse Mariana Córdova Sánchez		100501460
 * 
 * Fichero de cabeceras donde se definen las estructuras
 * de datos y funciones utilizadas para la gestión de la 
 * cola circular.*/

#ifndef HEADER_FILE
#define HEADER_FILE

// Estructura que representa los elementos de la queue.
struct element { // --- ???
	int operacion;
	//int crear;
	//int saldo;
	//int ingresar[];
	//int retirar[];
	//int traspasar[];
}; 
// int que sea la operacion

// Estructura que representa a la queue.
typedef struct queue {
	// Elementos de la queue.
	struct element *elements;
	// Otros atributos.
	int length;
	int head;
	int tail; 
	int size;
}queue;

queue* queue_init (int size);
int queue_destroy (queue *q);
int queue_put (queue *q, struct element* elem);
struct element * queue_get(queue *q);
int queue_empty (queue *q);
int queue_full(queue *q);

#endif
