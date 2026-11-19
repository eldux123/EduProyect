/* EduChain - credentials.c  (Fase 3)
 * Maquina de estados con transiciones explicitas (Fase 1 §5.1).
 */
#include "credentials.h"
#include "professionals.h"
#include "security.h"
#include "audit.h"
#ifdef __FRAMAC__
/* Fase 4 (correccion ACSL #1): predicado logico role_can (matriz de permisos).
 * Solo bajo Frama-C: vive en verification/ (fuera del -Iinclude del build gcc). */
#include "specifications.h"
#endif

int credential_status(const educhain_state_t *st, int cred_id)
{
    int i;
    if (st == 0) {
        return -1;
    }
    /*@
      loop invariant 0 <= i <= st->creds_count;
      loop invariant \forall integer k; 0 <= k < i ==> st->creds[k].id != cred_id;
      loop assigns i;
      loop variant st->creds_count - i;
    */
    for (i = 0; i < st->creds_count; i++) {
        if (st->creds[i].id == cred_id) {
            return (int)st->creds[i].state;
        }
    }
    return -1;
}

int credential_is_valid(const educhain_state_t *st, int cred_id)
{
    return (credential_status(st, cred_id) == (int)CRED_ACTIVE) ? 1 : 0;
}

/*@ requires \valid_read(st);
    requires 0 <= st->creds_count <= MAX_CREDENTIALS;
    assigns \nothing;
    ensures -1 <= \result < st->creds_count;
    ensures \result >= 0 ==> st->creds[\result].id == cred_id;
    ensures \result == -1 ==>
            \forall integer k; 0 <= k < st->creds_count ==> st->creds[k].id != cred_id;
*/
static int cred_find(const educhain_state_t *st, int cred_id)
{
    int i;
    /*@
      loop invariant 0 <= i <= st->creds_count;
      loop invariant \forall integer k; 0 <= k < i ==> st->creds[k].id != cred_id;
      loop assigns i;
      loop variant st->creds_count - i;
    */
    for (i = 0; i < st->creds_count; i++) {
        if (st->creds[i].id == cred_id) {
            return i;
        }
    }
    return -1;
}

/*@ requires \valid_read(st);
    requires 0 <= st->creds_count <= MAX_CREDENTIALS;
    assigns \nothing;
    ensures -1 <= \result < st->creds_count;
    ensures \result >= 0 ==> st->creds[\result].owner_id == owner_id;
    ensures \result == -1 ==>
            \forall integer k; 0 <= k < st->creds_count ==> st->creds[k].owner_id != owner_id;
*/
static int cred_find_by_owner(const educhain_state_t *st, int owner_id)
{
    int i;
    /*@
      loop invariant 0 <= i <= st->creds_count;
      loop invariant \forall integer k; 0 <= k < i ==> st->creds[k].owner_id != owner_id;
      loop assigns i;
      loop variant st->creds_count - i;
    */
    for (i = 0; i < st->creds_count; i++) {
        if (st->creds[i].owner_id == owner_id) {
            return i;
        }
    }
    return -1;
}

int credential_issue(educhain_state_t *st, role_t caller_role, int caller_id,
                     int cred_id, int owner_id)
{
    if (st == 0) {
        return ERR_NULL;
    }
    if (caller_id < ID_MIN || caller_id > ID_MAX ||
        cred_id < ID_MIN || cred_id > ID_MAX || owner_id < ID_MIN || owner_id > ID_MAX) {
        return ERR_BAD_ID;
    }
    if (!security_can(caller_role, OP_ISSUE_CRED)) {
        return ERR_PERM;
    }
    if (!professional_is_active(st, owner_id)) {
        return ERR_NOT_FOUND;
    }
    if (cred_find(st, cred_id) >= 0) {
        return ERR_DUPLICATE;
    }
    /* Una credencial por profesional. */
    if (cred_find_by_owner(st, owner_id) >= 0) {
        return ERR_DUPLICATE;
    }
    if (st->creds_count < 0 || st->creds_count >= MAX_CREDENTIALS) {
        return ERR_FULL;
    }
    if (st->log_count >= MAX_EVENTS) {
        return ERR_FULL;
    }

    {
        int n = st->creds_count;
        /*@ assert 0 <= n < MAX_CREDENTIALS; */
        st->creds[n].id       = cred_id;
        st->creds[n].owner_id = owner_id;
        st->creds[n].state    = CRED_PENDING;
        st->creds_count = n + 1;
    }

    /* actor_id = caller_id (el COORD que emite), NO el titular owner_id. */
    (void)audit_record_event(st, caller_id, OP_ISSUE_CRED, cred_id, EDU_OK);
    return EDU_OK;
}

/* Transicion generica validada contra la tabla (BR-7). */
/*@ requires \valid(st);
    requires 0 <= st->creds_count <= MAX_CREDENTIALS;
    requires 0 <= st->log_count   <= MAX_EVENTS;
    ensures \result == EDU_OK || \result == ERR_PERM || \result == ERR_BAD_ID
         || \result == ERR_NOT_FOUND || \result == ERR_BAD_TRANSITION || \result == ERR_FULL;
    ensures st->creds_count == \old(st->creds_count);
    // -------------------------------------------------------------------------
    // #1: contrato fuerte de la auxiliar, parametrizado por from/to/op_code.
    //     Expone a los wrappers lo que necesitan para derivar P-1 y P-7.
    // P-1 (autorizacion): rol sin permiso para op_code => el llamador NO tiene
    //     exito; obtiene ERR_PERM, o ERR_BAD_ID si ademas los ids estan fuera de
    //     rango. El consecuente admite ERR_BAD_ID porque el cuerpo chequea el
    //     rango de ids (ERR_BAD_ID) ANTES del permiso (ERR_PERM): con el
    //     consecuente == ERR_PERM a secas la postcondicion seria insatisfacible en
    //     el camino ERR_BAD_ID (p. ej. caller_id=0 con rol no autorizado), un
    //     contraejemplo real, no framing (BLOQUEANTE-5). Mismo patron que la mitad
    //     negativa de P-8 (credential_revoke) y P-2 (experience_validate).
    // role_can (verification/specifications.h) refleja la matriz de security_can.
    ensures !role_can(caller_role, op_code)
            ==> (\result == ERR_PERM || \result == ERR_BAD_ID);
    // P-7 (transicion legal from->to): en exito, la credencial cred_id estaba
    //     en `from` y queda en `to` (pre con \old, post sin \old, misma k).
    ensures \result == EDU_OK ==>
            (\exists integer k; 0 <= k < st->creds_count && st->creds[k].id == cred_id
             && \old(st->creds[k].state) == from && st->creds[k].state == to);
    // El exito exige que cred_id existiera en estado `from` al entrar.
    ensures \result == EDU_OK ==>
            (\exists integer k; 0 <= k < \old(st->creds_count)
             && st->creds[k].id == cred_id && \old(st->creds[k].state) == from);
    // -------------------------------------------------------------------------
    // #5: assigns partido por behavior de exito vs. error.
    behavior success:
      assumes st->log_count < MAX_EVENTS;
      assigns { st->creds[k].state | integer k;
                0 <= k < st->creds_count && st->creds[k].id == cred_id },
              st->log[st->log_count], st->log_count;
    behavior log_full:
      assumes st->log_count >= MAX_EVENTS;
      assigns \nothing;
    complete behaviors;
    disjoint behaviors success, log_full;
*/
static int cred_transition(educhain_state_t *st, role_t caller_role, int caller_id,
                           int op_code, int cred_id, cred_state_t from, cred_state_t to)
{
    int idx;
    if (st == 0) {
        return ERR_NULL;
    }
    if (caller_id < ID_MIN || caller_id > ID_MAX || cred_id < ID_MIN || cred_id > ID_MAX) {
        return ERR_BAD_ID;
    }
    if (!security_can(caller_role, op_code)) {
        return ERR_PERM;
    }
    idx = cred_find(st, cred_id);
    if (idx < 0) {
        return ERR_NOT_FOUND;
    }
    if (st->creds[idx].state != from) {
        return ERR_BAD_TRANSITION;
    }
    if (st->log_count >= MAX_EVENTS) {
        return ERR_FULL;
    }

    st->creds[idx].state = to;
    /* actor_id = caller_id real de quien ejecuta la transicion. */
    (void)audit_record_event(st, caller_id, op_code, cred_id, EDU_OK);
    return EDU_OK;
}

int credential_activate(educhain_state_t *st, role_t caller_role, int caller_id, int cred_id)
{
    return cred_transition(st, caller_role, caller_id, OP_ACTIVATE_CRED, cred_id,
                           CRED_PENDING, CRED_ACTIVE);
}

int credential_block(educhain_state_t *st, role_t caller_role, int caller_id, int cred_id)
{
    return cred_transition(st, caller_role, caller_id, OP_BLOCK_CRED, cred_id,
                           CRED_ACTIVE, CRED_BLOCKED);
}

int credential_reactivate(educhain_state_t *st, role_t caller_role, int caller_id, int cred_id)
{
    return cred_transition(st, caller_role, caller_id, OP_REACTIVATE_CRED, cred_id,
                           CRED_BLOCKED, CRED_ACTIVE);
}

int credential_revoke(educhain_state_t *st, role_t caller_role, int caller_id, int cred_id)
{
    /* REVOKED es terminal; se permite desde PENDING, ACTIVE o BLOCKED.
     * No desde REVOKED (quedaria rechazado como ERR_BAD_TRANSITION). */
    int idx;
    if (st == 0) {
        return ERR_NULL;
    }
    if (caller_id < ID_MIN || caller_id > ID_MAX || cred_id < ID_MIN || cred_id > ID_MAX) {
        return ERR_BAD_ID;
    }
    if (!security_can(caller_role, OP_REVOKE_CRED)) {
        return ERR_PERM;
    }
    idx = cred_find(st, cred_id);
    if (idx < 0) {
        return ERR_NOT_FOUND;
    }
    if (st->creds[idx].state == CRED_REVOKED) {
        return ERR_BAD_TRANSITION;
    }
    if (st->log_count >= MAX_EVENTS) {
        return ERR_FULL;
    }

    st->creds[idx].state = CRED_REVOKED;
    /* actor_id = caller_id real de quien revoca. */
    (void)audit_record_event(st, caller_id, OP_REVOKE_CRED, cred_id, EDU_OK);
    return EDU_OK;
}

/* ==========================================================================
 * Integracion con el flujo de solicitudes (Rev 5 §8-ter) — ADITIVA, SOLO LECTURA.
 * Opcion 2: credential_issue queda INTACTA; esta via nueva consulta la trayectoria
 * vigente y delega. (La Rev 5 §8-ter especificaba la Opcion 1 -ampliar la firma de
 * credential_issue-; se usa la via aditiva por el alcance Opcion 2, sin romper
 * call-sites. Ver nota en credentials.h.)
 * ========================================================================== */

/* credential_eligible: >=1 registro VIGENTE REQ_RECORDED del owner en
 * CAT_INSTITUTION_WORKED o CAT_LINK_PERIOD. Solo lectura. Patron gemelo de
 * experience_validated_count: busqueda lineal con flag acotado, sin aritmetica
 * de fechas. REQ_SUPERSEDED queda excluido por construccion (no es REQ_RECORDED). */
int credential_eligible(const requests_state_t *rs, int owner_id)
{
    int i;
    int found = 0;
    if (rs == 0) {
        return 0;
    }
    /*@
      loop invariant 0 <= i <= rs->reqs_count;
      loop invariant 0 <= found <= 1;
      loop invariant (found == 1) <==>
          (\exists integer k; 0 <= k < i
           && rs->reqs[k].owner_id == owner_id
           && rs->reqs[k].state == REQ_RECORDED
           && (rs->reqs[k].category == CAT_INSTITUTION_WORKED
               || rs->reqs[k].category == CAT_LINK_PERIOD));
      loop assigns i, found;
      loop variant rs->reqs_count - i;
    */
    for (i = 0; i < rs->reqs_count; i++) {
        if (rs->reqs[i].owner_id == owner_id &&
            rs->reqs[i].state == REQ_RECORDED &&
            (rs->reqs[i].category == CAT_INSTITUTION_WORKED ||
             rs->reqs[i].category == CAT_LINK_PERIOD)) {
            found = 1;
        }
    }
    return found;
}

/* credential_issue_eligible: emite SOLO si hay trayectoria vigente que habilite.
 * Valida elegibilidad ANTES de delegar (no muta si no es elegible). La emision en
 * si (rol COORD, ids, profesional activo, duplicados, capacidad, auditoria) la
 * hace credential_issue intacta. rs es de SOLO LECTURA. */
int credential_issue_eligible(educhain_state_t *st, const requests_state_t *rs,
                              role_t caller_role, int caller_id, int cred_id, int owner_id)
{
    if (st == 0 || rs == 0) {
        return ERR_NULL;
    }
    /* Elegibilidad obligatoria (consulta de solo lectura sobre la trayectoria
     * vigente REQ_RECORDED). Sin ella, NUNCA se emite y no se muta nada. */
    if (!credential_eligible(rs, owner_id)) {
        return ERR_PERM;
    }
    /* Delega en la emision existente (conserva toda su validacion y auditoria). */
    return credential_issue(st, caller_role, caller_id, cred_id, owner_id);
}
