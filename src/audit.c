/* EduChain - audit.c  (Fase 3)
 * Historial append-only. seq estrictamente creciente, sin sobrescribir eventos.
 */
#include "audit.h"

int audit_record_event(educhain_state_t *st, int actor_id, int op_code,
                        int target_id, int result)
{
    int next_seq;

    if (st == 0) {
        return ERR_NULL;
    }
    /* Capacidad (BR-8/BR-11). */
    if (st->log_count < 0 || st->log_count >= MAX_EVENTS) {
        return ERR_FULL;
    }
    /* seq estrictamente creciente, sin desbordar (P-10, P-12). */
    if (st->log_count == 0) {
        next_seq = 1;
    } else {
        int last = st->log[st->log_count - 1].seq;
        if (last >= 2000000000) {   /* margen seguro bajo INT_MAX */
            return ERR_FULL;
        }
        next_seq = last + 1;
    }

    st->log[st->log_count].seq       = next_seq;
    st->log[st->log_count].actor_id  = actor_id;
    st->log[st->log_count].op_code   = op_code;
    st->log[st->log_count].target_id = target_id;
    st->log[st->log_count].result    = result;
    st->log_count = st->log_count + 1;
    return EDU_OK;
}

int audit_verify_event(const educhain_state_t *st, int seq)
{
    int i;
    if (st == 0) {
        return -1;
    }
    /*@
      loop invariant 0 <= i <= st->log_count;
      loop invariant \forall integer k; 0 <= k < i ==> st->log[k].seq != seq;
      loop assigns i;
      loop variant st->log_count - i;
    */
    for (i = 0; i < st->log_count; i++) {
        if (st->log[i].seq == seq) {
            return i;
        }
    }
    return -1;
}

int audit_get_count(const educhain_state_t *st)
{
    if (st == 0) {
        return 0;
    }
    return st->log_count;
}
