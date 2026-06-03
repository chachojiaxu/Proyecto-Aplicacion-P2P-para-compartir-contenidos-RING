// FUNCIONALIDAD DE LA PARTE SERVIDORA
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <pthread.h>
#include "ring.h"
#include "common.h"

// ============================================================
// TODO [FASE 1, Paso 2]: Declara aquí la función que manejará
// cada conexión individual (el "request handler").
// Esta función se ejecutará en un thread separado por cada conexión.
// Recibe como argumento el socket de la conexión (por valor, como long).
// Necesitará también acceder al directorio compartido y al sucesor,
// así que considera si necesitas pasarle más información o si usas
// variables externas (las de ring_cln.c no son accesibles directamente
// desde aquí; valora si necesitas declararlas como extern o añadir
// una función getter en ring_cln.c / common.c).
//
// Una opción sencilla es declarar extern las variables static de
// ring_cln.c aquí, aunque técnicamente no es el mejor diseño.
// Otra opción es crear funciones getter en ring_cln.c y declararlas
// en common.h. Usa la que prefieras.
// ============================================================

// ============================================================
// función que maneja una petición (un thread por conexión)
// ============================================================
// TODO [FASE 1, Paso 2]: Implementa aquí la función request_handler
// (o el nombre que le pongas). Sigue este esquema:
//
// 1. Obtén el socket de conexión del argumento (castea de void* a long a int).
//
// 2. Recibe el código de operación (un char o int, según lo que
//    hayas definido en common.h).
//    Usa recv con MSG_WAITALL.
//    Si falla, cierra el socket y termina el thread (return NULL).
//
// 3. Según el código de operación recibido, usa un switch o if-else:
//
//    --- OP_GETPID [FASE 1, Paso 2] ---
//    - Obtén tu PID con getpid().
//    - Conviértelo a formato de red (htonl).
//    - Envíalo por el socket.
//    - Cierra el socket y termina (return NULL).
//
//    --- OP_ADD_NODE [FASE 2, Paso 1] ---
//    - Recibe el puerto del nuevo nodo (unsigned short, formato red).
//    - Obtén la IP del nuevo nodo: usa getpeername() sobre el socket
//      de conexión para saber de qué IP viene la conexión. Si no lo
//      conoces, mira la estructura sockaddr_in y la función getpeername.
//    - Guarda los valores del sucesor actual para enviarlos como respuesta.
//    - Actualiza el sucesor del nodo actual con la IP y puerto del nuevo nodo.
//    - Envía al nuevo nodo la IP y puerto del antiguo sucesor
//      (estos son los dos valores que el nuevo nodo guardará como SU sucesor).
//    - Cierra el socket y termina (return NULL).
//    NOTA: El acceso al sucesor requiere sincronización si puede haber
//    varios threads concurrentes modificándolo. Para simplificar, puedes
//    ignorar la concurrencia en esta práctica salvo que el enunciado
//    lo exija explícitamente.
//
//    --- OP_GET_SUCCESSOR [FASE 2, Paso 2] ---
//    - Obtén la IP y puerto del sucesor local (usa ring_successor).
//    - Envíalos por el socket (ya están en formato de red).
//    - Cierra el socket y termina (return NULL).
//
//    --- OP_GET_SUC_SUC [FASE 2, Paso 3] ---
//    - Actúa como cliente: llama a ring_remote_successor pasándole
//      la IP y puerto del sucesor local. Eso te dará el sucesor del sucesor.
//    - Envía esa respuesta (IP + puerto) al nodo cliente original.
//    - Cierra el socket y termina (return NULL).
//
//    --- OP_DOWNLOAD [FASE 3] ---
//    - Recibe la longitud del nombre del fichero (int, ntohl).
//    - Reserva memoria dinámica para el nombre + 1 byte para '\0'.
//      NO uses un buffer de tamaño fijo (el nombre puede ser largo).
//    - Recibe el nombre del fichero (recv, MSG_WAITALL).
//    - Añade el carácter '\0' al final.
//    - Construye la ruta completa: directorio_compartido + "/" + nombre.
//    - Abre el fichero con open(O_RDONLY).
//      Si falla (no existe), envía -1 como tamaño y cierra. Libera memoria.
//    - Obtén el tamaño del fichero con fstat.
//    - Envía el tamaño como respuesta (int, htonl).
//    - Usa sendfile para enviar el contenido del fichero por el socket.
//      Fíjate en el ejemplo copy_sendfile.c, sustituyendo el fichero
//      destino por el socket de conexión.
//    - Cierra el fichero, libera la memoria y cierra el socket.
//    - Termina (return NULL).
//
//    --- OP_LOOKUP [FASE 4] ---
//    - Recibe el número de saltos restantes (int, ntohl).
//    - Recibe la longitud del nombre del fichero (int, ntohl).
//    - Reserva memoria y recibe el nombre del fichero.
//    - Añade '\0' al final.
//    - Llama a ring_lookup(nombre, saltos, &ip, &port).
//    - Envía la respuesta al cliente:
//        Si ring_lookup retorna -1: envía un código de "no encontrado"
//        Si retorna 0: envía un código de "encontrado" + IP + puerto
//      Define tú cómo es este protocolo de respuesta, pero que sea
//      consistente con lo que espera ring_lookup en ring_cln.c.
//    - Libera la memoria, cierra el socket y termina (return NULL).
//
//    --- default ---
//    - Imprime un mensaje de error indicando el código desconocido.
//    - Cierra el socket y termina.
// ============================================================
static void *request_handler(void *arg){
    int s = (long)arg;
    int op;
    if(recv(s, &op, sizeof(op), MSG_WAITALL) != sizeof(op)){
        close(s);
        return NULL;
    }
    op = ntohl(op);
    switch (op) {
        case OP_GETPID: {
            int pid = htonl(getpid());
            send(s, &pid, sizeof(pid), 0);
            break;
        }
//    --- OP_ADD_NODE [FASE 2, Paso 1] ---
//    - Recibe el puerto del nuevo nodo (unsigned short, formato red).
//    - Obtén la IP del nuevo nodo: usa getpeername() sobre el socket
//      de conexión para saber de qué IP viene la conexión. Si no lo
//      conoces, mira la estructura sockaddr_in y la función getpeername.
//    - Guarda los valores del sucesor actual para enviarlos como respuesta.
//    - Actualiza el sucesor del nodo actual con la IP y puerto del nuevo nodo.
//    - Envía al nuevo nodo la IP y puerto del antiguo sucesor
//      (estos son los dos valores que el nuevo nodo guardará como SU sucesor).
//    - Cierra el socket y termina (return NULL).
//    NOTA: El acceso al sucesor requiere sincronización si puede haber
//    varios threads concurrentes modificándolo. Para simplificar, puedes
//    ignorar la concurrencia en esta práctica salvo que el enunciado
//    lo exija explícitamente.
        case OP_ADD_NODE: {
            unsigned short nuevo_puerto;
            unsigned int nueva_ip;
            unsigned int vieja_ip;
            unsigned short viejo_puerto;
            struct sockaddr_in addr;
            unsigned int len = sizeof(addr);
            recv(s, &nuevo_puerto, sizeof(nuevo_puerto), MSG_WAITALL);
            getpeername(s, (struct sockaddr*)&addr, &len);
            nueva_ip = addr.sin_addr.s_addr;
            ring_successor(&vieja_ip, &viejo_puerto);
            set_successor(nueva_ip, nuevo_puerto);
            struct iovec iov[2];
            iov[0].iov_base = &vieja_ip;
            iov[0].iov_len  = sizeof(vieja_ip);
            iov[1].iov_base = &viejo_puerto;
            iov[1].iov_len  = sizeof(viejo_puerto);
            writev(s, iov, 2);
            break;
        }
//    --- OP_GET_SUCCESSOR [FASE 2, Paso 2] ---
//    - Obtén la IP y puerto del sucesor local (usa ring_successor).
//    - Envíalos por el socket (ya están en formato de red).
//    - Cierra el socket y termina (return NULL).
        case OP_GET_SUCCESSOR: {
            unsigned short PuertoSucesor;
            unsigned int IPSucesor;
            ring_successor(&IPSucesor, &PuertoSucesor);
            struct iovec iov[2];
            iov[0].iov_base = &IPSucesor;
            iov[0].iov_len  = sizeof(IPSucesor);
            iov[1].iov_base = &PuertoSucesor;
            iov[1].iov_len  = sizeof(PuertoSucesor);
            writev(s, iov, 2);
            break;
        }
//    --- OP_GET_SUC_SUC [FASE 2, Paso 3] ---
//    - Actúa como cliente: llama a ring_remote_successor pasándole
//      la IP y puerto del sucesor local. Eso te dará el sucesor del sucesor.
//    - Envía esa respuesta (IP + puerto) al nodo cliente original.
//    - Cierra el socket y termina (return NULL).
        case OP_GET_SUC_SUC: {
            unsigned short PuertoSucesor;
            unsigned int IPSucesor;
            unsigned short PuertoSucSuc;
            unsigned int IPSucSuc;
            ring_successor(&IPSucesor, &PuertoSucesor);
            ring_remote_successor(IPSucesor, PuertoSucesor, &IPSucSuc, &PuertoSucSuc);
            struct iovec iov[2];
            iov[0].iov_base = &IPSucSuc;
            iov[0].iov_len  = sizeof(IPSucSuc);
            iov[1].iov_base = &PuertoSucSuc;
            iov[1].iov_len  = sizeof(PuertoSucSuc);
            writev(s, iov, 2);
            break;
        }
//    --- OP_DOWNLOAD [FASE 3] ---
//    - Recibe la longitud del nombre del fichero (int, ntohl).
//    - Reserva memoria dinámica para el nombre + 1 byte para '\0'.
//      NO uses un buffer de tamaño fijo (el nombre puede ser largo).
//    - Recibe el nombre del fichero (recv, MSG_WAITALL).
//    - Añade el carácter '\0' al final.
//    - Construye la ruta completa: directorio_compartido + "/" + nombre.
//    - Abre el fichero con open(O_RDONLY).
//      Si falla (no existe), envía -1 como tamaño y cierra. Libera memoria.
//    - Obtén el tamaño del fichero con fstat.
//    - Envía el tamaño como respuesta (int, htonl).
//    - Usa sendfile para enviar el contenido del fichero por el socket.
//      Fíjate en el ejemplo copy_sendfile.c, sustituyendo el fichero
//      destino por el socket de conexión.
//    - Cierra el fichero, libera la memoria y cierra el socket.
//    - Termina (return NULL).
        case OP_DOWNLOAD: {
            int longitud_net;
            recv(s, &longitud_net, sizeof(longitud_net), MSG_WAITALL);
            int longitud=ntohl(longitud_net);
            char *nombre=malloc(longitud + 1);
            recv(s, nombre, longitud, MSG_WAITALL);
            nombre[longitud] = '\0';
            char path[4096];
            snprintf(path, sizeof(path), "%s/%s", getDirectorioCompartido(), nombre);
            int fd;
            if((fd = open(path, O_RDONLY)) < 0) {
                int err = htonl(-1);
                send(s, &err, sizeof(err), 0);
                free(nombre);
                break;
            }
            struct stat st;
            fstat(fd, &st);
            int tam = htonl((int)st.st_size);
            send(s, &tam, sizeof(tam), MSG_MORE);
            sendfile(s, fd, NULL, st.st_size);
            close(s);
            close(fd);
            free(nombre);
            break;
        }
//    --- OP_LOOKUP [FASE 4] ---
//    - Recibe el número de saltos restantes (int, ntohl).
//    - Recibe la longitud del nombre del fichero (int, ntohl).
//    - Reserva memoria y recibe el nombre del fichero.
//    - Añade '\0' al final.
//    - Llama a ring_lookup(nombre, saltos, &ip, &port).
//    - Envía la respuesta al cliente:
//        Si ring_lookup retorna -1: envía un código de "no encontrado"
//        Si retorna 0: envía un código de "encontrado" + IP + puerto
//      Define tú cómo es este protocolo de respuesta, pero que sea
//      consistente con lo que espera ring_lookup en ring_cln.c.
//    - Libera la memoria, cierra el socket y termina (return NULL).
        case OP_LOOKUP: {
            int saltos_net;
            recv(s, &saltos_net, sizeof(saltos_net), MSG_WAITALL);
            int saltos = ntohl(saltos_net);
            int longitud_net;
            recv(s, &longitud_net, sizeof(longitud_net), MSG_WAITALL);
            int longitud=ntohl(longitud_net);
            char *nombre=malloc(longitud + 1);
            recv(s, nombre, longitud, MSG_WAITALL);
            nombre[longitud] = '\0';
            unsigned int ip;
            unsigned short puerto;
            int res = ring_lookup(nombre, saltos, &ip, &puerto);
            if(res == -1){
                int noEncontrado = htonl(-1);
                send(s, &noEncontrado, sizeof(noEncontrado), 0);
            }
            else{
                int encontrado = htonl(0);
                struct iovec iov[3];
                iov[0].iov_base = &encontrado;
                iov[0].iov_len  = sizeof(encontrado);
                iov[1].iov_base = &ip;
                iov[1].iov_len  = sizeof(ip);
                iov[2].iov_base = &puerto;
                iov[2].iov_len  = sizeof(puerto);
                writev(s, iov, 3);
            }
            free(nombre);
            break;
        }
        default:
            fprintf(stderr, "operacion desconocida: %d\n", op);
            break;
    }
    close(s);
    return NULL;
}

// función para el thread que implementa la funcionalidad de servidor
// debe recibir como argumento el socket de servicio
void *server_thread(void *arg){
    // ============================================================
    // TODO [FASE 1, Paso 1 - server_thread]:
    // Adapta el bucle principal del ejemplo server.c para que se
    // ejecute como un thread (no como un main).
    //
    // 1. Obtén el socket de servicio del argumento (castea de void* a long a int).
    //
    // 2. Bucle infinito:
    //    a) Declara una estructura sockaddr_in para la dirección del cliente.
    //    b) Llama a accept() sobre el socket de servicio para aceptar
    //       la siguiente conexión. Si falla, puedes imprimir el error
    //       y continuar o terminar (según prefieras).
    //    c) Lanza un thread con create_thread pasándole la función
    //       request_handler y el socket de conexión aceptado (por valor,
    //       como (void*)(long)s_conec).
    //
    // NOTA: El thread servidor no termina nunca (bucle infinito).
    // El socket de servicio NO se cierra aquí, solo los de conexión.
    // Fíjate en el main de server.c para ver el patrón exacto.
    // ============================================================
    int s = (long)arg;
    int s_conec;
    unsigned int addr_size;
    struct sockaddr_in clnt_addr;
    while(1){
        addr_size=sizeof(clnt_addr);
        if ((s_conec=accept(s, (struct sockaddr *)&clnt_addr, &addr_size))<0){
            perror("error en accept"); close(s); return 0;
        }
        create_thread(request_handler, (void *)(long)s_conec);
    }
    return NULL;
}
