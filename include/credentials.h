/* EduChain - credentials.h
 * Modulo 3: credenciales digitales como maquina de estados.
 * Estados: PENDING -> ACTIVE -> BLOCKED <-> ACTIVE -> REVOKED (terminal).
 */
#ifndef EDUCHAIN_CREDENTIALS_H
#define EDUCHAIN_CREDENTIALS_H

#include "common.h"

/* caller_id identifica al usuario que ejecuta (autenticado por capa externa).
 * No requiere existir en professionals[]. Se audita como actor_id del evento. */

/* Emite una credencial en CRED_PENDING para owner_id (una por profesional).
 * Permitido a ROLE_COORD. */
/*@
  requires \valid(st);
  requires 0 <= st->creds_count <= MAX_CREDENTIALS;
  requires 0 <= st->log_count   <= MAX_EVENTS;
  requires 0 <= st->pros_count  <= MAX_PROFESSIONALS;
  ensures \result == EDU_OK || \result == ERR_PERM || \result == ERR_BAD_ID
       || \result == ERR_NOT_FOUND || \result == ERR_DUPLICATE || \result == ERR_FULL;
  // P-1: solo COORD emite.
  ensures (caller_role != ROLE_COORD) ==> \result == ERR_PERM;
  // Atomicidad (BR-10).
  ensures \result == EDU_OK ==> st->creds_count == \old(st->creds_count) + 1;
  ensures \result != EDU_OK ==> st->creds_count == \old(st->creds_count);
  ensures st->creds_count <= MAX_CREDENTIALS;

  // #5: assigns partido por behavior segun haya o no espacio. La escritura solo
  // ocurre cuando existe hueco (creds_count < MAX && log_count < MAX): el indice
  // escrito es \old(creds_count) (en rango) y log[\old(log_count)] (en rango).
  // Cuando no hay hueco (rama ERR_FULL) no se escribe nada: asi el assigns no
  // nombra nunca creds[MAX]/log[MAX] fuera de rango (origen de *_assigns_part*).
  behavior space:
    assumes st->creds_count < MAX_CREDENTIALS && st->log_count < MAX_EVENTS;
    assigns st->creds[st->creds_count], st->creds_count,
            st->log[st->log_count], st->log_count;
  behavior no_space:
    assumes st->creds_count >= MAX_CREDENTIALS || st->log_count >= MAX_EVENTS;
    assigns \nothing;
  complete behaviors;
  disjoint behaviors space, no_space;
*/
int credential_issue(educhain_state_t *st, role_t caller_role, int caller_id,
                     int cred_id, int owner_id);

/* PENDING -> ACTIVE. Permitido a ROLE_COORD. */
/*@ requires \valid(st);
    requires 0 <= st->creds_count <= MAX_CREDENTIALS;
    requires 0 <= st->log_count   <= MAX_EVENTS;
    ensures \result == EDU_OK || \result == ERR_PERM || \result == ERR_BAD_ID
         || \result == ERR_NOT_FOUND || \result == ERR_BAD_TRANSITION || \result == ERR_FULL;
    ensures (caller_role != ROLE_COORD) ==> \result == ERR_PERM;   // P-1
    ensures st->creds_count == \old(st->creds_count);
    // P-7 (transicion legal PENDING->ACTIVE), derivada del contrato de cred_transition:
    // en exito, la credencial cred_id estaba PENDING y queda ACTIVE.
    ensures \result == EDU_OK ==>
            (\exists integer k; 0 <= k < st->creds_count && st->creds[k].id == cred_id
             && \old(st->creds[k].state) == CRED_PENDING && st->creds[k].state == CRED_ACTIVE);
    // Wrapper delgado: delega en cred_transition. El assigns coincide con el
    // frame-union que WP deriva del callee (set-comprehension sobre creds[k].state
    // + log). No se parte por behavior: WP usa el frame-union del callee en todas
    // las ramas, asi que un `assigns \nothing` en error seria irrefutable para WP.
    // El #5 (particion exito/error) se aplica donde SI hay sub-objetivos
    // *_assigns_part*: credential_issue y la auxiliar cred_transition.
    assigns { st->creds[k].state | integer k;
              0 <= k < st->creds_count && st->creds[k].id == cred_id },
            st->log[st->log_count], st->log_count;
*/
int credential_activate(educhain_state_t *st, role_t caller_role, int caller_id, int cred_id);

/* ACTIVE -> BLOCKED. Permitido a ROLE_ADMIN/ROLE_COORD. */
/*@ requires \valid(st);
    requires 0 <= st->creds_count <= MAX_CREDENTIALS;
    requires 0 <= st->log_count   <= MAX_EVENTS;
    ensures \result == EDU_OK || \result == ERR_PERM || \result == ERR_BAD_ID
         || \result == ERR_NOT_FOUND || \result == ERR_BAD_TRANSITION || \result == ERR_FULL;
    ensures (caller_role != ROLE_ADMIN && caller_role != ROLE_COORD) ==> \result == ERR_PERM;
    ensures st->creds_count == \old(st->creds_count);
    // P-7 (transicion legal ACTIVE->BLOCKED): en exito cred_id estaba ACTIVE y queda BLOCKED.
    ensures \result == EDU_OK ==>
            (\exists integer k; 0 <= k < st->creds_count && st->creds[k].id == cred_id
             && \old(st->creds[k].state) == CRED_ACTIVE && st->creds[k].state == CRED_BLOCKED);
    // Wrapper delgado (ver nota en credential_activate): assigns = frame-union del callee.
    assigns { st->creds[k].state | integer k;
              0 <= k < st->creds_count && st->creds[k].id == cred_id },
            st->log[st->log_count], st->log_count;
*/
int credential_block(educhain_state_t *st, role_t caller_role, int caller_id, int cred_id);

/* BLOCKED -> ACTIVE. Permitido a ROLE_ADMIN/ROLE_COORD. */
/*@ requires \valid(st);
    requires 0 <= st->creds_count <= MAX_CREDENTIALS;
    requires 0 <= st->log_count   <= MAX_EVENTS;
    ensures \result == EDU_OK || \result == ERR_PERM || \result == ERR_BAD_ID
         || \result == ERR_NOT_FOUND || \result == ERR_BAD_TRANSITION || \result == ERR_FULL;
    ensures (caller_role != ROLE_ADMIN && caller_role != ROLE_COORD) ==> \result == ERR_PERM;
    ensures st->creds_count == \old(st->creds_count);
    // P-7 (transicion legal BLOCKED->ACTIVE): en exito cred_id estaba BLOCKED y queda ACTIVE.
    ensures \result == EDU_OK ==>
            (\exists integer k; 0 <= k < st->creds_count && st->creds[k].id == cred_id
             && \old(st->creds[k].state) == CRED_BLOCKED && st->creds[k].state == CRED_ACTIVE);
    // Wrapper delgado (ver nota en credential_activate): assigns = frame-union del callee.
    assigns { st->creds[k].state | integer k;
              0 <= k < st->creds_count && st->creds[k].id == cred_id },
            st->log[st->log_count], st->log_count;
*/
int credential_reactivate(educhain_state_t *st, role_t caller_role, int caller_id, int cred_id);

/* {PENDING,ACTIVE,BLOCKED} -> REVOKED (terminal). Permitido a ROLE_ADMIN/ROLE_COORD. */
/*@ requires \valid(st);
    requires 0 <= st->creds_count <= MAX_CREDENTIALS;
    requires 0 <= st->log_count   <= MAX_EVENTS;
    ensures \result == EDU_OK || \result == ERR_PERM || \result == ERR_BAD_ID
         || \result == ERR_NOT_FOUND || \result == ERR_BAD_TRANSITION || \result == ERR_FULL;
    ensures (caller_role != ROLE_ADMIN && caller_role != ROLE_COORD) ==> \result == ERR_PERM;
    ensures st->creds_count == \old(st->creds_count);
    // -------------------------------------------------------------------------
    // #4 P-8 (REVOKED terminal), anclado TODO en \old, con las dos mitades:
    //  (negativa) si cred_id ya estaba REVOKED antes de la llamada => se rechaza
    //             con ERR_BAD_TRANSITION (nunca re-revoca un terminal).
    ensures (\exists integer k; 0 <= k < \old(st->creds_count)
             && st->creds[k].id == cred_id && \old(st->creds[k].state) == CRED_REVOKED)
            ==> \result == ERR_BAD_TRANSITION;
    //  (positiva) en exito, la credencial cred_id (que por la mitad negativa NO
    //             estaba REVOKED) queda REVOKED. Pre con \old, post sin \old,
    //             ambos para la misma k con id == cred_id.
    ensures \result == EDU_OK ==>
            (\exists integer k; 0 <= k < st->creds_count && st->creds[k].id == cred_id
             && \old(st->creds[k].state) != CRED_REVOKED
             && st->creds[k].state == CRED_REVOKED);
    // -------------------------------------------------------------------------
    // #5: assigns partido por behavior de exito vs. error.
    behavior success:
      assumes st->log_count < MAX_EVENTS;
      assigns { st->creds[k].state | integer k;
                0 <= k < st->creds_count && st->creds[k].id == cred_id },
              st->log[st->log_count], st->log_count;
    behavior log_full:
      // Rama ERR_FULL (log lleno): no se escribe el log (evita nombrar log[MAX]
      // fuera de rango). El frame del set-comprehension sobre creds[k].state se
      // conserva porque es SIEMPRE en rango y coincide con el frame-union que WP
      // deriva del callee cred_transition en el call-site de los wrappers.
      assumes st->log_count >= MAX_EVENTS;
      assigns { st->creds[k].state | integer k;
                0 <= k < st->creds_count && st->creds[k].id == cred_id };
    complete behaviors;
    disjoint behaviors success, log_full;
*/
int credential_revoke(educhain_state_t *st, role_t caller_role, int caller_id, int cred_id);

/* Retorna el estado (cred_state_t) de la credencial, o -1 si no existe. Solo lectura. */
/*@ requires \valid_read(st);
    requires 0 <= st->creds_count <= MAX_CREDENTIALS;
    assigns \nothing;
    ensures \result == -1 || \result == CRED_PENDING || \result == CRED_ACTIVE
         || \result == CRED_BLOCKED || \result == CRED_REVOKED;
    ensures \result >= 0 ==>
            (\exists integer k; 0 <= k < st->creds_count
             && st->creds[k].id == cred_id && st->creds[k].state == \result);
*/
int credential_status(const educhain_state_t *st, int cred_id);

/* Retorna 1 si la credencial es valida para uso (solo CRED_ACTIVE), 0 en otro caso. Solo lectura. */
/*@ requires \valid_read(st); requires 0 <= st->creds_count <= MAX_CREDENTIALS;
    assigns \nothing;
    ensures \result == 0 || \result == 1;
    // P-6: valida <=> existe una credencial con ese id en estado ACTIVE.
    ensures \result == 1 <==>
            (\exists integer k; 0 <= k < st->creds_count
             && st->creds[k].id == cred_id && st->creds[k].state == CRED_ACTIVE);
*/
int credential_is_valid(const educhain_state_t *st, int cred_id);

#endif /* EDUCHAIN_CREDENTIALS_H */
