/* EduChain - audit.h
 * Modulo 4: historial de operaciones append-only.
 */
#ifndef EDUCHAIN_AUDIT_H
#define EDUCHAIN_AUDIT_H

#include "common.h"

/* Anade un evento al final del log. El seq es estrictamente creciente.
 * No modifica eventos previos (append-only). Retorna EDU_OK o ERR_NULL/ERR_FULL. */
int audit_record_event(educhain_state_t *st, int actor_id, int op_code,
                        int target_id, int result);

/* Busca un evento por seq. Retorna indice 0..log_count-1 o -1. Solo lectura. */
int audit_verify_event(const educhain_state_t *st, int seq);

/* Retorna la cantidad de eventos registrados. Solo lectura. */
int audit_get_count(const educhain_state_t *st);

#endif /* EDUCHAIN_AUDIT_H */
