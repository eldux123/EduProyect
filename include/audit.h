/* EduChain - audit.h
 * Modulo 4: historial de operaciones append-only.
 */
#ifndef EDUCHAIN_AUDIT_H
#define EDUCHAIN_AUDIT_H

#include "common.h"

/* Anade un evento al final del log. El seq es estrictamente creciente.
 * No modifica eventos previos (append-only). Retorna EDU_OK o ERR_NULL/ERR_FULL. */
/*@
  requires \valid(st);
  requires 0 <= st->log_count <= MAX_EVENTS;
  assigns st->log[st->log_count], st->log_count;
  // Frame explicito: auditar NO toca el resto del estado (ayuda a los llamadores
  // a probar la preservacion de pros_count/exps/creds tras auditar).
  ensures st->pros_count  == \old(st->pros_count);
  ensures st->exps_count  == \old(st->exps_count);
  ensures st->creds_count == \old(st->creds_count);
  behavior full:
    assumes st->log_count >= MAX_EVENTS;
    assigns \nothing;
    ensures \result == ERR_FULL;
    ensures st->log_count == \old(st->log_count);
  behavior ok:
    assumes st->log_count < MAX_EVENTS;
    ensures \result == EDU_OK || \result == ERR_FULL;
    // Si exitoso, el log crecio exactamente en 1 (append, P-12).
    ensures \result == EDU_OK ==> st->log_count == \old(st->log_count) + 1;
    // Atomicidad: si ERR_FULL (saturacion de seq), el log no cambia.
    ensures \result == ERR_FULL ==> st->log_count == \old(st->log_count);
    // P-12: seq estrictamente creciente respecto al ultimo evento.
    ensures (\result == EDU_OK && \old(st->log_count) > 0) ==>
            st->log[\old(st->log_count)].seq == st->log[\old(st->log_count)-1].seq + 1;
    ensures (\result == EDU_OK && \old(st->log_count) == 0) ==>
            st->log[0].seq == 1;
  complete behaviors;
  disjoint behaviors;
*/
int audit_record_event(educhain_state_t *st, int actor_id, int op_code,
                        int target_id, int result);

/* Busca un evento por seq. Retorna indice 0..log_count-1 o -1. Solo lectura. */
/*@ requires \valid_read(st);
    requires 0 <= st->log_count <= MAX_EVENTS;
    assigns \nothing;
    ensures -1 <= \result < st->log_count;
    ensures \result >= 0 ==> st->log[\result].seq == seq;
    ensures \result == -1 ==>
            \forall integer k; 0 <= k < st->log_count ==> st->log[k].seq != seq;
*/
int audit_verify_event(const educhain_state_t *st, int seq);

/* Retorna la cantidad de eventos registrados. Solo lectura. */
/*@ requires \valid_read(st);
    requires 0 <= st->log_count <= MAX_EVENTS;
    assigns \nothing;
    ensures \result == st->log_count;
*/
int audit_get_count(const educhain_state_t *st);

#endif /* EDUCHAIN_AUDIT_H */
