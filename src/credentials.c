/* EduChain - credentials.c  (Fase 3)
 * Maquina de estados con transiciones explicitas (Fase 1 §5.1).
 */
#include "credentials.h"
#include "professionals.h"
#include "security.h"
#include "audit.h"

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
    assigns { st->creds[k].state | integer k; 0 <= k < st->creds_count && st->creds[k].id == cred_id },
            st->log[st->log_count], st->log_count;
    ensures \result == EDU_OK || \result == ERR_PERM || \result == ERR_BAD_ID
         || \result == ERR_NOT_FOUND || \result == ERR_BAD_TRANSITION || \result == ERR_FULL;
    ensures st->creds_count == \old(st->creds_count);
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
