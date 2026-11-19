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
  assigns st->creds[st->creds_count], st->creds_count,
          st->log[st->log_count], st->log_count;
  ensures \result == EDU_OK || \result == ERR_PERM || \result == ERR_BAD_ID
       || \result == ERR_NOT_FOUND || \result == ERR_DUPLICATE || \result == ERR_FULL;
  // P-1: solo COORD emite.
  ensures (caller_role != ROLE_COORD) ==> \result == ERR_PERM;
  // Atomicidad (BR-10).
  ensures \result == EDU_OK ==> st->creds_count == \old(st->creds_count) + 1;
  ensures \result != EDU_OK ==> st->creds_count == \old(st->creds_count);
  ensures st->creds_count <= MAX_CREDENTIALS;
*/
int credential_issue(educhain_state_t *st, role_t caller_role, int caller_id,
                     int cred_id, int owner_id);

/* PENDING -> ACTIVE. Permitido a ROLE_COORD. */
/*@ requires \valid(st);
    requires 0 <= st->creds_count <= MAX_CREDENTIALS;
    requires 0 <= st->log_count   <= MAX_EVENTS;
    assigns { st->creds[k].state | integer k; 0 <= k < st->creds_count && st->creds[k].id == cred_id }, st->log[st->log_count], st->log_count;
    ensures \result == EDU_OK || \result == ERR_PERM || \result == ERR_BAD_ID
         || \result == ERR_NOT_FOUND || \result == ERR_BAD_TRANSITION || \result == ERR_FULL;
    ensures (caller_role != ROLE_COORD) ==> \result == ERR_PERM;   // P-1
    ensures st->creds_count == \old(st->creds_count);
*/
int credential_activate(educhain_state_t *st, role_t caller_role, int caller_id, int cred_id);

/* ACTIVE -> BLOCKED. Permitido a ROLE_ADMIN/ROLE_COORD. */
/*@ requires \valid(st);
    requires 0 <= st->creds_count <= MAX_CREDENTIALS;
    requires 0 <= st->log_count   <= MAX_EVENTS;
    assigns { st->creds[k].state | integer k; 0 <= k < st->creds_count && st->creds[k].id == cred_id }, st->log[st->log_count], st->log_count;
    ensures \result == EDU_OK || \result == ERR_PERM || \result == ERR_BAD_ID
         || \result == ERR_NOT_FOUND || \result == ERR_BAD_TRANSITION || \result == ERR_FULL;
    ensures (caller_role != ROLE_ADMIN && caller_role != ROLE_COORD) ==> \result == ERR_PERM;
    ensures st->creds_count == \old(st->creds_count);
*/
int credential_block(educhain_state_t *st, role_t caller_role, int caller_id, int cred_id);

/* BLOCKED -> ACTIVE. Permitido a ROLE_ADMIN/ROLE_COORD. */
/*@ requires \valid(st);
    requires 0 <= st->creds_count <= MAX_CREDENTIALS;
    requires 0 <= st->log_count   <= MAX_EVENTS;
    assigns { st->creds[k].state | integer k; 0 <= k < st->creds_count && st->creds[k].id == cred_id }, st->log[st->log_count], st->log_count;
    ensures \result == EDU_OK || \result == ERR_PERM || \result == ERR_BAD_ID
         || \result == ERR_NOT_FOUND || \result == ERR_BAD_TRANSITION || \result == ERR_FULL;
    ensures (caller_role != ROLE_ADMIN && caller_role != ROLE_COORD) ==> \result == ERR_PERM;
    ensures st->creds_count == \old(st->creds_count);
*/
int credential_reactivate(educhain_state_t *st, role_t caller_role, int caller_id, int cred_id);

/* {PENDING,ACTIVE,BLOCKED} -> REVOKED (terminal). Permitido a ROLE_ADMIN/ROLE_COORD. */
/*@ requires \valid(st);
    requires 0 <= st->creds_count <= MAX_CREDENTIALS;
    requires 0 <= st->log_count   <= MAX_EVENTS;
    assigns { st->creds[k].state | integer k; 0 <= k < st->creds_count && st->creds[k].id == cred_id }, st->log[st->log_count], st->log_count;
    ensures \result == EDU_OK || \result == ERR_PERM || \result == ERR_BAD_ID
         || \result == ERR_NOT_FOUND || \result == ERR_BAD_TRANSITION || \result == ERR_FULL;
    ensures (caller_role != ROLE_ADMIN && caller_role != ROLE_COORD) ==> \result == ERR_PERM;
    ensures st->creds_count == \old(st->creds_count);
    // P-8 (terminalidad): si la credencial ya estaba REVOKED, se rechaza.
    ensures (\exists integer k; 0 <= k < \old(st->creds_count)
             && st->creds[k].id == cred_id && \old(st->creds[k].state) == CRED_REVOKED)
            ==> \result == ERR_BAD_TRANSITION;
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
