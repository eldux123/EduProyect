/* EduChain - professionals.h
 * Modulo 1: registro de profesionales.
 */
#ifndef EDUCHAIN_PROFESSIONALS_H
#define EDUCHAIN_PROFESSIONALS_H

#include "common.h"

/* Registra un profesional nuevo y activo con identificador new_id.
 * Permitido a ROLE_ADMIN y ROLE_HR. Evita duplicados y respeta capacidad.
 * Retorna EDU_OK o ERR_NULL/ERR_PERM/ERR_BAD_ID/ERR_DUPLICATE/ERR_FULL. */
int professional_register(educhain_state_t *st, role_t caller_role, int new_id);

/* Busca por id. Retorna indice 0..pros_count-1 o -1 si no existe. Solo lectura. */
int professional_find(const educhain_state_t *st, int id);

/* Retorna 1 si el profesional existe y esta activo, 0 en caso contrario. Solo lectura. */
int professional_is_active(const educhain_state_t *st, int id);

#endif /* EDUCHAIN_PROFESSIONALS_H */
