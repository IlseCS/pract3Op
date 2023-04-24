/* SSOO-P3 2022-2023
 * David Roldán Nogal				100451289
 * Ilse Mariana Córdova Sánchez		100501460
 *
 * Sistema multi-hilo concurrente que que actúa como un banco 
 * que proporciona operaciones sobre cuentas desde cajeros 
 * automáticos, de tal forma que el saldo de las cuentas sea 
 * siempre correcto. */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <math.h>

#include "queue.h"

// Variables globales.
int client_numop = 0;   // Incrementa concurrentemente cada vez que un cliente hace una operación.
int bank_numop = 0;     // Incrementa concurrentemente cada vez que un trabajador hace una operación.
int global_balance = 0; // Se modifica cada vez que se hace un ingreso o retirada de fondos.
int *list_client_ops;    // Array donde se cargan las operaciones del fichero y a la que todos los cajeros acceden
int *saldo_cuenta;      // Actualizar el saldo de la cuenta i. 

// Mutex y condiciones.
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t desc = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// Variables de sincronización.
pthread_cond_t no_lleno; // Podemos agregar o no elementos.
pthread_cond_t no_vacio; // Podemos remover o no elementos.

 // NO ESTOY SEGURA DE SI ESTO SIRVE DE ALGO
// Estructuras necesarias para el desarrollo del programa. 
struct cajeroParams {
  int id;
  int operaciones;
};

struct trabajadorParams {
  int operaciones;
};
 // NO ESTOY SEGURA DE SI ESTO SIRVE DE ALGO

// Declaración de las funciones para el cajero y el trabajador.
void *cajero(void *cajeroArg);
void *trabajador(void *trabajadorArg);

/**
 * Función main que se encarga de las siguientes tareas:
 * 
 * Leer los argumentos input.
 * Leer los datos del fichero a memoria.
 * Crear cajeros.
 * Crear trabajadores.
 * Esperar a que la ejecución de los cajeros termine.
 * Terminar la ejecución de los trabajadores.
 * Mostrar los resultados. 
 * 
 * @param argc
 * @param argv
 * @return
 */
int main (int argc, const char * argv[] ) {
    // Variables utilizadas.
    FILE *nombre_fichero;       // Para el descriptor del fichero que se procesa.
    struct queue *buffer;       // Estructura para el buffer.
    int max_cuentas;            // Para guardar el número máximo de cuentas que puede tener un banco.
    int num_cajeros;            // Número de cajeros especificado como argumento de entrada.
    int num_empleados;          // Número de trabajadores especificado como argumento de entrada.
    int tam_buff;               // Tamaño del buffer especificado como argumento de entrada.
    int max_operaciones;        // Para guardar el número de operaciones.
    char caracter;              // Utilizado para contar número de líneas del archivo.
    int num_lines = 0;          // Utilizado para contar número de líneas del archivo.
    int lines = num_lines - 1;  // Utilizado para contar número de líneas del archivo.
    int init;                   // Indice para apuntar a la línea de inicio.

    // Para checar si el número de argumentos es correcto.
    if (argc != 6) {
    	perror("Error: número de argumentos no válido (<nombre_fichero> <num_cajeros> <num_trabajadores> <max_cuentas> <tam_buff>).\n");
    	return -1;
  	}

    // Extraer el número de cajeros del segundo argumento. 
    num_cajeros = atoi(argv[2]);
    if (num_cajeros <= 0) {
      perror("Error: Número de cajeros debe ser al menos 1.\n");
      return -1;
    }

    // Extraer el número de trabajadores del tercer argumento. 
    num_empleados = atoi(argv[3]);
    if (num_empleados <= 0) {
      perror("Error: Número de trabajadores debe ser al menos 1.\n");
      return -1;
    }

    // Extraer el número máximo de cuentas del cuarto argumento. --- ???
    max_cuentas = atoi(argv[4]);
    if (max_cuentas <= 0) {
      perror("Error: Número máximo de cuentas debe ser al menos 1.\n");
      return -1;
    } 

    // Extraer el tamaño del buffer del quinto argumento. 
    tam_buff = atoi(argv[4]);
    if (tam_buff <= 0) {
      perror("Error: Tamaño del buffer debe ser al menos 1.\n");
      return -1 ;
    }

    // Para abrir el archivo pasado en el primer argumento.
    nombre_fichero = fopen(argv[1], "r");

    // Verificar si hubo error al abrir el archivo.
    if (nombre_fichero == NULL) {
      perror("Error: problema al abrir el archivo indivado en argv[1].\n");
      return -1;
    }

    // Obtenemos el número de operaciones de la primera línea del archivo.
    if (fscanf(nombre_fichero, "%d", &max_operaciones) < 0) {
      perror("Error: problema al obtener el número de operaciones.\n");
      return -1;
    }

    // Reservar memoria para el máximo de operaciones permitidas (max operaciones) con malloc.
    list_client_ops = malloc(sizeof(struct element) * max_operaciones);
    saldo_cuenta = malloc(sizeof(int) * max_cuentas);
    
    /* Se debe comprobar que no se supere en número de operaciones indicadas. 
     * Además, no puede haber menos operaciones en el fichero que operaciones 
     * indica el primer valor. Para comprobar esto, primero calculamos el 
     * número de operaciones en el archivo. */
 
    while(!feof(nombre_fichero)) { // El final de archivo (feof) es usado para checar si ya se procesó todo el fichero.
      // Leer caracter.
      caracter = fgetc(nombre_fichero);

      // Si el caracter es '\n', el contador se incrementa.
      if (caracter == '\n') {
        num_lines++;
      }
    }

    // Checar error al cerrar el archivo. 
    if (fclose(nombre_fichero) < 0) {
      perror("Error: problema al cerrar el fichero.\n");
      return -1;
    }
    
    /* Verificamos que el número de operaciones sea igual o menor que el total
     * de líneas en el archivo. No pueden existir menos transacciones que el
     * número de operaciones a llevar a cabo. */   
    if (max_operaciones > lines) { 
      perror("Error: no pueden existir menos operaciones en el archivo que el máximo de operaciones contenidas en el mismo.\n");
      return -1;
    }

    // Creamos el buffer, que en este caso es una queue, del tamaño introducido como argumento.
    buffer = queue_init(tam_buff);

    // Inicializamos los mutex para tratar el buffer (queue) compartido y checamos errores de inicialización. 
    if (pthread_mutex_init(&mutex, NULL) < 0) {
      perror("Error: problema en la inicialización del mutex.\n");
      return -1;
    }

    if (pthread_mutex_init(&desc, NULL) < 0) {
      perror("Error: problema en la inicialización del mutex.\n");
      return -1;
    }

    // Variable condicional no_lleno.
    if (pthread_cond_init(&no_lleno, NULL) < 0) {
      perror("Error: problema en la inicialización de la variable condicional no_lleno.\n");
      return -1;
    }

    // Variable condicional no_vacio.
    if (pthread_cond_init(&no_vacio, NULL) < 0) {
      perror("Error: problema en la inicialización de la variable condicional no_vacio.\n");
      return -1;
    }

    // Definimos mismo número de hilos como cajeros y empleados tenemos. 
    pthread_t threads_cajeros[num_cajeros];
    pthread_t threads_empleados[num_empleados];

    // NO ESTOY SEGURA DE SI ESTO SIRVE DE ALGO
    /* Para establecer el número de operaciones que cada empleado debe insertar
     * al buffer circular. Para evitar problemas utilizamos la función floor. */
    int operaciones_cajeros = floor((max_operaciones / num_cajeros));
    int operaciones_empleados = floor((max_operaciones / num_empleados));
    // NO ESTOY SEGURA DE SI ESTO SIRVE DE ALGO

    // ---------- Sección dedicada a los cajeros (productores). ----------
    
    // Pointer inicial a 1.
    init = 1;

    // Estructura para guardar los parámetros del hilo.
    struct cajeros_params cajeros_args[num_cajeros];
    
    // Crear los hilos para los cajeros (productores). 
	  int j;

    // NOTA: excluimos el último cajero (tendrá menos o el mismo número de operaciones que el resto).
    for (j = 0; j < (num_cajeros - 1); j++) {
        // Parámetros del hilo.
        cajeros_args[j].operations = operaciones_cajeros;
        cajeros_args[j].id = init;

        // Para checar errores durante la creación.
        if (pthread_create(&threads_cajeros[j], NULL, (void*)cajero, &cajeros_args[j]) < 0) {
            perror("Error: problema al crear hilo del cajero.\n");
            return -1;
        }

        // SEGUIR TRADUCIENDO A PARTIR DE AQUÍ

        /*
        */
        // We put the init (pointer) in the position of the next set of operations that the next producer will insert.
        // In this case, producer_operations acts as an offset.
        init += operaciones_cajeros;
    }

    // Check how many operations have the last producer, since the last one has less operations.
    int last_producer_operations = num_operations - (j * producer_operations);
    producer_args[num_producers - 1].operations = last_producer_operations;
    producer_args[num_producers - 1].initial_id = init;

    // Create the thread for the remaining producer.
    if (pthread_create(&producer_threads[num_producers - 1], NULL, (void*)producer, &producer_args[num_producers - 1]) < 0) {
        perror("[ERROR] Error while creating the last producer thread.\n");
        return(-1);
    }

    // ---- ???
    //const char *file;

    // Para reservar memoria dinámica para guardar el archivo.
    //file = malloc(sizeof(char[strlen(argv[1])]));
    //file = argv[1];
    // ---- ???

    // var global no pasar de 200 el crear
    // struct gestor cuentas - max y creadas para ir comparando 

    // RECUERDA HACER FREE DE MALLOC.
    
    return 0;
}