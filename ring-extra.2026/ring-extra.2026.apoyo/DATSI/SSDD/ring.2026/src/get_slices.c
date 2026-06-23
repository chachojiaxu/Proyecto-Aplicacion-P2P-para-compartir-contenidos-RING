#include <stdio.h>
#include <stdlib.h>

// Dados un número de nodos, un tamaño de rodaja mínimo y un tamaño de fichero,
// devuelve en los dos últimos parámetros el offset y el tamaño de la rodaja
// que le corresponde a cada nodo, retornando cuántos nodos están involucrados.
// NOTA: Para evitar la ineficiencia por descargas demasiado pequeñas,
// asegura que a ningún nodo le corresponde un tamaño menor que la rodaja
// mínima (excepto si el tamaño del fichero es menor que esa rodaja mínima),
// lo que puede conllevar que no todos los nodos estén involucrados.
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
        
int main(int argc, char *argv[]) {
    if (argc!=4) {
        fprintf(stderr, "usage: %s num_nodes slice_size file_size\n", argv[0]);
        return 1;
    }
    int num_nodes = atoi(argv[1]), slice_size = atoi(argv[2]);
    size_t file_size = atol(argv[3]);
    off_t slice_start[num_nodes];
    size_t slice_length[num_nodes];
    int n=get_slices(num_nodes, slice_size, file_size, slice_start, slice_length);
    for (int i = 0; i < n; i++) 
        printf("node %d slice_start %ld slice_length %ld\n",
                i, slice_start[i], slice_length[i]); 

    return 0;
}

