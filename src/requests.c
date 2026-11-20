/* EduChain - requests.c  (ETAPA 2, PRIMER ENTREGABLE)
 * Flujo de solicitudes (presentador -> verificador -> administrador).
 * Fuente de verdad: docs/diseno_flujo_solicitudes.md (Rev 5).
 *
 * Modulo AISLADO: NO llama a security_can (la integracion con security.c es un
 * entregable posterior); lleva su propia comprobacion de rol interna
 * (request_role_can, switch puro, patron security_can). Usa audit_record_event
 * para auditar en el log compartido (educhain_state_t.log), con actor_id =
 * caller_id (quien ejecuta), NUNCA el titular owner_id.
 *
 * Alcance de este entregable: request_init, request_find, request_status,
 * request_requires_endorsement, request_submit, request_submit_correction,
 * request_endorse, request_verify, request_reject. NO incluye request_record,
 * consultas publicas *_for, ni la gestion de auth[] (authz_grant/revoke).
 */
#include "requests.h"
#include "audit.h"
#ifdef __FRAMAC__
#include "specifications.h"
#endif

/* ==========================================================================
 * Helpers internos (solo lectura o puros).
 * ========================================================================== */

/* Comprobacion de rol del flujo nuevo (switch puro, patron security_can).
 * SOLO las OPs de este entregable; refleja req_role_can (requests.h) y la
 * matriz Rev 5 §3.3. HR/COORD/ADMIN quedan negados en estas OPs (separacion
 * estricta del flujo nuevo). Cuando el entregable posterior integre security_can,
 * esta funcion se reemplaza por la llamada real. */
/*@ assigns \nothing;
    ensures \result == 0 || \result == 1;
    ensures \result == 1 <==> req_role_can(caller_role, op_code);
*/
static int request_role_can(role_t caller_role, int op_code)
{
    switch (op_code) {
    case OP_SUBMIT_REQUEST:
        return (caller_role == ROLE_INSTITUTION || caller_role == ROLE_PRO) ? 1 : 0;
    case OP_ENDORSE_REQUEST:
        return (caller_role == ROLE_INSTITUTION) ? 1 : 0;
    case OP_VERIFY_REQUEST:
    case OP_REJECT_REQUEST:
        return (caller_role == ROLE_VERIFIER) ? 1 : 0;
    case OP_RECORD_REQUEST:
        return (caller_role == ROLE_ADMIN) ? 1 : 0;
    default:
        return 0;
    }
}

/* Fecha dentro del rango valido (copia local, patron experience.c). */
/*@ assigns \nothing;
    ensures \result == 0 || \result == 1;
    ensures \result == 1 <==> (DATE_MIN <= d <= DATE_MAX);
*/
static int date_in_range(int d)
{
    return (d >= DATE_MIN && d <= DATE_MAX) ? 1 : 0;
}

/* exige_respaldo efectivo (Rev 5 §5.3 bis): la categoria exige respaldo, o es
 * un proyecto de investigacion con institucion asociada (institution_id != 0). */
/*@ assigns \nothing;
    ensures \result == 0 || \result == 1;
*/
static int req_needs_endorsement(int category, int institution_id)
{
    if (request_requires_endorsement(category)) {
        return 1;
    }
    if (category == CAT_RESEARCH_PROJECT && institution_id != 0) {
        return 1;
    }
    return 0;
}

/* Vinculo de representacion ACTIVO user_id -> institution_id en auth[].
 * Indice 0..auth_count-1, o -1 si no hay vinculo activo. Solo lectura.
 * Gemelo de cred_find_by_owner. La GESTION de auth[] (grant/revoke) es de un
 * entregable posterior; aqui solo se LEE. */
/*@ requires \valid_read(rs);
    requires 0 <= rs->auth_count <= MAX_AUTHZ;
    assigns \nothing;
    ensures -1 <= \result < rs->auth_count;
    ensures \result >= 0 ==>
            (rs->auth[\result].user_id == user_id
             && rs->auth[\result].institution_id == institution_id
             && rs->auth[\result].active == 1);
*/
static int req_find_authz_active(const requests_state_t *rs, int user_id, int institution_id)
{
    int i;
    /*@
      loop invariant 0 <= i <= rs->auth_count;
      loop invariant \forall integer k; 0 <= k < i ==>
          !(rs->auth[k].user_id == user_id
            && rs->auth[k].institution_id == institution_id
            && rs->auth[k].active == 1);
      loop assigns i;
      loop variant rs->auth_count - i;
    */
    for (i = 0; i < rs->auth_count; i++) {
        if (rs->auth[i].user_id == user_id &&
            rs->auth[i].institution_id == institution_id &&
            rs->auth[i].active == 1) {
            return i;
        }
    }
    return -1;
}

/* ==========================================================================
 * Inicializacion y consultas primitivas.
 * ========================================================================== */

void request_init(requests_state_t *rs)
{
    if (rs == 0) {
        return;
    }
    rs->reqs_count = 0;
    rs->auth_count = 0;
}

int request_find(const requests_state_t *rs, int req_id)
{
    int i;
    if (rs == 0) {
        return -1;
    }
    /*@
      loop invariant 0 <= i <= rs->reqs_count;
      loop invariant \forall integer k; 0 <= k < i ==> rs->reqs[k].id != req_id;
      loop assigns i;
      loop variant rs->reqs_count - i;
    */
    for (i = 0; i < rs->reqs_count; i++) {
        if (rs->reqs[i].id == req_id) {
            return i;
        }
    }
    return -1;
}

int request_status(const requests_state_t *rs, int req_id)
{
    int i;
    if (rs == 0) {
        return -1;
    }
    /*@
      loop invariant 0 <= i <= rs->reqs_count;
      loop invariant \forall integer k; 0 <= k < i ==> rs->reqs[k].id != req_id;
      loop assigns i;
      loop variant rs->reqs_count - i;
    */
    for (i = 0; i < rs->reqs_count; i++) {
        if (rs->reqs[i].id == req_id) {
            return (int)rs->reqs[i].state;
        }
    }
    return -1;
}

int request_requires_endorsement(int category)
{
    switch (category) {
    case CAT_INSTITUTION_WORKED:
    case CAT_LINK_PERIOD:
    case CAT_LINK_TYPE_DEDICATION:
    case CAT_COURSES_TAUGHT:
    case CAT_ACADEMIC_PERIOD:
    case CAT_TEACHING_HOURS:
    case CAT_ACADEMIC_ACTIVITY:
    case CAT_THESIS_SUPERVISION:
    case CAT_OUTREACH:
    case CAT_COMMITTEES:
        return 1;
    case CAT_RESEARCH_PROJECT:   /* condicional por institution_id, §5.3 bis -> 0 base */
    case CAT_PUBLICATION:
    case CAT_TRAINING:
    case CAT_CERTIFICATION:
        return 0;
    default:
        return 0;
    }
}

/* ==========================================================================
 * Insercion comun de una solicitud (original o correccion).
 * Precondicion de llamada: ya validados rol, ids, fechas, duplicado y capacidad.
 * Escribe el slot reqs[reqs_count], fija el estado de nacimiento (§5.0) y audita.
 * ========================================================================== */
/*@ requires \valid(st) && \valid(rs);
    requires \separated(st, rs);   // estados separados (Rev 5 §4.5): st y rs distintos
    requires 0 <= rs->reqs_count < MAX_REQUESTS;
    requires 0 <= st->log_count  < MAX_EVENTS;
    assigns rs->reqs[rs->reqs_count], rs->reqs_count,
            st->log[st->log_count], st->log_count;
    ensures rs->reqs_count == \old(rs->reqs_count) + 1;
    ensures \result == EDU_OK;
*/
static int req_insert(educhain_state_t *st, requests_state_t *rs,
                      role_t caller_role, int caller_id, int req_id, int owner_id,
                      int institution_id, int category, int start_date, int end_date,
                      int evidence_ref, int prev_id,
                      int num_a, int num_b, int num_c, int num_d)
{
    int n = rs->reqs_count;
    int needs = req_needs_endorsement(category, institution_id);
    /*@ assert 0 <= n < MAX_REQUESTS; */
    rs->reqs[n].id             = req_id;
    rs->reqs[n].owner_id       = owner_id;
    rs->reqs[n].institution_id = institution_id;
    rs->reqs[n].submitter_id   = caller_id;
    rs->reqs[n].verifier_id    = 0;
    rs->reqs[n].category       = (request_category_t)category;
    rs->reqs[n].start_date     = start_date;
    rs->reqs[n].end_date       = end_date;
    rs->reqs[n].evidence_ref   = evidence_ref;
    rs->reqs[n].prev_id        = prev_id;
    rs->reqs[n].num_a          = num_a;
    rs->reqs[n].num_b          = num_b;
    rs->reqs[n].num_c          = num_c;
    rs->reqs[n].num_d          = num_d;

    /* Nacimiento (Rev 5 §5.0 / §5.3 ter): INSTITUTION que presenta una categoria
     * que exige respaldo deja constancia en el acto (nace ENDORSED, fija
     * endorser_id). PROFESSOR o categoria sin respaldo nacen PENDING. */
    if (needs && caller_role == ROLE_INSTITUTION) {
        rs->reqs[n].state       = REQ_ENDORSED;
        rs->reqs[n].endorser_id = institution_id;
    } else {
        rs->reqs[n].state       = REQ_PENDING;
        rs->reqs[n].endorser_id = 0;
    }

    rs->reqs_count = n + 1;

    /* actor_id = caller_id (quien presenta), NO el titular owner_id. */
    (void)audit_record_event(st, caller_id, OP_SUBMIT_REQUEST, req_id, EDU_OK);
    return EDU_OK;
}

/* Validaciones comunes de presentacion (rol, ids, representacion, categoria,
 * fechas, duplicado, capacidad). Devuelve EDU_OK si todo pasa, o el ERR_*.
 * No muta nada. */
/*@ requires \valid_read(rs);
    requires 0 <= rs->reqs_count <= MAX_REQUESTS;
    requires 0 <= rs->auth_count <= MAX_AUTHZ;
    requires 0 <= st->log_count  <= MAX_EVENTS;
    assigns \nothing;
    ensures \result == EDU_OK || \result == ERR_PERM || \result == ERR_BAD_ID
         || \result == ERR_BAD_DATE || \result == ERR_DUPLICATE || \result == ERR_FULL;
    ensures !req_role_can(caller_role, OP_SUBMIT_REQUEST)
            ==> (\result == ERR_PERM || \result == ERR_BAD_ID);
    // En exito, el cuerpo ya comprobo que hay hueco y el permiso: se exporta para
    // que los llamadores deriven la precondicion de req_insert y la atomicidad.
    ensures \result == EDU_OK ==> (rs->reqs_count < MAX_REQUESTS
                                && st->log_count < MAX_EVENTS
                                && req_role_can(caller_role, OP_SUBMIT_REQUEST));
*/
static int req_submit_checks(const educhain_state_t *st, const requests_state_t *rs,
                             role_t caller_role, int caller_id, int req_id, int owner_id,
                             int institution_id, int category, int start_date, int end_date)
{
    if (caller_id < ID_MIN || caller_id > ID_MAX ||
        req_id < ID_MIN || req_id > ID_MAX ||
        owner_id < ID_MIN || owner_id > ID_MAX) {
        return ERR_BAD_ID;
    }
    /* institution_id: 0 = no aplica; si no, en rango. */
    if (institution_id != 0 && (institution_id < ID_MIN || institution_id > ID_MAX)) {
        return ERR_BAD_ID;
    }
    if (!request_role_can(caller_role, OP_SUBMIT_REQUEST)) {
        return ERR_PERM;
    }
    /* PROFESSOR solo presenta lo propio (patron experience_register). */
    if (caller_role == ROLE_PRO && caller_id != owner_id) {
        return ERR_PERM;
    }
    /* INSTITUTION solo representa lo autorizado (§3.2): vinculo activo en auth[].
     * institution_id debe estar presente (!= 0) para una INSTITUTION. */
    if (caller_role == ROLE_INSTITUTION) {
        if (institution_id == 0) {
            return ERR_PERM;
        }
        if (req_find_authz_active(rs, caller_id, institution_id) < 0) {
            return ERR_PERM;
        }
    }
    /* Categoria valida 0..13. */
    if (category < CAT_INSTITUTION_WORKED || category > CAT_CERTIFICATION) {
        return ERR_BAD_ID;
    }
    /* Fechas validas (BR-4). */
    if (!date_in_range(start_date) || !date_in_range(end_date)) {
        return ERR_BAD_DATE;
    }
    if (start_date > end_date) {
        return ERR_BAD_DATE;
    }
    /* Id unico. */
    if (request_find(rs, req_id) >= 0) {
        return ERR_DUPLICATE;
    }
    /* Capacidad y hueco de log. */
    if (rs->reqs_count < 0 || rs->reqs_count >= MAX_REQUESTS) {
        return ERR_FULL;
    }
    if (st->log_count >= MAX_EVENTS) {
        return ERR_FULL;
    }
    return EDU_OK;
}

/* ==========================================================================
 * request_submit: solicitud ORIGINAL (prev_id = 0).
 * ========================================================================== */
int request_submit(educhain_state_t *st, requests_state_t *rs,
                   role_t caller_role, int caller_id, int req_id, int owner_id,
                   int institution_id, int category, int start_date, int end_date,
                   int evidence_ref, int num_a, int num_b, int num_c, int num_d)
{
    int rc;
    if (st == 0 || rs == 0) {
        return ERR_NULL;
    }
    rc = req_submit_checks(st, rs, caller_role, caller_id, req_id, owner_id,
                           institution_id, category, start_date, end_date);
    if (rc != EDU_OK) {
        return rc;
    }
    return req_insert(st, rs, caller_role, caller_id, req_id, owner_id,
                      institution_id, category, start_date, end_date,
                      evidence_ref, 0 /* prev_id */, num_a, num_b, num_c, num_d);
}

/* ==========================================================================
 * request_submit_correction: CORRECCION (prev_id != 0), coherencia T-1.
 * ========================================================================== */
int request_submit_correction(educhain_state_t *st, requests_state_t *rs,
                              role_t caller_role, int caller_id, int req_id, int owner_id,
                              int institution_id, int category, int start_date, int end_date,
                              int evidence_ref, int prev_id,
                              int num_a, int num_b, int num_c, int num_d)
{
    int rc;
    int prev;
    int cat_has_inst;
    if (st == 0 || rs == 0) {
        return ERR_NULL;
    }
    /* Validaciones comunes de presentacion (rol, ids, representacion, fechas,
     * duplicado del nuevo id, capacidad). */
    rc = req_submit_checks(st, rs, caller_role, caller_id, req_id, owner_id,
                           institution_id, category, start_date, end_date);
    if (rc != EDU_OK) {
        return rc;
    }
    /* prev_id valido y distinto de 0 (una correccion SIEMPRE apunta a un original). */
    if (prev_id < ID_MIN || prev_id > ID_MAX) {
        return ERR_BAD_ID;
    }
    /* Validacion-S al PRESENTAR (Rev 5 §5.5). */
    prev = request_find(rs, prev_id);
    if (prev < 0) {
        return ERR_NOT_FOUND;
    }
    /* El anterior debe estar vigente (REQ_RECORDED). */
    if (rs->reqs[prev].state != REQ_RECORDED) {
        return ERR_BAD_TRANSITION;
    }
    /* Mismo propietario que el registro corregido. */
    if (rs->reqs[prev].owner_id != owner_id) {
        return ERR_PERM;
    }
    /* Coherencia categoria/institucion T-1 (Rev 5 §5.5):
     * la categoria SIEMPRE coincide con la del original. */
    if ((int)rs->reqs[prev].category != category) {
        return ERR_BAD_TRANSITION;
    }
    /* Institucion fijada por la categoria, sin grado de libertad:
     * si la categoria asocia institucion, mismo institution_id que el original;
     * si no la asocia, ambos institution_id == 0. "Asocia institucion" se decide
     * por la regla efectiva de respaldo (§5.3/§5.3 bis): las categorias con
     * respaldo (o investigacion con institucion) asocian institucion. */
    cat_has_inst = req_needs_endorsement(category, institution_id);
    if (cat_has_inst) {
        if (institution_id != rs->reqs[prev].institution_id) {
            return ERR_BAD_TRANSITION;
        }
    } else {
        if (institution_id != 0 || rs->reqs[prev].institution_id != 0) {
            return ERR_BAD_TRANSITION;
        }
    }

    return req_insert(st, rs, caller_role, caller_id, req_id, owner_id,
                      institution_id, category, start_date, end_date,
                      evidence_ref, prev_id, num_a, num_b, num_c, num_d);
}

/* ==========================================================================
 * request_endorse: PENDING -> ENDORSED (INSTITUTION que representa).
 * ========================================================================== */
int request_endorse(educhain_state_t *st, requests_state_t *rs,
                    role_t caller_role, int caller_id, int req_id)
{
    int idx;
    if (st == 0 || rs == 0) {
        return ERR_NULL;
    }
    if (caller_id < ID_MIN || caller_id > ID_MAX ||
        req_id < ID_MIN || req_id > ID_MAX) {
        return ERR_BAD_ID;
    }
    if (!request_role_can(caller_role, OP_ENDORSE_REQUEST)) {
        return ERR_PERM;
    }
    idx = request_find(rs, req_id);
    if (idx < 0) {
        return ERR_NOT_FOUND;
    }
    /* Estado origen exacto: solo se respalda una PENDING (patron cred_transition). */
    if (rs->reqs[idx].state != REQ_PENDING) {
        return ERR_BAD_TRANSITION;
    }
    /* Solo categorias que exigen respaldo: si no hay nada que respaldar, se rechaza. */
    if (!req_needs_endorsement((int)rs->reqs[idx].category, rs->reqs[idx].institution_id)) {
        return ERR_BAD_TRANSITION;
    }
    /* La INSTITUTION debe representar a la institucion de la solicitud (§3.2). */
    if (rs->reqs[idx].institution_id == 0 ||
        req_find_authz_active(rs, caller_id, rs->reqs[idx].institution_id) < 0) {
        return ERR_PERM;
    }
    if (st->log_count >= MAX_EVENTS) {
        return ERR_FULL;
    }

    rs->reqs[idx].state       = REQ_ENDORSED;
    rs->reqs[idx].endorser_id = rs->reqs[idx].institution_id;

    (void)audit_record_event(st, caller_id, OP_ENDORSE_REQUEST, req_id, EDU_OK);
    return EDU_OK;
}

/* ==========================================================================
 * request_verify: -> VERIFIED (VERIFIER, no autovalidacion).
 * ========================================================================== */
int request_verify(educhain_state_t *st, requests_state_t *rs,
                   role_t caller_role, int caller_id, int req_id)
{
    int idx;
    int needs;
    if (st == 0 || rs == 0) {
        return ERR_NULL;
    }
    if (caller_id < ID_MIN || caller_id > ID_MAX ||
        req_id < ID_MIN || req_id > ID_MAX) {
        return ERR_BAD_ID;
    }
    if (!request_role_can(caller_role, OP_VERIFY_REQUEST)) {
        return ERR_PERM;
    }
    idx = request_find(rs, req_id);
    if (idx < 0) {
        return ERR_NOT_FOUND;
    }
    /* El VERIFIER no aprueba solicitudes presentadas por el mismo (Rev 5 §3.4;
     * patron experience_validate), con independencia del estado. */
    if (rs->reqs[idx].submitter_id == caller_id) {
        return ERR_SELF_VALIDATE;
    }
    /* Estado origen: si exige respaldo, debe estar ENDORSED; si no, PENDING.
     * Exige respaldo pero aun PENDING -> ERR_BAD_TRANSITION (P-14). */
    needs = req_needs_endorsement((int)rs->reqs[idx].category, rs->reqs[idx].institution_id);
    if (needs) {
        if (rs->reqs[idx].state != REQ_ENDORSED) {
            return ERR_BAD_TRANSITION;
        }
    } else {
        if (rs->reqs[idx].state != REQ_PENDING) {
            return ERR_BAD_TRANSITION;
        }
    }
    if (st->log_count >= MAX_EVENTS) {
        return ERR_FULL;
    }

    rs->reqs[idx].state       = REQ_VERIFIED;
    rs->reqs[idx].verifier_id = caller_id;

    (void)audit_record_event(st, caller_id, OP_VERIFY_REQUEST, req_id, EDU_OK);
    return EDU_OK;
}

/* ==========================================================================
 * request_reject: {PENDING|ENDORSED} -> REJECTED (VERIFIER, no autovalidacion).
 * ========================================================================== */
int request_reject(educhain_state_t *st, requests_state_t *rs,
                   role_t caller_role, int caller_id, int req_id)
{
    int idx;
    if (st == 0 || rs == 0) {
        return ERR_NULL;
    }
    if (caller_id < ID_MIN || caller_id > ID_MAX ||
        req_id < ID_MIN || req_id > ID_MAX) {
        return ERR_BAD_ID;
    }
    if (!request_role_can(caller_role, OP_REJECT_REQUEST)) {
        return ERR_PERM;
    }
    idx = request_find(rs, req_id);
    if (idx < 0) {
        return ERR_NOT_FOUND;
    }
    if (rs->reqs[idx].submitter_id == caller_id) {
        return ERR_SELF_VALIDATE;
    }
    /* Solo se rechaza una solicitud en tramite (PENDING o ENDORSED). */
    if (rs->reqs[idx].state != REQ_PENDING && rs->reqs[idx].state != REQ_ENDORSED) {
        return ERR_BAD_TRANSITION;
    }
    if (st->log_count >= MAX_EVENTS) {
        return ERR_FULL;
    }

    rs->reqs[idx].state = REQ_REJECTED;

    (void)audit_record_event(st, caller_id, OP_REJECT_REQUEST, req_id, EDU_OK);
    return EDU_OK;
}

/* ==========================================================================
 * request_record: incorporacion al historial oficial. SOLO ADMIN (Rev 5 §5.4).
 *   Variante A (prev_id == 0): REQ_VERIFIED -> REQ_RECORDED (simple).
 *   Variante B (prev_id != 0): en la misma operacion atomica,
 *     req_id: REQ_VERIFIED -> REQ_RECORDED  y  prev_id: REQ_RECORDED -> REQ_SUPERSEDED.
 * Patron "validar TODO antes de mutar NADA" (atomicidad BR-10): toda la validacion-R
 * (§5.5) se ejecuta antes de la primera escritura, de modo que en cualquier error
 * NINGUNO de los dos slots se modifica. El anterior cambia SOLO su campo state.
 * ========================================================================== */
int request_record(educhain_state_t *st, requests_state_t *rs,
                   role_t caller_role, int caller_id, int req_id)
{
    int idx;
    int prev_id;
    int prev;
    int cat_has_inst;
    if (st == 0 || rs == 0) {
        return ERR_NULL;
    }
    if (caller_id < ID_MIN || caller_id > ID_MAX ||
        req_id < ID_MIN || req_id > ID_MAX) {
        return ERR_BAD_ID;
    }
    /* SOLO ADMIN (separacion estricta del flujo nuevo). */
    if (!request_role_can(caller_role, OP_RECORD_REQUEST)) {
        return ERR_PERM;
    }
    idx = request_find(rs, req_id);
    if (idx < 0) {
        return ERR_NOT_FOUND;
    }
    /* La solicitud a incorporar debe estar aprobada (REQ_VERIFIED), ambas variantes. */
    if (rs->reqs[idx].state != REQ_VERIFIED) {
        return ERR_BAD_TRANSITION;
    }

    prev_id = rs->reqs[idx].prev_id;

    /* ---------- Variante B: correccion con sustitucion (prev_id != 0) ----------
     * Validacion-R COMPLETA antes de tocar nada (re-valida prev_id: su estado pudo
     * cambiar entre presentar e incorporar). */
    if (prev_id != 0) {
        /* 1. prev_id existe. */
        prev = request_find(rs, prev_id);
        if (prev < 0) {
            return ERR_NOT_FOUND;
        }
        /* 2. prev vigente REQ_RECORDED. Rechaza re-sustituir un REQ_SUPERSEDED y la
         *    2a correccion de una carrera (prev ya paso a SUPERSEDED): falla LIMPIO. */
        if (rs->reqs[prev].state != REQ_RECORDED) {
            return ERR_BAD_TRANSITION;
        }
        /* 3. Mismo propietario que el registro corregido. */
        if (rs->reqs[idx].owner_id != rs->reqs[prev].owner_id) {
            return ERR_PERM;
        }
        /* 4. Coherencia T-1: misma categoria; institucion fijada por la categoria. */
        if ((int)rs->reqs[idx].category != (int)rs->reqs[prev].category) {
            return ERR_BAD_TRANSITION;
        }
        cat_has_inst = req_needs_endorsement((int)rs->reqs[idx].category,
                                             rs->reqs[idx].institution_id);
        if (cat_has_inst) {
            if (rs->reqs[idx].institution_id != rs->reqs[prev].institution_id) {
                return ERR_BAD_TRANSITION;
            }
        } else {
            if (rs->reqs[idx].institution_id != 0 || rs->reqs[prev].institution_id != 0) {
                return ERR_BAD_TRANSITION;
            }
        }
        /* 6. Hueco de log (5 ya comprobada arriba: req_id esta REQ_VERIFIED). */
        if (st->log_count >= MAX_EVENTS) {
            return ERR_FULL;
        }

        /* --- Mutacion atomica: ambos slots, SOLO su campo state. --- */
        rs->reqs[idx].state  = REQ_RECORDED;
        rs->reqs[prev].state = REQ_SUPERSEDED;

        (void)audit_record_event(st, caller_id, OP_RECORD_REQUEST, req_id, EDU_OK);
        return EDU_OK;
    }

    /* ---------- Variante A: incorporacion simple (prev_id == 0) ---------- */
    if (st->log_count >= MAX_EVENTS) {
        return ERR_FULL;
    }

    rs->reqs[idx].state = REQ_RECORDED;

    (void)audit_record_event(st, caller_id, OP_RECORD_REQUEST, req_id, EDU_OK);
    return EDU_OK;
}
