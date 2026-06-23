// DECLARACIÓN DE LA FUNCIONALIDAD COMÚN PARA LA PARTE CLIENTE
// Y LA PARTE SERVIDORA
#ifndef _COMMON_H
#define _COMMON_H        1

// ============================================================
// TODO: Define aquí los códigos de operación que usará el
// protocolo cliente-servidor. Cada operación necesita un código
// distinto. Puedes usar un char, un int, o una constante.
// Ejemplo de opciones:
//   - Un char: 'P' para getPid, 'N' para alta de nodo, etc.
//   - Un int: #define OP_GETPID 0, #define OP_ADD_NODE 1, etc.
// Todas las operaciones que implementes deben tener un código aquí:
//   - OP_GETPID        : obtener el PID del proceso remoto
//   - OP_ADD_NODE      : dar de alta un nodo nuevo en el anillo
//   - OP_GET_SUCCESSOR : obtener el sucesor del nodo remoto
//   - OP_GET_SUC_SUC   : obtener el sucesor del sucesor (reenvía la petición)
//   - OP_DOWNLOAD      : descargar un fichero del nodo remoto
//   - OP_LOOKUP        : buscar un fichero en el anillo
// ============================================================
#define OP_GETPID 1
#define OP_ADD_NODE 2
#define OP_GET_SUCCESSOR 3
#define OP_GET_SUC_SUC 4 
#define OP_DOWNLOAD 5
#define OP_LOOKUP 6
// thread de servicio
void *server_thread(void *arg);

// crea un thread detached
int create_thread(void *(*func)(void *), void *arg);

// Inicializa el socket, le asigna un puerto seleccionado por el SO
// y lo prepara para aceptar conexiones. Recibe un parámetro de salida
// donde devuelve el puerto asignado.
int create_socket_srv(unsigned short *port);

// crea socket y se conecta al servidor a partir de IP y puerto en formato red
int create_socket_cln(unsigned int ip, unsigned short port);

void set_successor(unsigned int ip, unsigned short port);

char *getDirectorioCompartido();

#endif // _COMMON_H
