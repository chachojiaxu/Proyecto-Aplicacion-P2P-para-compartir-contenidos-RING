#!/bin/bash
# Script para subir los ficheros del proyecto a triqui
# USO: ./subir_triqui.sh

REMOTE_USER="jiaxu.he"
REMOTE_HOST="triqui1.fi.upm.es"
REMOTE_DIR="~/DATSI/SSDD/ring.2026/src"
LOCAL_SRC="$(dirname "$0")/ring-2026/DATSI/SSDD/ring.2026/src"

GREEN='\033[0;32m'; YELLOW='\033[1;33m'; RED='\033[0;31m'; NC='\033[0m'

# Socket de control: una sola conexión SSH compartida por todas las operaciones
CTRL="/tmp/ring_ssh_ctrl_$$"
SSH_OPTS="-o StrictHostKeyChecking=no -o ConnectTimeout=10"
CTRL_OPTS="-o ControlPath=$CTRL"

ssh_cmd() { ssh $SSH_OPTS $CTRL_OPTS "${REMOTE_USER}@${REMOTE_HOST}" "$1"; }
scp_cmd() { scp -q $SSH_OPTS $CTRL_OPTS "$1" "$2"; }

echo -e "${GREEN}================================================${NC}"
echo -e "${GREEN}   Subiendo proyecto RING a triqui              ${NC}"
echo -e "${GREEN}   ${REMOTE_USER}@${REMOTE_HOST}               ${NC}"
echo -e "${GREEN}================================================${NC}"
echo ""

# ── Abrir conexión maestra (única vez que se pide contraseña) ──
echo -e "${YELLOW}Conectando a triqui (introduce la contraseña una sola vez)...${NC}"
ssh $SSH_OPTS -M -S "$CTRL" -o ControlPersist=120 \
    "${REMOTE_USER}@${REMOTE_HOST}" "echo '    Conexión establecida'" || {
    echo -e "${RED}Error al conectar${NC}"; exit 1
}

# ── 1. Crear directorios ───────────────────────────────────────
echo -e "${YELLOW}[1/3] Creando directorios en triqui...${NC}"
ssh_cmd "mkdir -p ${REMOTE_DIR}/include"
echo -e "${GREEN}    OK${NC}"

# ── 2. Subir ficheros ──────────────────────────────────────────
echo -e "${YELLOW}[2/3] Subiendo ficheros fuente...${NC}"

FILES=(
    "ring_cln.c"
    "ring_srv.c"
    "common.c"
    "Makefile"
    "main.c"
    "include/common.h"
    "include/ring.h"
)

for f in "${FILES[@]}"; do
    src="${LOCAL_SRC}/${f}"
    dst_subdir=$(dirname "$f")
    if [ "$dst_subdir" = "." ]; then
        dst="${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_DIR}/"
    else
        dst="${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_DIR}/${dst_subdir}/"
    fi

    if [ -f "$src" ]; then
        scp_cmd "$src" "$dst" && \
            echo -e "    ${GREEN}✓${NC} $f" || \
            echo -e "    ${RED}✗${NC} $f (error al subir)"
    else
        echo -e "    ${YELLOW}⚠${NC}  $f (no existe localmente, saltando)"
    fi
done

# ── 3. Compilar en remoto ──────────────────────────────────────
echo ""
echo -e "${YELLOW}[3/3] Compilando en triqui...${NC}"
ssh_cmd "cd ${REMOTE_DIR} && make clean && make 2>&1"

# ── Cerrar conexión maestra ────────────────────────────────────
ssh -S "$CTRL" -O exit "${REMOTE_USER}@${REMOTE_HOST}" 2>/dev/null

echo ""
echo -e "${GREEN}================================================${NC}"
echo -e "${GREEN}   Hecho.                                       ${NC}"
echo -e "${GREEN}================================================${NC}"
echo ""
echo -e "Para conectarte manualmente:"
echo -e "  ssh ${REMOTE_USER}@${REMOTE_HOST}"
echo -e "  cd ${REMOTE_DIR}"
