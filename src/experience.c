/* EduChain - experience.c  (Fase 3) */
#include "experience.h"
#include "professionals.h"
#include "security.h"
#include "audit.h"

int experience_find(const educhain_state_t *st, int exp_id)
{
    int i;
    if (st == 0) {
        return -1;
    }
    /*@
      loop invariant 0 <= i <= st->exps_count;
      loop invariant \forall integer k; 0 <= k < i ==> st->exps[k].id != exp_id;
      loop assigns i;
      loop variant st->exps_count - i;
    */
    for (i = 0; i < st->exps_count; i++) {
        if (st->exps[i].id == exp_id) {
            return i;
        }
    }
    return -1;
}

static int date_in_range(int d)
{
    return (d >= DATE_MIN && d <= DATE_MAX) ? 1 : 0;
}



int experience_register(educhain_state_t *st, role_t caller_role, int caller_id,
                        int exp_id, int owner_id, int start_date, int end_date)
{
    if (st == 0) {
        return ERR_NULL;
    }
    if (exp_id < ID_MIN || exp_id > ID_MAX ||
        owner_id < ID_MIN || owner_id > ID_MAX ||
        caller_id < ID_MIN || caller_id > ID_MAX) {
        return ERR_BAD_ID;
    }
    if (!security_can(caller_role, OP_REGISTER_EXP)) {
        return ERR_PERM;
    }
    /* Un PRO solo registra experiencia propia. */
    if (caller_role == ROLE_PRO && caller_id != owner_id) {
        return ERR_PERM;
    }
    /* Fechas validas: rango y start <= end (BR-4). */
    if (!date_in_range(start_date) || !date_in_range(end_date)) {
        return ERR_BAD_DATE;
    }
    if (start_date > end_date) {
        return ERR_BAD_DATE;
    }
    if (!professional_is_active(st, owner_id)) {
        return ERR_NOT_FOUND;
    }
    if (experience_find(st, exp_id) >= 0) {
        return ERR_DUPLICATE;
    }
    if (st->exps_count < 0 || st->exps_count >= MAX_EXPERIENCES) {
        return ERR_FULL;
    }
    if (st->log_count >= MAX_EVENTS) {
        return ERR_FULL;
    }

    {
        int n = st->exps_count;
        /*@ assert 0 <= n < MAX_EXPERIENCES; */
        st->exps[n].id           = exp_id;
        st->exps[n].owner_id     = owner_id;
        st->exps[n].start_date   = start_date;
        st->exps[n].end_date     = end_date;
        st->exps[n].state        = EXP_PENDING;
        st->exps[n].validator_id = 0;
        st->exps_count = n + 1;
    }

    (void)audit_record_event(st, caller_id, OP_REGISTER_EXP, exp_id, EDU_OK);
    return EDU_OK;
}

int experience_validate(educhain_state_t *st, role_t caller_role, int caller_id, int exp_id)
{
    int idx;
    if (st == 0) {
        return ERR_NULL;
    }
    if (caller_id < ID_MIN || caller_id > ID_MAX || exp_id < ID_MIN || exp_id > ID_MAX) {
        return ERR_BAD_ID;
    }
    if (!security_can(caller_role, OP_VALIDATE_EXP)) {
        return ERR_PERM;
    }
    idx = experience_find(st, exp_id);
    if (idx < 0) {
        return ERR_NOT_FOUND;
    }
    /* No autovalidacion (BR-2), con independencia del rol. */
    if (st->exps[idx].owner_id == caller_id) {
        return ERR_SELF_VALIDATE;
    }
    /* Inmutabilidad: si ya esta validada, no se toca (BR-5). */
    if (st->exps[idx].state == EXP_VALIDATED) {
        return ERR_ALREADY_VALID;
    }
    if (st->log_count >= MAX_EVENTS) {
        return ERR_FULL;
    }

    /* Solo cambian state y validator_id; fechas y owner NO cambian. */
    st->exps[idx].state        = EXP_VALIDATED;
    st->exps[idx].validator_id = caller_id;

    (void)audit_record_event(st, caller_id, OP_VALIDATE_EXP, exp_id, EDU_OK);
    return EDU_OK;
}

int experience_validated_count(const educhain_state_t *st, int owner_id)
{
    int count = 0;
    int i;
    if (st == 0) {
        return ERR_NULL;
    }
    if (owner_id < ID_MIN || owner_id > ID_MAX) {
        return ERR_BAD_ID;
    }
    if (!professional_is_active(st, owner_id)) {
        return ERR_NOT_FOUND;
    }
    /* Cuenta experiencias VALIDADAS del titular. Sin aritmetica de fechas.
     * Cada indice se visita una sola vez (no hay doble conteo). */
    /*@
      loop invariant 0 <= i <= st->exps_count;
      loop invariant 0 <= count <= i;
      loop assigns i, count;
      loop variant st->exps_count - i;
    */
    for (i = 0; i < st->exps_count; i++) {
        if (st->exps[i].owner_id == owner_id &&
            st->exps[i].state == EXP_VALIDATED) {
            count = count + 1;
        }
    }
    return count;
}
