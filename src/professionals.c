/* EduChain - professionals.c  (Fase 3) */
#include "professionals.h"
#include "security.h"
#include "audit.h"

int professional_find(const educhain_state_t *st, int id)
{
    int i;
    if (st == 0) {
        return -1;
    }
    /*@
      loop invariant 0 <= i <= st->pros_count;
      loop invariant \forall integer k; 0 <= k < i ==> st->pros[k].id != id;
      loop assigns i;
      loop variant st->pros_count - i;
    */
    for (i = 0; i < st->pros_count; i++) {
        if (st->pros[i].id == id) {
            return i;
        }
    }
    return -1;
}

int professional_is_active(const educhain_state_t *st, int id)
{
    int idx = professional_find(st, id);
    if (idx < 0) {
        return 0;
    }
    return st->pros[idx].active ? 1 : 0;
}

int professional_register(educhain_state_t *st, role_t caller_role, int caller_id, int new_id)
{
    /* Patron: validar TODO antes de mutar NADA (atomicidad, BR-10). */
    if (st == 0) {
        return ERR_NULL;
    }
    /* caller_id y new_id deben estar en rango. caller_id NO requiere existir
     * en professionals[] (puede ser un admin/coordinador no profesional). */
    if (caller_id < ID_MIN || caller_id > ID_MAX ||
        new_id < ID_MIN || new_id > ID_MAX) {
        return ERR_BAD_ID;
    }
    if (!security_can(caller_role, OP_REGISTER_PRO)) {
        return ERR_PERM;
    }
    if (professional_find(st, new_id) >= 0) {
        return ERR_DUPLICATE;
    }
    if (st->pros_count < 0 || st->pros_count >= MAX_PROFESSIONALS) {
        return ERR_FULL;
    }
    /* Reservar espacio de log ANTES de mutar (concilia BR-10 + BR-11). */
    if (st->log_count >= MAX_EVENTS) {
        return ERR_FULL;
    }
    {
        /* Snapshot del indice: ayuda a WP a tratar n como valor fijo. */
        int n = st->pros_count;
        /*@ assert 0 <= n < MAX_PROFESSIONALS; */
        st->pros[n].id     = new_id;
        st->pros[n].active = 1;
        st->pros_count = n + 1;
    }

    /* actor_id = caller_id real (quien ejecuta); target_id = new_id (registro afectado).
     * audit_record_event preserva pros_count/exps_count/creds_count (frame ensures). */
    (void)audit_record_event(st, caller_id, OP_REGISTER_PRO, new_id, EDU_OK);
    return EDU_OK;
}
