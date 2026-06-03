// FUNCIONALIDAD DE LA PARTE CLIENTE
#include <sys/mman.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/stat.h>

#include "ring.h"
#include "common.h"

static int is_initialized(void);
static int initialize(void);

// ============================================================
// TODO [FASE 1, Paso 1]: Declara aquí las variables estáticas
// (globales de módulo) que necesitas para guardar el estado del nodo:
//   - Directorio compartido (string, copia del parámetro shrd_dir)
//   - IP local (unsigned int, formato red)
//   - Puerto local (unsigned short, formato red) — lo devuelve create_socket_srv
//   - IP del sucesor (unsigned int, formato red)
//   - Puerto del sucesor (unsigned short, formato red)
// Estas variables deben ser accesibles desde todas las funciones
// de este fichero.
// ============================================================
static char *DirectorioCompartido;
static unsigned int IPlocal;
static unsigned short PuertoLocal;
static unsigned int IPsucesor;
static unsigned short PuertoSucesor;

// inicia el nodo añadiéndolo a la red P2P si ya está creada;
// los puertos e IPs deben estar en formato de red;
// debe devolver en el último parámetro el puerto reservado en formato red;
// retorna 0 si OK y -1 si error
int ring_init(const char *shrd_dir, unsigned int local_ip, unsigned int remote_ip, unsigned short remote_port, unsigned short *alloc_port) {
    if (initialize()) return -1; // ya está inicializada

    // ============================================================
    // TODO [FASE 1, Paso 1 - ring_init]:
    //
    // 1. Guarda los parámetros recibidos en las variables estáticas:
    //    - Copia shrd_dir en la variable de directorio compartido
    //      (usa strdup o malloc+strcpy; recuerda liberar si fuera necesario)
    //    - Guarda local_ip en la variable de IP local
    //
    // 2. Crea el socket de servicio llamando a create_socket_srv.
    //    - Esa función devuelve el descriptor del socket y escribe en
    //      el parámetro de salida el puerto asignado por el SO (ya en
    //      formato de red).
    //    - Guarda ese puerto en tu variable de puerto local.
    //    - Devuélvelo también en *alloc_port.
    //    - Si falla, retorna -1.
    //
    // 3. Inicializa el sucesor:
    //    - Si remote_ip == 0 (primer nodo del anillo), el sucesor
    //      eres tú mismo: guarda local_ip y el puerto local como sucesor.
    //    - Si remote_ip != 0, tienes que incorporarte al anillo:
    //      Llama a la operación de alta (OP_ADD_NODE) que implementarás
    //      en ring_srv.c. Conecta al nodo de contacto (remote_ip, remote_port),
    //      envía el código de operación y tu puerto de servicio, y recibe
    //      como respuesta la IP y el puerto que será tu sucesor.
    //      Guarda esa respuesta en tus variables de sucesor.
    //      Cierra el socket de conexión al terminar.
    //      Si algún paso falla, retorna -1.
    //
    // 4. Lanza el thread servidor con create_thread, pasándole como
    //    argumento el descriptor del socket de servicio (por valor,
    //    como (void*)(long)s_srv). La función a ejecutar es server_thread.
    //    Si falla, retorna -1.
    //
    // NOTA: Fíjate en el ejemplo server.c para ver el patrón de servidor.
    // NOTA: Para el alta, lee la Fase 2 Paso 1 del enunciado.
    // ============================================================
    DirectorioCompartido = strdup(shrd_dir);
    IPlocal = local_ip;
    int s_srv;
    if ((s_srv=create_socket_srv(&PuertoLocal)) < 0) return -1;
    *alloc_port = PuertoLocal;
    if(remote_ip == 0){
        IPsucesor = local_ip;
        PuertoSucesor = PuertoLocal;
    }
    else{
        int s;
        if ((s=create_socket_cln(remote_ip,remote_port)) < 0) return -1;
        int op = htonl(OP_ADD_NODE);
        struct iovec iov[2];
        iov[0].iov_base = &op;
        iov[0].iov_len  = sizeof(op);
        iov[1].iov_base = &PuertoLocal;
        iov[1].iov_len  = sizeof(PuertoLocal);
        writev(s, iov, 2);
        if (recv(s, &IPsucesor, sizeof(IPsucesor), MSG_WAITALL) <= 0) return -1;
        if (recv(s, &PuertoSucesor, sizeof(PuertoSucesor), MSG_WAITALL) <= 0) return -1;
        close(s);
    }
    if(create_thread(server_thread, (void*)(long)s_srv) != 0) return -1;
    return 0;
}



// función local que devuelve la IP y el puerto del nodo;
// retorna 0 si OK y -1 si error
int ring_self(unsigned int *ip, unsigned short *port) {
    if (!is_initialized()) return -1; // no está inicializada

    // ============================================================
    // TODO [FASE 1, Paso 1 - ring_self]:
    // Escribe en *ip y *port la IP y el puerto locales que guardaste
    // en ring_init. Recuerda que ambos ya están en formato de red.
    // ============================================================
    *ip = IPlocal;
    *port = PuertoLocal;
    return 0;
}

// devuelve el PID del nodo remoto especificado o -1 si error
int ring_remote_pid(unsigned int remote_ip, unsigned short remote_port) {
    if (!is_initialized()) return -1; // no está inicializada

    // ============================================================
    // TODO [FASE 1, Paso 2 - ring_remote_pid]:
    //
    // 1. Crea un socket cliente con create_socket_cln(remote_ip, remote_port).
    //    Si falla, retorna -1.
    //
    // 2. Envía solo el código de operación OP_GETPID al servidor remoto
    //    (sin parámetros adicionales). Usa send o write.
    //    Si falla, cierra el socket y retorna -1.
    //
    // 3. Recibe la respuesta: un entero con el PID.
    //    Usa recv con MSG_WAITALL para garantizar la recepción completa.
    //    Convierte de formato de red a formato host (ntohl).
    //    Si falla, cierra el socket y retorna -1.
    //
    // 4. Cierra el socket y retorna el PID recibido.
    //
    // NOTA: Mira los ejemplos client_writev.c y client_sendmore.c
    // para ver cómo conectar y enviar/recibir.
    // ============================================================
    int s_cln;
    if((s_cln = create_socket_cln(remote_ip, remote_port)) < 0) return -1;
    int op = htonl(OP_GETPID);
    if(send(s_cln, &op, sizeof(op), 0) <= 0){
        close(s_cln);
        return -1;
    }
    int PID;
    if (recv(s_cln, &PID, sizeof(PID), MSG_WAITALL) != sizeof(PID)){
        close(s_cln);
        return -1;
    }
    PID = ntohl(PID);
    close(s_cln);
    return PID;
}

// función local que devuelve la IP y el puerto del nodo sucesor;
// retorna 0 si OK y -1 si error
int ring_successor(unsigned int *ip, unsigned short *port) {
    if (!is_initialized()) return -1; // no está inicializada

    // ============================================================
    // TODO [FASE 2, Paso 1 - ring_successor]:
    // Igual que ring_self, pero devuelve la IP y puerto del sucesor
    // que guardaste en ring_init.
    // ============================================================
    *ip = IPsucesor;
    *port = PuertoSucesor;
    return 0;
}

// auxiliar para usarlo en OP_ADD_NODE
void set_successor(unsigned int ip, unsigned short port) {
    IPsucesor = ip;
    PuertoSucesor = port;
}


// devuelve la IP y el puerto del nodo sucesor del especificado;
// retorna 0 si OK y -1 si error
int ring_remote_successor(unsigned int remote_ip, unsigned short remote_port, unsigned int *suc_ip, unsigned short *suc_port) {
    if (!is_initialized()) return -1; // no está inicializada

    // ============================================================
    // TODO [FASE 2, Paso 2 - ring_remote_successor]:
    //
    // Igual que ring_remote_pid, pero:
    // - El código de operación es OP_GET_SUCCESSOR.
    // - La respuesta no es un entero sino dos valores: IP (unsigned int)
    //   y puerto (unsigned short), ambos en formato de red.
    // - Recíbelos y escríbelos en *suc_ip y *suc_port.
    //
    // NOTA: Los valores ya llegan en formato de red desde el servidor,
    // así que NO hay que convertirlos (main.c tampoco los convierte).
    // ============================================================
    int s;
    if ((s = create_socket_cln(remote_ip, remote_port)) < 0) return -1;
    int op = htonl(OP_GET_SUCCESSOR);
    if (send(s, &op, sizeof(op), 0) <= 0) { 
        close(s); 
        return -1; 
    }
    if (recv(s, suc_ip, sizeof(*suc_ip), MSG_WAITALL) != sizeof(*suc_ip)) { 
        close(s); 
        return -1; 
    }
    if (recv(s, suc_port, sizeof(*suc_port), MSG_WAITALL) != sizeof(*suc_port)) { 
        close(s); 
        return -1; 
    }
    close(s);
    return 0;
}

// devuelve la IP y el puerto del nodo sucesor del sucesor del especificado;
// retorna 0 si OK y -1 si error
int ring_remote_successor_successor(unsigned int remote_ip, unsigned short remote_port, unsigned int *suc_suc_ip, unsigned short *suc_suc_port) {
    if (!is_initialized()) return -1; // no está inicializada

    // ============================================================
    // TODO [FASE 2, Paso 3 - ring_remote_successor_successor]:
    //
    // Idéntico a ring_remote_successor en el lado cliente:
    // solo cambia el código de operación a OP_GET_SUC_SUC.
    //
    // La diferencia está en el servidor (ring_srv.c):
    // cuando recibe OP_GET_SUC_SUC, el servidor no responde directamente
    // sino que reenvía la petición de sucesor al siguiente nodo del anillo.
    // ============================================================
    int s;
    if ((s = create_socket_cln(remote_ip, remote_port)) < 0) return -1;
    int op = htonl(OP_GET_SUC_SUC);
    if (send(s, &op, sizeof(op), 0) <= 0) { 
        close(s); 
        return -1; 
    }
    if (recv(s, suc_suc_ip, sizeof(*suc_suc_ip), MSG_WAITALL) != sizeof(*suc_suc_ip)) { 
        close(s); 
        return -1; 
    }
    if (recv(s, suc_suc_port, sizeof(*suc_suc_port), MSG_WAITALL) != sizeof(*suc_suc_port)) { 
        close(s); 
        return -1; 
    }
    close(s);
    return 0;
}

char *getDirectorioCompartido(){
    return DirectorioCompartido;
}

// descarga el fichero del nodo especificado;
// retorna el tamaño del fichero si OK y -1 en caso de error
int ring_download(unsigned int remote_ip, unsigned short remote_port, const char *filename) {
    if (!is_initialized()) return -1; // no está inicializada

    // ============================================================
    // TODO [FASE 3 - ring_download]:
    //
    // 1. Calcula la longitud del nombre del fichero con strlen
    //    (sin incluir el '\0').
    //
    // 2. Crea socket cliente y conéctate al nodo remoto.
    //    Si falla, retorna -1.
    //
    // 3. Envía en un solo writev (o con MSG_MORE) para evitar
    //    fragmentación:
    //      a) El código de operación OP_DOWNLOAD
    //      b) La longitud del nombre del fichero (int, htonl)
    //      c) El nombre del fichero en sí (sin '\0')
    //    NO copies el nombre en un buffer intermedio (zerocopy).
    //
    // 4. Recibe del servidor el tamaño del fichero (int o long,
    //    decide qué tipo usas, en formato de red). Si es -1, indica
    //    error (fichero no encontrado). Cierra el socket y retorna -1.
    //
    // 5. Construye la ruta completa del fichero destino:
    //    directorio_compartido + "/" + filename.
    //    NO copies filename, construye la ruta con snprintf o similar.
    //
    // 6. Crea el fichero destino (open con O_CREAT|O_TRUNC|O_RDWR).
    //
    // 7. Ajusta el tamaño del fichero destino con ftruncate(fd, tamaño).
    //
    // 8. Proyecta el fichero en memoria con mmap(PROT_WRITE, MAP_SHARED).
    //    Fíjate en el ejemplo copy_read_mmap.c.
    //
    // 9. Cierra el descriptor del fichero (ya no hace falta tras mmap).
    //
    // 10. Recibe los datos del socket directamente sobre la zona mmap:
    //     recv(socket, puntero_mmap, tamaño, MSG_WAITALL).
    //
    // 11. Libera el mmap con munmap.
    //
    // 12. Cierra el socket y retorna el tamaño del fichero.
    //
    // NOTA: Mira el ejemplo copy_read_mmap.c como referencia para
    // los pasos 6-11, sustituyendo el read por el recv del socket.
    // ============================================================
    int namelen = strlen(filename);
    int len_net = htonl(namelen);
    int s;
    if ((s = create_socket_cln(remote_ip, remote_port)) < 0) return -1;
    int op = htonl(OP_DOWNLOAD);
    struct iovec iov[3];
    iov[0].iov_base = &op;
    iov[0].iov_len  = sizeof(op);
    iov[1].iov_base = &len_net;
    iov[1].iov_len  = sizeof(len_net);
    iov[2].iov_base = (void *)filename;
    iov[2].iov_len  = namelen;
    writev(s, iov, 3);
    int tam;
    if (recv(s, &tam, sizeof(tam), MSG_WAITALL) != sizeof(tam)) { 
        close(s); 
        return -1; 
    }
    tam = ntohl(tam);
    if (tam < 0) { 
        close(s); 
        return -1; 
    }
    char path[4096];
    snprintf(path, sizeof(path), "%s/%s", DirectorioCompartido, filename);
    int fd;
    fd = open(path, O_CREAT|O_TRUNC|O_RDWR, 0644);
    ftruncate(fd, tam);
    char *p;
    if ((p=mmap(0, tam, PROT_WRITE, MAP_SHARED, fd, 0))==MAP_FAILED){
        perror("mmap");
        close(fd); 
        return -1;
    }
    close(fd);
    recv(s, p, tam, MSG_WAITALL);
    munmap(p, tam);
    close(s);
    return tam;
}

// busca el fichero en el anillo dando un número máximo de saltos y devolviendo
// la IP y el puerto del nodo que lo contiene;
// retorna 0 si OK y -1 si error
int ring_lookup(const char *filename, int hops, unsigned int *ip, unsigned short *port) {
    if (!is_initialized()) return -1; // no está inicializada

    // ============================================================
    // TODO [FASE 4 - ring_lookup]:
    //
    // 1. Comprueba si el fichero existe en el directorio compartido LOCAL:
    //    Construye la ruta completa y usa open o access para comprobarlo.
    //    Si existe, escribe en *ip y *port tu propia IP y puerto (ring_self)
    //    y retorna 0.
    //
    // 2. Si no existe localmente y hops == 0, retorna -1 (no encontrado).
    //
    // 3. Si hops > 0, hay que preguntar al sucesor:
    //    - Obtén la IP y puerto del sucesor (ring_successor).
    //    - Conecta al sucesor con create_socket_cln.
    //    - Envía en un único writev (sin fragmentar):
    //        a) Código de operación OP_LOOKUP
    //        b) El número de saltos restantes (hops - 1), en formato red
    //        c) La longitud del nombre del fichero (int, htonl)
    //        d) El nombre del fichero (sin '\0')
    //      NO copies el nombre (zerocopy).
    //    - Recibe la respuesta: un código de resultado (int) que indica
    //      si se encontró o no, y si se encontró, la IP y puerto del nodo.
    //      Define cómo será el protocolo de respuesta (por ejemplo: -1 si
    //      no encontrado, o 0 + IP + port si encontrado).
    //    - Cierra el socket.
    //    - Si se encontró, escribe en *ip y *port y retorna 0.
    //    - Si no, retorna -1.
    //
    // NOTA: La función ring_lookup también puede ser llamada desde la
    // parte servidora (ring_srv.c) para propagar la búsqueda por el anillo.
    // Ese es el patrón de "recursividad distribuida" descrito en el enunciado.
    // ============================================================
    char path[4096];
    snprintf(path, sizeof(path), "%s/%s", DirectorioCompartido, filename);
    if(access(path, F_OK) == 0){
        ring_self(ip, port);
        return 0;
    }
    else{
        if(hops == 0){
            return -1;
        }
        else{
            unsigned int suc_ip;
            unsigned short suc_port;
            ring_successor(&suc_ip, &suc_port);
            int s = create_socket_cln(suc_ip, suc_port);
            int op = htonl(OP_LOOKUP);
            int hops_net = htonl(hops - 1);
            int namelen = strlen(filename);
            int len_net = htonl(namelen);
            struct iovec iov[4];
            iov[0].iov_base = &op;
            iov[0].iov_len  = sizeof(op);
            iov[1].iov_base = &hops_net;
            iov[1].iov_len  = sizeof(hops_net);
            iov[2].iov_base = &len_net;
            iov[2].iov_len  = sizeof(len_net);
            iov[3].iov_base = (void *)filename;
            iov[3].iov_len  = namelen;
            writev(s, iov, 4);
            int codigoRes;
            recv(s, &codigoRes, sizeof(codigoRes), MSG_WAITALL);
            if(codigoRes == -1){
                close(s);
                return -1;
            }
            else{
                recv(s, ip, sizeof(*ip), MSG_WAITALL);
                recv(s, port, sizeof(*port), MSG_WAITALL);
                close(s);
                return 0;
            }
            
        }
    }
    return 0;
}

// busca y descarga el fichero del nodo encontrado en el anillo que lo contiene;
// retorna el tamaño del fichero si OK y -1 en caso de error;
// ESTA FUNCIÓN YA ESTA COMPLETADA
int ring_get_file(const char *filename, int hops) {
    if (!is_initialized()) return -1; // no está inicializada
    unsigned int ip, ip_local;
    unsigned short port, port_local;
    int res = ring_lookup(filename, hops, &ip, &port);
    ring_self(&ip_local, &port_local);
    // realiza la descarga si encontrado en un nodo que no es el local
    if ((res!=-1) && ((ip!=ip_local) || (port!=port_local)))
        res = ring_download(ip, port, filename);
    return res;
}

// funciones auxiliares
static int initialized;

static int initialize(void) {
    return initialized?1:(initialized=1,0);
}
static int is_initialized(void) {
    return initialized;
}
