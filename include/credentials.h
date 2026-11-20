/* EduChain - credentials.h
 * Modulo 3: credenciales digitales como maquina de estados.
 * Estados: PENDING -> ACTIVE -> BLOCKED <-> ACTIVE -> REVOKED (terminal).
 */
#ifndef EDUCHAIN_CREDENTIALS_H
#define EDUCHAIN_CREDENTIALS_H

#include "common.h"
#include "requests.h"   /* requests_state_t + (bajo __FRAMAC__) predicado req_eligible */
#ifdef __FRAMAC__
/* Fase 4 (correccion BLOQUEANTE-2): predicado logico role_can (matriz de permisos).
 * Solo bajo Frama-C: vive en verification/ (fuera del -Iinclude del build gcc). */
#include "specifications.h"
#endif

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
    //  (negativa) si el llamador esta AUTORIZADO (rol con permiso OP_REVOKE_CRED y
    //             los ids en rango valido) y cred_id ya estaba REVOKED antes de la
    //             llamada => se rechaza con ERR_BAD_TRANSITION (nunca re-revoca un
    //             terminal). El antecedente se condiciona a la autorizacion y al
    //             rango de ids porque esos chequeos (ERR_PERM / ERR_BAD_ID) PRECEDEN
    //             al chequeo de estado REVOKED en el cuerpo: sin esa guarda la
    //             postcondicion seria insatisfacible en el camino ERR_PERM (rol no
    //             autorizado) o ERR_BAD_ID (contraejemplo real, no framing).
    ensures (role_can(caller_role, OP_REVOKE_CRED)
             && ID_MIN <= caller_id <= ID_MAX && ID_MIN <= cred_id <= ID_MAX
             && (\exists integer k; 0 <= k < \old(st->creds_count)
                 && st->creds[k].id == cred_id && \old(st->creds[k].state) == CRED_REVOKED))
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

/* =====================================================================
 * Integracion con el flujo de solicitudes (Rev 5 §8-ter) — ADITIVA, SOLO LECTURA.
 *
 * NOTA DE ALCANCE. La Rev 5 §8-ter especifica la "Opcion 1": AMPLIAR la firma de
 * credential_issue con `const requests_state_t *rs`. Por el alcance acotado a la
 * Opcion 2 (no inflar el proyecto ni romper call-sites), aqui se implementa la
 * "Opcion 2" ADITIVA: credential_issue queda INTACTA (y sus call-sites en main.c,
 * ui/educhain_cli.c, tests/) y se expone una NUEVA via de emision
 * credential_issue_eligible que consulta credential_eligible y delega en
 * credential_issue. La regla de elegibilidad es identica a la Rev 5.
 * ===================================================================== */

/* credential_eligible: 1 si owner_id es elegible para recibir/renovar credencial,
 * 0 si no. Elegible = tiene >=1 registro VIGENTE (state == REQ_RECORDED) cuya
 * categoria sea CAT_INSTITUTION_WORKED o CAT_LINK_PERIOD (criterio DEFINITIVO
 * Rev 5 §8-ter). Consulta de SOLO LECTURA (assigns \nothing). Excluye por
 * construccion REQ_SUPERSEDED (no es REQ_RECORDED) y PENDING/ENDORSED/VERIFIED/
 * REJECTED; los registros heredados de experience.c viven en educhain_state_t,
 * NO en rs, de modo que quedan fuera por tipo. Patron gemelo de
 * experience_validated_count (src/experience.c). */
/*@ requires \valid_read(rs);
    requires 0 <= rs->reqs_count <= MAX_REQUESTS;
    assigns \nothing;
    ensures \result == 0 || \result == 1;
    // P-20 (se espera WP): elegible <=> req_eligible(rs, owner_id) (predicado de
    // requests.h, gemelo logico). El existencial positivo global puede caer en la
    // limitacion (G) (sin invariante global) y quedar parcial; respaldo dinamico en
    // tests/test_credentials.c. Nada demostrado sin WP.
    ensures \result == 1 <==> req_eligible(rs, owner_id);
*/
int credential_eligible(const requests_state_t *rs, int owner_id);

/* credential_issue_eligible: via de emision ADITIVA. Emite la credencial SOLO si
 * owner_id es elegible segun la trayectoria VIGENTE (credential_eligible sobre rs);
 * si no es elegible, devuelve ERR_PERM SIN mutar nada. Si es elegible, delega en
 * credential_issue (que conserva TODA su validacion: rol COORD, ids, profesional
 * activo, duplicados, capacidad, auditoria). rs es de SOLO LECTURA: no amplia el
 * frame de escritura de credential_issue (que sigue escribiendo solo educhain_state_t).
 * Mantiene la consistencia registros<->credenciales (solo REQ_RECORDED habilita). */
/*@ requires \valid(st) && \valid_read(rs);
    requires \separated(st, rs);
    requires 0 <= st->creds_count <= MAX_CREDENTIALS;
    requires 0 <= st->log_count   <= MAX_EVENTS;
    requires 0 <= st->pros_count  <= MAX_PROFESSIONALS;
    requires 0 <= rs->reqs_count  <= MAX_REQUESTS;
    ensures \result == EDU_OK || \result == ERR_PERM || \result == ERR_BAD_ID
         || \result == ERR_NOT_FOUND || \result == ERR_DUPLICATE || \result == ERR_FULL;
    // Elegibilidad obligatoria (ambas mitades), via el predicado req_eligible:
    //  - sin trayectoria vigente que habilite, NUNCA se emite (no muta, ERR_PERM).
    ensures !req_eligible(rs, owner_id) ==> \result == ERR_PERM;
    //  - si se emite, el owner era elegible.
    ensures \result == EDU_OK ==> req_eligible(rs, owner_id);
    // Atomicidad heredada de credential_issue: el contador crece 1 en exito,
    // intacto en error. (Se deriva del contrato de credential_issue.)
    ensures \result == EDU_OK ==> st->creds_count == \old(st->creds_count) + 1;
    ensures \result != EDU_OK ==> st->creds_count == \old(st->creds_count);
    ensures st->creds_count <= MAX_CREDENTIALS;
    // assigns por behavior (patron credential_issue): solo el camino elegible + con
    // hueco escribe; el no-elegible (y el sin-hueco) no tocan nada. rs NUNCA se escribe.
    behavior not_eligible_or_no_space:
      assumes !req_eligible(rs, owner_id)
           || st->creds_count >= MAX_CREDENTIALS || st->log_count >= MAX_EVENTS;
      assigns \nothing;
    behavior eligible_with_space:
      assumes req_eligible(rs, owner_id)
           && st->creds_count < MAX_CREDENTIALS && st->log_count < MAX_EVENTS;
      assigns st->creds[st->creds_count], st->creds_count,
              st->log[st->log_count], st->log_count;
    complete behaviors;
    disjoint behaviors not_eligible_or_no_space, eligible_with_space;
*/
int credential_issue_eligible(educhain_state_t *st, const requests_state_t *rs,
                              role_t caller_role, int caller_id, int cred_id, int owner_id);

#endif /* EDUCHAIN_CREDENTIALS_H */
