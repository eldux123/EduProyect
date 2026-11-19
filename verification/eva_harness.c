/* EduChain - eva_harness.c
 * Punto de entrada para el analisis de valores (Frama-C Eva).
 * Ejercita las funciones con un estado inicializado y entradas no deterministas
 * (Frama_C_interval) para que Eva explore rutas y busque OOB/overflow/no-init.
 * No forma parte de la libreria ni de las pruebas unitarias.
 */
#include "professionals.h"
#include "experience.h"
#include "credentials.h"
#include "audit.h"

extern int Frama_C_interval(int min, int max);

int eva_main(void)
{
    static educhain_state_t st; /* static => cero-inicializado, estado valido vacio */

    role_t r1  = (role_t)Frama_C_interval(0, 4);
    role_t r2  = (role_t)Frama_C_interval(0, 4);
    int caller = Frama_C_interval(1, 2000000000);  /* ejecutor (externo) */
    int id1    = Frama_C_interval(1, 2000000000);
    int id2    = Frama_C_interval(1, 2000000000);
    int cid    = Frama_C_interval(1, 2000000000);
    int sd     = Frama_C_interval(19000101, 21001231);
    int ed     = Frama_C_interval(19000101, 21001231);

    /* Firmas actualizadas con caller_id (Fase 4). */
    professional_register(&st, r1, caller, id1);
    professional_register(&st, r2, caller, id2);
    experience_register(&st, r1, caller, cid, id1, sd, ed);
    experience_validate(&st, r2, caller, cid);
    credential_issue(&st, r2, caller, cid, id1);
    credential_activate(&st, r2, caller, cid);
    credential_block(&st, r1, caller, cid);
    credential_reactivate(&st, r1, caller, cid);
    credential_revoke(&st, r1, caller, cid);

    (void)credential_is_valid(&st, cid);
    (void)credential_status(&st, cid);
    (void)experience_validated_count(&st, id1);
    (void)audit_get_count(&st);
    (void)audit_verify_event(&st, 1);
    return 0;
}
