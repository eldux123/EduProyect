/* EduChain - requests.h
 * Modulo del flujo de solicitudes (presentador -> verificador -> administrador).
 * Fuente de verdad: docs/diseno_flujo_solicitudes.md (Rev 5).
 *
 * ETAPA 2: estructura inicial + ciclo de vida completo de solicitudes.
 *   - request_init, request_find, request_status, request_requires_endorsement
 *   - request_submit, request_submit_correction
 *   - request_endorse, request_verify, request_reject
 *   - request_record (incorporacion al historial oficial, ADMIN; Variante A simple
 *     REQ_VERIFIED->REQ_RECORDED y Variante B correccion con sustitucion atomica
 *     del registro anterior REQ_RECORDED->REQ_SUPERSEDED, Rev 5 §5.4/§5.5/§6)
 * NO incluye (Opcion 3, excluido): consultas publicas *_for, authz_grant/revoke,
 * requests_is_representative, P-SCOPE. La elegibilidad de credenciales
 * (credential_eligible) y su integracion con credentials.c se tratan aparte.
 *
 * Autorizacion en este entregable: el modulo lleva su propia comprobacion de rol
 * interna (request_role_can, un switch puro en src/requests.c). NO se llama a
 * security_can ni se edita security.c/security.h (integracion posterior).
 *
 * evidence_ref (request_t, common.h) es una referencia EXTERNA OPACA: NO es un
 * hash criptografico ni garantiza integridad documental (Rev 5 §4.4).
 */
#ifndef EDUCHAIN_REQUESTS_H
#define EDUCHAIN_REQUESTS_H

#include "common.h"
#ifdef __FRAMAC__
/* Reutiliza tipos/predicados ACSL compartidos (patron credentials.h).
 * NO se modifica specifications.h: el predicado del flujo nuevo (req_role_can)
 * se declara localmente mas abajo, bajo __FRAMAC__, sin tocar el fichero comun. */
#include "specifications.h"

/*@
  // req_role_can: matriz rol x operacion del flujo nuevo (Rev 5 §3.3), SOLO para
  // las OPs de este entregable. Es el gemelo de role_can (specifications.h) pero
  // para las operaciones de requests; refleja request_role_can (src/requests.c).
  // SUBMIT: INSTITUTION | PROFESSOR(=ROLE_PRO); ENDORSE: INSTITUTION;
  // VERIFY/REJECT: VERIFIER; RECORD: ADMIN (incorporacion al historial oficial,
  // Rev 5 §5.4/§5.5/§8-bis). HR/COORD negados en estas OPs (separacion estricta
  // del flujo nuevo). No presupone nada: es la definicion de la matriz.
  predicate req_role_can(role_t r, integer op) =
       (op == OP_SUBMIT_REQUEST  ==> (r == ROLE_INSTITUTION || r == ROLE_PRO))
    && (op == OP_ENDORSE_REQUEST ==> (r == ROLE_INSTITUTION))
    && (op == OP_VERIFY_REQUEST  ==> (r == ROLE_VERIFIER))
    && (op == OP_REJECT_REQUEST  ==> (r == ROLE_VERIFIER))
    && (op == OP_RECORD_REQUEST  ==> (r == ROLE_ADMIN))
    && (!(op == OP_SUBMIT_REQUEST || op == OP_ENDORSE_REQUEST
          || op == OP_VERIFY_REQUEST || op == OP_REJECT_REQUEST
          || op == OP_RECORD_REQUEST) ==> \false);

  // req_eligible: criterio DEFINITIVO de elegibilidad de credenciales (Rev 5
  // §8-ter / P-20). owner_id es elegible si existe >=1 registro VIGENTE
  // (REQ_RECORDED) suyo en CAT_INSTITUTION_WORKED o CAT_LINK_PERIOD. REQ_SUPERSEDED
  // queda excluido por construccion (no es REQ_RECORDED). Es el gemelo logico de la
  // funcion C credential_eligible (src/credentials.c), igual que role_can refleja
  // security_can; permite referenciar la elegibilidad en los contratos ACSL.
  predicate req_eligible{L}(requests_state_t *rs, integer owner_id) =
    \exists integer k; 0 <= k < rs->reqs_count
      && rs->reqs[k].owner_id == owner_id
      && rs->reqs[k].state == REQ_RECORDED
      && (rs->reqs[k].category == CAT_INSTITUTION_WORKED
          || rs->reqs[k].category == CAT_LINK_PERIOD);
*/
#endif

/* --------------------------------------------------------------------------
 * Inicializacion del estado de solicitudes.
 * -------------------------------------------------------------------------- */
/*@ requires \valid(rs);
    assigns rs->reqs_count, rs->auth_count;
    ensures rs->reqs_count == 0 && rs->auth_count == 0;
*/
void request_init(requests_state_t *rs);

/* --------------------------------------------------------------------------
 * Consultas primitivas de solo lectura (sin chequeo de rol, assigns \nothing).
 * Gemelas de experience_find / credential_status.
 * -------------------------------------------------------------------------- */

/* Indice 0..reqs_count-1 de la solicitud req_id, o -1 si no existe. */
/*@ requires \valid_read(rs);
    requires 0 <= rs->reqs_count <= MAX_REQUESTS;
    assigns \nothing;
    ensures -1 <= \result < rs->reqs_count;
    ensures \result >= 0 ==> rs->reqs[\result].id == req_id;
    ensures \result == -1 ==>
            \forall integer k; 0 <= k < rs->reqs_count ==> rs->reqs[k].id != req_id;
*/
int request_find(const requests_state_t *rs, int req_id);

/* Estado (request_state_t) de la solicitud req_id, o -1 si no existe. */
/*@ requires \valid_read(rs);
    requires 0 <= rs->reqs_count <= MAX_REQUESTS;
    assigns \nothing;
    ensures \result == -1 || \result == REQ_PENDING || \result == REQ_ENDORSED
         || \result == REQ_VERIFIED || \result == REQ_REJECTED
         || \result == REQ_RECORDED || \result == REQ_SUPERSEDED;
    ensures \result >= 0 ==>
            (\exists integer k; 0 <= k < rs->reqs_count
             && rs->reqs[k].id == req_id && rs->reqs[k].state == \result);
*/
int request_status(const requests_state_t *rs, int req_id);

/* --------------------------------------------------------------------------
 * Respaldo institucional por categoria (Rev 5 §5.3). Switch puro, sin estado.
 * Devuelve 1 si la categoria exige respaldo institucional previo; 0 si no.
 * CAT_RESEARCH_PROJECT devuelve 0 aqui (base); su condicionalidad por
 * institution_id (§5.3 bis) se compone en el cuerpo de submit, NO aqui.
 * -------------------------------------------------------------------------- */
/*@ assigns \nothing;
    ensures \result == 0 || \result == 1;
*/
int request_requires_endorsement(int category);

/* --------------------------------------------------------------------------
 * request_submit: presenta una solicitud ORIGINAL (prev_id = 0 interno).
 * Nacimiento (Rev 5 §5.0):
 *   - INSTITUTION representa + exige respaldo -> REQ_ENDORSED, endorser_id=institution_id
 *   - resto (PROFESSOR; o sin respaldo)       -> REQ_PENDING, endorser_id=0
 * PROFESSOR solo presenta lo propio (caller_id == owner_id).
 * INSTITUTION solo si hay vinculo activo en auth[] (caller_id -> institution_id).
 * -------------------------------------------------------------------------- */
/*@ requires \valid(st) && \valid(rs);
    requires \separated(st, rs);   // estados separados (Rev 5 §4.5)
    requires 0 <= rs->reqs_count <= MAX_REQUESTS;
    requires 0 <= rs->auth_count <= MAX_AUTHZ;
    requires 0 <= st->log_count  <= MAX_EVENTS;
    // ERR_NULL NO figura: \valid(st)&&\valid(rs) lo hacen inalcanzable (patron
    // credential_issue). El cuerpo conserva el guard defensivo igualmente.
    ensures \result == EDU_OK || \result == ERR_PERM || \result == ERR_BAD_ID
         || \result == ERR_BAD_DATE || \result == ERR_DUPLICATE || \result == ERR_FULL;
    // P-1 (mitad negativa): rol sin permiso de SUBMIT => ERR_PERM o ERR_BAD_ID
    // (ERR_BAD_ID porque el rango de ids se chequea ANTES del permiso, patron
    // cred_transition). Se espera demostrar en WP.
    ensures !req_role_can(caller_role, OP_SUBMIT_REQUEST)
            ==> (\result == ERR_PERM || \result == ERR_BAD_ID);
    // Atomicidad (BR-10): el conteo crece 1 en exito, intacto en error.
    ensures \result == EDU_OK ==> rs->reqs_count == \old(rs->reqs_count) + 1;
    ensures \result != EDU_OK ==> rs->reqs_count == \old(rs->reqs_count);
    ensures rs->reqs_count <= MAX_REQUESTS;
    // assigns partido por behavior (patron credential_issue space/no_space):
    // solo el camino con hueco escribe el slot reqs[reqs_count] y el log.
    behavior space:
      assumes rs->reqs_count < MAX_REQUESTS && st->log_count < MAX_EVENTS;
      assigns rs->reqs[rs->reqs_count], rs->reqs_count,
              st->log[st->log_count], st->log_count;
    behavior no_space:
      assumes rs->reqs_count >= MAX_REQUESTS || st->log_count >= MAX_EVENTS;
      assigns \nothing;
    complete behaviors;
    disjoint behaviors space, no_space;
*/
int request_submit(educhain_state_t *st, requests_state_t *rs,
                   role_t caller_role, int caller_id, int req_id, int owner_id,
                   int institution_id, int category, int start_date, int end_date,
                   int evidence_ref, int num_a, int num_b, int num_c, int num_d);

/* --------------------------------------------------------------------------
 * request_submit_correction: presenta una CORRECCION (prev_id != 0) de un
 * registro vigente (REQ_RECORDED). Coherencia categoria/institucion T-1 (Rev 5
 * §5.5): misma categoria que el original; si la categoria asocia institucion,
 * mismo institution_id; si no, ambos institution_id == 0. La correccion nace
 * como una original (§5.0) con prev_id almacenado. NO toca el registro anterior.
 * -------------------------------------------------------------------------- */
/*@ requires \valid(st) && \valid(rs);
    requires \separated(st, rs);   // estados separados (Rev 5 §4.5)
    requires 0 <= rs->reqs_count <= MAX_REQUESTS;
    requires 0 <= rs->auth_count <= MAX_AUTHZ;
    requires 0 <= st->log_count  <= MAX_EVENTS;
    ensures \result == EDU_OK || \result == ERR_PERM || \result == ERR_BAD_ID
         || \result == ERR_BAD_DATE || \result == ERR_DUPLICATE || \result == ERR_FULL
         || \result == ERR_NOT_FOUND || \result == ERR_BAD_TRANSITION;
    ensures !req_role_can(caller_role, OP_SUBMIT_REQUEST)
            ==> (\result == ERR_PERM || \result == ERR_BAD_ID);
    ensures \result == EDU_OK ==> rs->reqs_count == \old(rs->reqs_count) + 1;
    ensures \result != EDU_OK ==> rs->reqs_count == \old(rs->reqs_count);
    ensures rs->reqs_count <= MAX_REQUESTS;
    behavior space:
      assumes rs->reqs_count < MAX_REQUESTS && st->log_count < MAX_EVENTS;
      assigns rs->reqs[rs->reqs_count], rs->reqs_count,
              st->log[st->log_count], st->log_count;
    behavior no_space:
      assumes rs->reqs_count >= MAX_REQUESTS || st->log_count >= MAX_EVENTS;
      assigns \nothing;
    complete behaviors;
    disjoint behaviors space, no_space;
*/
int request_submit_correction(educhain_state_t *st, requests_state_t *rs,
                              role_t caller_role, int caller_id, int req_id, int owner_id,
                              int institution_id, int category, int start_date, int end_date,
                              int evidence_ref, int prev_id,
                              int num_a, int num_b, int num_c, int num_d);

/* --------------------------------------------------------------------------
 * request_endorse: PENDING -> ENDORSED. Solo INSTITUTION que representa a la
 * institution_id de la solicitud, y solo para categorias que exigen respaldo.
 * -------------------------------------------------------------------------- */
/*@ requires \valid(st) && \valid(rs);
    requires \separated(st, rs);   // estados separados (Rev 5 §4.5)
    requires 0 <= rs->reqs_count <= MAX_REQUESTS;
    requires 0 <= rs->auth_count <= MAX_AUTHZ;
    requires 0 <= st->log_count  <= MAX_EVENTS;
    ensures \result == EDU_OK || \result == ERR_PERM || \result == ERR_BAD_ID
         || \result == ERR_NOT_FOUND || \result == ERR_BAD_TRANSITION || \result == ERR_FULL;
    ensures !req_role_can(caller_role, OP_ENDORSE_REQUEST)
            ==> (\result == ERR_PERM || \result == ERR_BAD_ID);
    ensures rs->reqs_count == \old(rs->reqs_count);
    // P-7 (transicion legal PENDING->ENDORSED): en exito, req_id estaba PENDING
    // y queda ENDORSED (misma k). Se espera WP.
    ensures \result == EDU_OK ==>
            (\exists integer k; 0 <= k < rs->reqs_count && rs->reqs[k].id == req_id
             && \old(rs->reqs[k].state) == REQ_PENDING && rs->reqs[k].state == REQ_ENDORSED);
    behavior success:
      assumes st->log_count < MAX_EVENTS;
      assigns { rs->reqs[k].state | integer k;
                0 <= k < rs->reqs_count && rs->reqs[k].id == req_id },
              { rs->reqs[k].endorser_id | integer k;
                0 <= k < rs->reqs_count && rs->reqs[k].id == req_id },
              st->log[st->log_count], st->log_count;
    behavior log_full:
      assumes st->log_count >= MAX_EVENTS;
      assigns { rs->reqs[k].state | integer k;
                0 <= k < rs->reqs_count && rs->reqs[k].id == req_id },
              { rs->reqs[k].endorser_id | integer k;
                0 <= k < rs->reqs_count && rs->reqs[k].id == req_id };
    complete behaviors;
    disjoint behaviors success, log_full;
*/
int request_endorse(educhain_state_t *st, requests_state_t *rs,
                    role_t caller_role, int caller_id, int req_id);

/* --------------------------------------------------------------------------
 * request_verify: {PENDING si no exige respaldo | ENDORSED si exige} -> VERIFIED.
 * Solo VERIFIER. No autovalidacion: submitter_id == caller_id -> ERR_SELF_VALIDATE.
 * -------------------------------------------------------------------------- */
/*@ requires \valid(st) && \valid(rs);
    requires \separated(st, rs);   // estados separados (Rev 5 §4.5)
    requires 0 <= rs->reqs_count <= MAX_REQUESTS;
    requires 0 <= st->log_count  <= MAX_EVENTS;
    ensures \result == EDU_OK || \result == ERR_PERM || \result == ERR_BAD_ID
         || \result == ERR_NOT_FOUND || \result == ERR_SELF_VALIDATE
         || \result == ERR_BAD_TRANSITION || \result == ERR_FULL;
    ensures !req_role_can(caller_role, OP_VERIFY_REQUEST)
            ==> (\result == ERR_PERM || \result == ERR_BAD_ID);
    ensures rs->reqs_count == \old(rs->reqs_count);
    // P-3 (no autovalidacion, mitad negativa, patron experience_validate): si el
    // submitter de la solicitud es el propio caller => ERR_SELF_VALIDATE (nunca OK).
    ensures (ID_MIN <= caller_id <= ID_MAX && ID_MIN <= req_id <= ID_MAX
             && req_role_can(caller_role, OP_VERIFY_REQUEST)
             && (\exists integer k; 0 <= k < rs->reqs_count
                 && rs->reqs[k].id == req_id && rs->reqs[k].submitter_id == caller_id))
            ==> \result == ERR_SELF_VALIDATE;
    // P-7 (en exito queda VERIFIED).
    ensures \result == EDU_OK ==>
            (\exists integer k; 0 <= k < rs->reqs_count && rs->reqs[k].id == req_id
             && rs->reqs[k].state == REQ_VERIFIED);
    behavior success:
      assumes st->log_count < MAX_EVENTS;
      assigns { rs->reqs[k].state | integer k;
                0 <= k < rs->reqs_count && rs->reqs[k].id == req_id },
              { rs->reqs[k].verifier_id | integer k;
                0 <= k < rs->reqs_count && rs->reqs[k].id == req_id },
              st->log[st->log_count], st->log_count;
    behavior log_full:
      assumes st->log_count >= MAX_EVENTS;
      assigns { rs->reqs[k].state | integer k;
                0 <= k < rs->reqs_count && rs->reqs[k].id == req_id },
              { rs->reqs[k].verifier_id | integer k;
                0 <= k < rs->reqs_count && rs->reqs[k].id == req_id };
    complete behaviors;
    disjoint behaviors success, log_full;
*/
int request_verify(educhain_state_t *st, requests_state_t *rs,
                   role_t caller_role, int caller_id, int req_id);

/* --------------------------------------------------------------------------
 * request_reject: {PENDING | ENDORSED} -> REJECTED (terminal). Solo VERIFIER.
 * No autovalidacion (igual que verify).
 * -------------------------------------------------------------------------- */
/*@ requires \valid(st) && \valid(rs);
    requires \separated(st, rs);   // estados separados (Rev 5 §4.5)
    requires 0 <= rs->reqs_count <= MAX_REQUESTS;
    requires 0 <= st->log_count  <= MAX_EVENTS;
    ensures \result == EDU_OK || \result == ERR_PERM || \result == ERR_BAD_ID
         || \result == ERR_NOT_FOUND || \result == ERR_SELF_VALIDATE
         || \result == ERR_BAD_TRANSITION || \result == ERR_FULL;
    ensures !req_role_can(caller_role, OP_REJECT_REQUEST)
            ==> (\result == ERR_PERM || \result == ERR_BAD_ID);
    ensures rs->reqs_count == \old(rs->reqs_count);
    ensures (ID_MIN <= caller_id <= ID_MAX && ID_MIN <= req_id <= ID_MAX
             && req_role_can(caller_role, OP_REJECT_REQUEST)
             && (\exists integer k; 0 <= k < rs->reqs_count
                 && rs->reqs[k].id == req_id && rs->reqs[k].submitter_id == caller_id))
            ==> \result == ERR_SELF_VALIDATE;
    ensures \result == EDU_OK ==>
            (\exists integer k; 0 <= k < rs->reqs_count && rs->reqs[k].id == req_id
             && rs->reqs[k].state == REQ_REJECTED);
    behavior success:
      assumes st->log_count < MAX_EVENTS;
      assigns { rs->reqs[k].state | integer k;
                0 <= k < rs->reqs_count && rs->reqs[k].id == req_id },
              st->log[st->log_count], st->log_count;
    behavior log_full:
      assumes st->log_count >= MAX_EVENTS;
      assigns { rs->reqs[k].state | integer k;
                0 <= k < rs->reqs_count && rs->reqs[k].id == req_id };
    complete behaviors;
    disjoint behaviors success, log_full;
*/
int request_reject(educhain_state_t *st, requests_state_t *rs,
                   role_t caller_role, int caller_id, int req_id);

/* --------------------------------------------------------------------------
 * request_record: incorporacion al historial oficial. SOLO ADMIN (Rev 5 §5.4).
 * Detecta la variante por el prev_id de la solicitud req_id que incorpora:
 *   - Variante A (simple, prev_id == 0): REQ_VERIFIED -> REQ_RECORDED. El contenido
 *     aprobado NO se altera (solo cambia state).
 *   - Variante B (correccion, prev_id != 0): en la MISMA operacion logica,
 *     req_id: REQ_VERIFIED -> REQ_RECORDED  Y  prev_id: REQ_RECORDED -> REQ_SUPERSEDED.
 *     Validacion-R ANTES de mutar (patron "validar todo antes de mutar nada", §5.5):
 *     prev existe; prev vigente REQ_RECORDED (rechaza re-sustituir un SUPERSEDED y la
 *     2a de dos correcciones en carrera); mismo owner; coherencia T-1 (misma categoria;
 *     institucion fijada por la categoria: igual si asocia, ambas 0 si no); req_id
 *     verificada; hueco de log. Aciclicidad T-2 por transiciones de estado (SIN
 *     recorrer la cadena prev_id): un REQ_SUPERSEDED (terminal) no vuelve a ser
 *     prev_id, luego las cadenas incorporadas son lineales y finitas.
 * ATOMICIDAD (BR-10): en cualquier error NINGUNO de los dos slots se modifica.
 * El cambio del anterior se LIMITA a su campo state (ensures de inmutabilidad de
 * owner/categoria/institution/metadatos/evidence_ref/fechas).
 * -------------------------------------------------------------------------- */
/*@ requires \valid(st) && \valid(rs);
    requires \separated(st, rs);   // estados separados (Rev 5 §4.5)
    requires 0 <= rs->reqs_count <= MAX_REQUESTS;
    requires 0 <= st->log_count  <= MAX_EVENTS;
    ensures \result == EDU_OK || \result == ERR_PERM || \result == ERR_BAD_ID
         || \result == ERR_NOT_FOUND || \result == ERR_BAD_TRANSITION || \result == ERR_FULL;
    // P-1 (mitad negativa): rol sin permiso de RECORD (todo rol != ADMIN) => se
    // rechaza con ERR_PERM, o ERR_BAD_ID si ademas los ids estan fuera de rango
    // (el rango se chequea antes del permiso, patron cred_transition). Se espera WP.
    ensures !req_role_can(caller_role, OP_RECORD_REQUEST)
            ==> (\result == ERR_PERM || \result == ERR_BAD_ID);
    // request_record NUNCA crea ni borra solicitudes: el contador es invariante.
    ensures rs->reqs_count == \old(rs->reqs_count);
    // P-7 Variante A/B (positiva): en exito, req_id estaba REQ_VERIFIED y queda
    // REQ_RECORDED (misma k). Se espera WP (misma clase que cred_transition P-7).
    ensures \result == EDU_OK ==>
            (\exists integer k; 0 <= k < rs->reqs_count && rs->reqs[k].id == req_id
             && \old(rs->reqs[k].state) == REQ_VERIFIED && rs->reqs[k].state == REQ_RECORDED);
    // --- Mejora ACSL #2: ensures EXPLICITOS de la doble transicion e inmutabilidad ---
    // (b) En exito, el NUEVO registro (req_id) pasa a REQ_RECORDED (igual que P-7 arriba,
    //     reiterado aqui por claridad de la doble transicion). Se espera WP (framing).
    // (a) En exito de la Variante B (correccion, el req_id incorporado tenia prev_id != 0),
    //     el registro ANTERIOR (el de id == \old(prev_id de req_id)) pasa de REQ_RECORDED
    //     a REQ_SUPERSEDED (anclado en \old). En la Variante A (prev_id == 0) no hay anterior,
    //     por eso el antecedente exige \old(prev_id) != 0. Se espera WP (misma clase que la
    //     doble transicion de cred_transition cruzando audit_record_event: PENDIENTE honesto).
    ensures \result == EDU_OK ==>
            (\forall integer j; (0 <= j < rs->reqs_count && rs->reqs[j].id == req_id) ==>
                (\old(rs->reqs[j].prev_id) != 0 ==>
                    (\exists integer p; 0 <= p < rs->reqs_count
                     && rs->reqs[p].id == \old(rs->reqs[j].prev_id)
                     && \old(rs->reqs[p].state) == REQ_RECORDED
                     && rs->reqs[p].state == REQ_SUPERSEDED)));
    // (c) Inmutabilidad-salvo-state del registro ANTERIOR: en exito de la Variante B,
    //     el anterior conserva TODOS sus campos EXCEPTO state (anclados en \old):
    //     owner_id, category, institution_id, metadatos num_a..d, evidence_ref, prev_id,
    //     submitter_id, verifier_id, endorser_id, start_date, end_date.
    ensures \result == EDU_OK ==>
            (\forall integer j; (0 <= j < rs->reqs_count && rs->reqs[j].id == req_id
             && \old(rs->reqs[j].prev_id) != 0) ==>
                (\forall integer p; (0 <= p < rs->reqs_count
                 && rs->reqs[p].id == \old(rs->reqs[j].prev_id)) ==>
                    (rs->reqs[p].owner_id       == \old(rs->reqs[p].owner_id)
                  && rs->reqs[p].institution_id == \old(rs->reqs[p].institution_id)
                  && rs->reqs[p].submitter_id   == \old(rs->reqs[p].submitter_id)
                  && rs->reqs[p].verifier_id    == \old(rs->reqs[p].verifier_id)
                  && rs->reqs[p].endorser_id    == \old(rs->reqs[p].endorser_id)
                  && rs->reqs[p].category       == \old(rs->reqs[p].category)
                  && rs->reqs[p].start_date     == \old(rs->reqs[p].start_date)
                  && rs->reqs[p].end_date       == \old(rs->reqs[p].end_date)
                  && rs->reqs[p].evidence_ref   == \old(rs->reqs[p].evidence_ref)
                  && rs->reqs[p].prev_id        == \old(rs->reqs[p].prev_id)
                  && rs->reqs[p].num_a          == \old(rs->reqs[p].num_a)
                  && rs->reqs[p].num_b          == \old(rs->reqs[p].num_b)
                  && rs->reqs[p].num_c          == \old(rs->reqs[p].num_c)
                  && rs->reqs[p].num_d          == \old(rs->reqs[p].num_d))));
    behavior success:
      assumes st->log_count < MAX_EVENTS;
      // --- Mejora ACSL #1: assigns acotado a los DOS slots concretos de la operacion ---
      // Solo cambia el campo state de: (i) el slot de req_id (-> REQ_RECORDED) y (ii) en la
      // Variante B, el slot del registro anterior, cuyo id == prev_id del propio req_id
      // (-> REQ_SUPERSEDED). El set-comprehension restringe los indices escribibles a esos
      // dos slots por su id (req_id, o prev_id de algun slot cuyo id sea req_id), en lugar
      // de .state de TODOS los slots. Sigue siendo campo-state-only, asi la inmutabilidad
      // de owner/institution_id/category/fechas/evidence_ref/prev_id/num_a..d se deriva del
      // frame; ademas acota ahora QUE slots pueden cambiar (los dos involucrados, no todos).
      assigns { rs->reqs[k].state | integer k;
                0 <= k < rs->reqs_count
                && (rs->reqs[k].id == req_id
                    || (\exists integer j; 0 <= j < rs->reqs_count
                        && rs->reqs[j].id == req_id
                        && rs->reqs[k].id == rs->reqs[j].prev_id)) },
              st->log[st->log_count], st->log_count;
    behavior log_full:
      assumes st->log_count >= MAX_EVENTS;
      assigns \nothing;
    complete behaviors;
    disjoint behaviors success, log_full;
*/
int request_record(educhain_state_t *st, requests_state_t *rs,
                   role_t caller_role, int caller_id, int req_id);

#endif /* EDUCHAIN_REQUESTS_H */
