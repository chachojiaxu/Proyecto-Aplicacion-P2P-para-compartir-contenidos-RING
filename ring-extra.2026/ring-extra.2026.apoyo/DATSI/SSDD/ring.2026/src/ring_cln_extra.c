
// PARTE EXTRA
static int get_slices(int num_nodes, int slice_size, size_t file_size, off_t *slice_start, size_t *slice_length) {
    if (!(num_nodes > 0 && slice_size > 0 && file_size > 0 &&
           slice_start && slice_length)) return -1;
    size_t num_file_blocks = file_size / slice_size;
    int last_block_rem = file_size % slice_size;
    size_t blocks_per_node = num_file_blocks / num_nodes;
    int blocks_rem = num_file_blocks % num_nodes;
    size_t size, offset = 0;
    int n, end = 0;
    for (n=0; !end; n++) {
        slice_start[n] = offset * slice_size;
        size = blocks_per_node + ((n < blocks_rem) ? 1 : 0);
        slice_length[n] = size * slice_size;
        offset += size;
        end = offset >= num_file_blocks;
    }
    if (last_block_rem) slice_length[n-1]+=last_block_rem;
    return n;
}

// descarga el fichero en paralelo de los nodos especificados;
// retorna el tamaño del fichero si OK y -1 en caso de error
int ring_pdownload(int num_nodes, int slice_size, const unsigned int *remote_ips, const unsigned short *remote_ports, const char *filename) {
    if (!is_initialized()) return -1; // no está inicializada
    return 0;
}

// busca el fichero en el anillo, incluido localmente, dando un número
// máximo de saltos y devolviendo las IPs y los puertos de los nodos que
// lo contienen; retorna el número de nodos que lo contienen si OK y -1 si error
// NOTA: Dado que se comprueba la existencia del fichero en hops + 1
// nodos, el llamador debe especificar en los dos últimos parámetros
// dos vectores con tamaño igual hops + 1
int ring_mlookup(const char *filename, int hops, unsigned int *ips, unsigned short *ports) {
    if (!is_initialized()) return -1; // no está inicializada
    return 0;
}

// busca y descarga en paralelo el fichero de los nodos encontrados en el
// anillo que lo contienen siempre que no esté almacenado localmente;
// retorna el tamaño transferido si el fichero existe y -1 en caso de error
// ESTA FUNCIÓN YA ESTA COMPLETADA
int ring_pget_file(const char *filename, int hops, int slice_size) {
    if (!is_initialized()) return -1; // no está inicializada
    unsigned int ips[hops+1], ip_local;
    unsigned short ports[hops+1], port_local;
    int res;
    if ((res=ring_mlookup(filename, hops, ips, ports)) <= 0) return -1;
    ring_self(&ip_local, &port_local);
    for (int i=0; i<res; i++) // si copia local no hace nada
        if ((ips[i]==ip_local) && (ports[i]==port_local)) return 0;
    return ring_pdownload(res, slice_size, ips, ports, filename);
}

