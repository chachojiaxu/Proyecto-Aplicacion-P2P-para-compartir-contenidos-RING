// API DE LA PARTE CLIENTE
#ifndef _RING_H
#define _RING_H        1

// inicia el nodo añadiéndolo a la red P2P si ya está creada;
// los puertos e IPs deben estar en formato de red;
// debe devolver en el último parámetro el puerto reservado en formato red;
// retorna 0 si OK y -1 si error
int ring_init(const char *shared_dir, unsigned int local_ip, unsigned int remote_ip, unsigned short remote_port, unsigned short *alloc_port);

// función local que devuelve la IP y el puerto del nodo
int ring_self(unsigned int *ip, unsigned short *port);

// devuelve el PID del nodo remoto especificado o -1 si error
int ring_remote_pid(unsigned int remote_ip, unsigned short remote_port);

// función local que devuelve la IP y el puerto del nodo sucesor
int ring_successor(unsigned int *ip, unsigned short *port);

// devuelve la IP y el puerto del nodo sucesor del especificado;
// retorna 0 si OK y -1 si error
int ring_remote_successor(unsigned int remote_ip, unsigned short remote_port, unsigned int *suc_ip, unsigned short *suc_port);

// devuelve la IP y el puerto del nodo sucesor del sucesor del especificado;
// retorna 0 si OK y -1 si error
int ring_remote_successor_successor(unsigned int remote_ip, unsigned short remote_port, unsigned int *suc_suc_ip, unsigned short *suc_suc_port);

// descarga el fichero del nodo especificado;
// retorna el tamaño del fichero si OK y -1 en caso de error
int ring_download(unsigned int remote_ip, unsigned short remote_port, const char *filename);

// busca el fichero en el anillo dando un número máximo de saltos y devolviendo
// la IP y el puerto del nodo que lo contiene;
// retorna 0 si OK y -1 si error
int ring_lookup(const char *filename, int hops, unsigned int *ip, unsigned short *port);

// busca y descarga el fichero del nodo encontrado en el anillo que lo contiene;
// retorna el tamaño del fichero si OK y -1 en caso de error
int ring_get_file(const char *filename, int hops);

// PARTE EXTRA
// descarga el fichero en paralelo de los nodos especificados;
// retorna el tamaño del fichero si OK y -1 en caso de error
int ring_pdownload(int num_nodes, int slice_size, const unsigned int *remote_ips, const unsigned short *remote_ports, const char *filename);

// busca el fichero en el anillo, incluido localmente, dando un número
// máximo de saltos y devolviendo las IPs y los puertos de los nodos que
// lo contienen; retorna el número de nodos que lo contienen si OK y -1 si error
// NOTA: Dado que se comprueba la existencia del fichero en hops + 1
// nodos, el llamador debe especificar en los dos últimos parámetros
// dos vectores con tamaño igual hops + 1

int ring_mlookup(const char *filename, int hops, unsigned int *ips, unsigned short *ports);

// busca y descarga en paralelo el fichero de los nodos encontrados en el
// anillo que lo contienen siempre que no esté almacenado localmente;
// retorna el tamaño transferido si el fichero existe y -1 en caso de error
int ring_pget_file(const char *filename, int hops, int slice_size);

#endif // _RING_H
