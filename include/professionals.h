/* EduChain - professionals.h
 * Modulo 1: registro de profesionales.
 */
#ifndef EDUCHAIN_PROFESSIONALS_H
#define EDUCHAIN_PROFESSIONALS_H

#include "common.h"

/* Registra un profesional nuevo y activo con identificador new_id.
 * caller_id identifica al usuario que ejecuta (autenticado por capa externa;
 * no requiere existir en professionals[]). Se audita como actor_id.
 * Permitido a ROLE_ADMIN y ROLE_HR. Evita duplicados y respeta capacidad.
 * Retorna EDU_OK o ERR_NULL/ERR_PERM/ERR_BAD_ID/ERR_DUPLICATE/ERR_FULL. */
/*@
  requires \valid(st);
  requires 0 <= st->pros_count <= MAX_PROFESSIONALS;
  requires 0 <= st->log_count  <= MAX_EVENTS;
  assigns st->pros[st->pros_count], st->pros_count,
          st->log[st->log_count], st->log_count;
  ensures \result == EDU_OK || \result == ERR_PERM || \result == ERR_BAD_ID
       || \result == ERR_DUPLICATE || \result == ERR_FULL;
  // P-1: rol no autorizado => ERR_PERM y conteo sin cambios (atomicidad BR-10).
  ensures (caller_role != ROLE_ADMIN && caller_role != ROLE_HR)
          ==> (\result == ERR_PERM && st->pros_count == \old(st->pros_count));
  // P-9/P-10: en exito, el conteo crece en 1 y sigue dentro de la cota.
  ensures \result == EDU_OK ==> st->pros_count == \old(st->pros_count) + 1;
  // Atomicidad: en cualquier error, el conteo no cambia (BR-10).
  ensures \result != EDU_OK ==> st->pros_count == \old(st->pros_count);
  ensures st->pros_count <= MAX_PROFESSIONALS;
*/
int professional_register(educhain_state_t *st, role_t caller_role, int caller_id, int new_id);

/* Busca por id. Retorna indice 0..pros_count-1 o -1 si no existe. Solo lectura. */
/*@ requires \valid_read(st); requires 0 <= st->pros_count <= MAX_PROFESSIONALS;
    assigns \nothing;
    ensures -1 <= \result < st->pros_count; */
int professional_find(const educhain_state_t *st, int id);

/* Retorna 1 si el profesional existe y esta activo, 0 en caso contrario. Solo lectura. */
/*@ requires \valid_read(st); requires 0 <= st->pros_count <= MAX_PROFESSIONALS;
    assigns \nothing;
    ensures \result == 0 || \result == 1; */
int professional_is_active(const educhain_state_t *st, int id);

#endif /* EDUCHAIN_PROFESSIONALS_H */
