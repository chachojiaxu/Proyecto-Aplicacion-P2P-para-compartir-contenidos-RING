# RING 2026 — Red P2P en anillo para compartir ficheros

Práctica de Sistemas Distribuidos (SSDD) — DATSI, UPM  
Implementación en C de una red P2P estructurada en anillo para compartir ficheros entre nodos.

## Descripción

Cada nodo del anillo actúa simultáneamente como cliente y servidor TCP. Los nodos se organizan en un anillo unidireccional: cada nodo conoce únicamente a su sucesor. La búsqueda de ficheros se propaga por el anillo saltando de nodo en nodo hasta encontrar el fichero o agotar los saltos permitidos.

## Arquitectura

```
Nodo A → Nodo B → Nodo C → Nodo A
```

- **Servidor multihilo**: un thread principal acepta conexiones y lanza un thread por cada petición.
- **Protocolo TCP** con operaciones codificadas como enteros en formato de red (`htonl`/`ntohl`).
- **Zerocopy**: envíos múltiples con `writev`/`MSG_MORE`, envío de ficheros con `sendfile`, recepción con `mmap`.

## Operaciones del protocolo

| Código | Operación | Descripción |
|--------|-----------|-------------|
| `OP_GETPID` (1) | Remote PID | Devuelve el PID del proceso remoto |
| `OP_ADD_NODE` (2) | Alta de nodo | Incorpora un nuevo nodo al anillo |
| `OP_GET_SUCCESSOR` (3) | Sucesor remoto | Devuelve el sucesor del nodo remoto |
| `OP_GET_SUC_SUC` (4) | Sucesor del sucesor | Devuelve el sucesor del sucesor (reenvío) |
| `OP_DOWNLOAD` (5) | Descarga | Descarga un fichero de un nodo remoto |
| `OP_LOOKUP` (6) | Búsqueda | Busca un fichero en el anillo |

## API pública

```c
// Inicializa el nodo y lo incorpora al anillo (remote_ip=0 si es el primero)
int ring_init(const char *shared_dir, unsigned int local_ip,
              unsigned int remote_ip, unsigned short remote_port,
              unsigned short *alloc_port);

// Devuelve la IP y puerto propios
int ring_self(unsigned int *ip, unsigned short *port);

// Devuelve el sucesor local
int ring_successor(unsigned int *ip, unsigned short *port);

// Busca un fichero en el anillo (máximo hops saltos)
int ring_lookup(const char *filename, int hops, unsigned int *ip, unsigned short *port);

// Busca y descarga un fichero del anillo
int ring_get_file(const char *filename, int hops);
```

Todas las IPs y puertos se manejan en **formato de red** (big-endian).

## Estructura del proyecto

```
src/
├── ring_cln.c          # Lógica cliente: ring_init, ring_lookup, ring_download, ...
├── ring_srv.c          # Lógica servidor: request_handler, server_thread
├── common.c            # Utilidades: create_socket_srv, create_socket_cln, create_thread
├── main.c              # Punto de entrada
├── include/
│   ├── ring.h          # API pública
│   └── common.h        # Códigos de operación y prototipos internos
└── Makefile
```

## Compilación

```bash
make
```

## Uso

```bash
./ring <directorio_compartido> [<ip_contacto> <puerto_contacto>]
```

- Sin `ip_contacto`: crea un anillo nuevo (primer nodo).
- Con `ip_contacto`: se une al anillo existente a través del nodo de contacto.

## Entrega

```bash
entrega.sd ring.2026
```

En el servidor `triqui1.fi.upm.es` (usuario `jiaxu.he`).

---

## Parte extra (+4 pts)

### Nuevas operaciones del protocolo

| Código | Operación | Descripción |
|--------|-----------|-------------|
| `OP_MLOOKUP` (7) | Multi-lookup | Busca el fichero en todos los nodos del anillo |
| `OP_DOWNLOAD_SLICE` (8) | Descarga rodaja | Descarga un fragmento del fichero (offset + tamaño) |

### Funciones a implementar

**`ring_mlookup` (2 pts)** — Busca el fichero en todos los nodos visitados (hasta `hops+1`) y devuelve arrays con las IPs y puertos de todos los que lo tienen.

- [ ] Cliente (`ring_cln.c`): comprueba local, si `hops > 0` envía `OP_MLOOKUP` al sucesor, combina resultados
- [ ] Servidor (`ring_srv.c`): handler `OP_MLOOKUP` — llama a `ring_mlookup` recursivamente y devuelve lista de nodos

**`ring_pdownload` (2 pts)** — Descarga el fichero en paralelo de varios nodos usando `epoll` (sin threads).

- [ ] Servidor (`ring_srv.c`): handler `OP_DOWNLOAD_SLICE` — recibe filename + offset + size, responde con `sendfile` desde el offset
- [ ] Cliente (`ring_cln.c`): obtiene tamaño del fichero, calcula rodajas con `get_slices`, abre sockets a cada nodo, registra en `epoll`, recibe datos en paralelo sobre `mmap`

### Ya implementado (no tocar)
- `get_slices()` — calcula qué rodaja le toca a cada nodo
- `ring_pget_file()` — orquesta `ring_mlookup` + `ring_pdownload`
