/* EduChain - test_attacks.c
 * Pruebas adversariales: intentan romper reglas de seguridad y verifican el rechazo.
 * caller_id: 50 = admin/HR ejecutor, 60 = coordinador ejecutor.
 */
#include <string.h>
#include "test_framework.h"
#include "professionals.h"
#include "experience.h"
#include "credentials.h"
#include "audit.h"

void test_attacks(void)
{
    educhain_state_t st;
    int i, before_count, before_exps;
    printf("[test_attacks]\n");

    memset(&st, 0, sizeof(st));
    professional_register(&st, ROLE_HR, 50, 100);
    professional_register(&st, ROLE_HR, 50, 200);

    /* A1: escalada de privilegios - PRO intenta validar */
    experience_register(&st, ROLE_HR, 9, 5000, 100, 20200101, 20201231);
    CHECK_EQ(experience_validate(&st, ROLE_PRO, 200, 5000), ERR_PERM, "A1 escalada rechazada");

    /* A2: autovalidacion encubierta - HR dueno valida lo suyo */
    experience_register(&st, ROLE_HR, 200, 5001, 200, 20200101, 20201231);
    CHECK_EQ(experience_validate(&st, ROLE_HR, 200, 5001), ERR_SELF_VALIDATE, "A2 autovalidacion rechazada");

    /* A3: alterar registro validado -> operacion ordinaria no lo permite */
    experience_validate(&st, ROLE_COORD, 999, 5000);
    before_exps = st.exps_count;
    CHECK_EQ(experience_validate(&st, ROLE_COORD, 999, 5000), ERR_ALREADY_VALID, "A3 no se re-modifica lo validado");
    CHECK_EQ(st.exps_count, before_exps, "A3 conteo intacto");

    /* A4: duplicado de identificador (profesional, experiencia) */
    CHECK_EQ(professional_register(&st, ROLE_HR, 50, 100), ERR_DUPLICATE, "A4 pro id duplicado rechazado");
    CHECK_EQ(experience_register(&st, ROLE_HR, 9, 5000, 100, 20200101, 20201231), ERR_DUPLICATE, "A4 exp_id duplicado rechazado");

    /* A5: fechas extremas / invertidas */
    CHECK_EQ(experience_register(&st, ROLE_HR, 9, 6000, 100, DATE_MAX, DATE_MIN), ERR_BAD_DATE, "A5 fechas invertidas");

    /* A6: uso de credencial revocada */
    credential_issue(&st, ROLE_COORD, 60, 7000, 100);
    credential_activate(&st, ROLE_COORD, 60, 7000);
    credential_revoke(&st, ROLE_ADMIN, 50, 7000);
    CHECK_EQ(credential_is_valid(&st, 7000), 0, "A6 revocada no es valida");

    /* A7: transicion ilegal (revivir revocada) */
    CHECK_EQ(credential_reactivate(&st, ROLE_ADMIN, 50, 7000), ERR_BAD_TRANSITION, "A7 no se revive revocada");

    /* A8: parametros nulos no corrompen */
    CHECK_EQ(professional_register(0, ROLE_HR, 50, 1), ERR_NULL, "A8 null manejado");
    CHECK_EQ(audit_record_event(0, 1, 1, 1, 0), ERR_NULL, "A8 audit null manejado");

    /* A9: valores extremos de id (new_id y caller_id) */
    CHECK_EQ(professional_register(&st, ROLE_HR, 50, -5), ERR_BAD_ID, "A9 id negativo");
    CHECK_EQ(professional_register(&st, ROLE_HR, 50, ID_MAX + 1), ERR_BAD_ID, "A9 id excesivo");
    CHECK_EQ(professional_register(&st, ROLE_HR, 0, 300), ERR_BAD_ID, "A9 caller_id 0 invalido");

    /* A10: atomicidad en error - un error no cambia contadores */
    before_count = st.pros_count;
    professional_register(&st, ROLE_PRO, 50, 300);   /* ERR_PERM */
    CHECK_EQ(st.pros_count, before_count, "A10 error no muta pros_count");

    /* A11: historial append-only / monotono */
    {
        int ok_seq = 1, prev = 0;
        for (i = 0; i < st.log_count; i++) {
            if (st.log[i].seq <= prev) { ok_seq = 0; }
            prev = st.log[i].seq;
        }
        CHECK(ok_seq, "A11 seq estrictamente creciente");
    }

    /* A12: log lleno -> operaciones dependientes del log fallan limpio */
    {
        educhain_state_t full;
        memset(&full, 0, sizeof(full));
        professional_register(&full, ROLE_HR, 50, 100);
        full.log_count = MAX_EVENTS;  /* simular log lleno */
        CHECK_EQ(professional_register(&full, ROLE_HR, 50, 101), ERR_FULL, "A12 registro con log lleno -> ERR_FULL");
        CHECK_EQ(full.pros_count, 1, "A12 no se agrego profesional con log lleno");
    }

    /* ---- Brechas adicionales (sugeridas por revisor-seguridad) ---- */

    /* A13: transiciones ilegales de estado */
    {
        educhain_state_t s;
        memset(&s, 0, sizeof(s));
        professional_register(&s, ROLE_HR, 50, 100);
        credential_issue(&s, ROLE_COORD, 60, 7000, 100);           /* PENDING */
        CHECK_EQ(credential_block(&s, ROLE_ADMIN, 50, 7000), ERR_BAD_TRANSITION, "A13 PENDING->BLOCKED ilegal");
        CHECK_EQ(credential_reactivate(&s, ROLE_ADMIN, 50, 7000), ERR_BAD_TRANSITION, "A13 PENDING->ACTIVE(react) ilegal");
        credential_activate(&s, ROLE_COORD, 60, 7000);             /* ACTIVE */
        CHECK_EQ(credential_activate(&s, ROLE_COORD, 60, 7000), ERR_BAD_TRANSITION, "A13 ACTIVE->ACTIVE ilegal");
        CHECK_EQ(credential_reactivate(&s, ROLE_ADMIN, 50, 7000), ERR_BAD_TRANSITION, "A13 ACTIVE->reactivate ilegal");
    }

    /* A14: autorizacion de transiciones por rol */
    {
        educhain_state_t s;
        memset(&s, 0, sizeof(s));
        professional_register(&s, ROLE_HR, 50, 100);
        credential_issue(&s, ROLE_COORD, 60, 7000, 100);
        credential_activate(&s, ROLE_COORD, 60, 7000);
        CHECK_EQ(credential_block(&s, ROLE_PRO, 50, 7000), ERR_PERM, "A14 PRO no bloquea");
        CHECK_EQ(credential_block(&s, ROLE_VERIFIER, 50, 7000), ERR_PERM, "A14 VERIFIER no bloquea");
        CHECK_EQ(credential_revoke(&s, ROLE_HR, 50, 7000), ERR_PERM, "A14 HR no revoca");
    }

    /* A15: PENDING->REVOKED es transicion PERMITIDA (alineado con tabla y codigo) */
    {
        educhain_state_t s;
        memset(&s, 0, sizeof(s));
        professional_register(&s, ROLE_HR, 50, 100);
        credential_issue(&s, ROLE_COORD, 60, 7000, 100);           /* PENDING */
        CHECK_EQ(credential_revoke(&s, ROLE_ADMIN, 50, 7000), EDU_OK, "A15 PENDING->REVOKED permitido");
        CHECK_EQ(credential_status(&s, 7000), CRED_REVOKED, "A15 queda REVOKED");
    }

    /* A16: duplicado de cred_id y segunda credencial por owner */
    {
        educhain_state_t s;
        memset(&s, 0, sizeof(s));
        professional_register(&s, ROLE_HR, 50, 100);
        professional_register(&s, ROLE_HR, 50, 101);
        credential_issue(&s, ROLE_COORD, 60, 7000, 100);
        CHECK_EQ(credential_issue(&s, ROLE_COORD, 60, 7000, 101), ERR_DUPLICATE, "A16 cred_id duplicado");
        CHECK_EQ(credential_issue(&s, ROLE_COORD, 60, 7001, 100), ERR_DUPLICATE, "A16 segunda credencial por owner");
    }

    /* A17: estados no-ACTIVE no son validos */
    {
        educhain_state_t s;
        memset(&s, 0, sizeof(s));
        professional_register(&s, ROLE_HR, 50, 100);
        credential_issue(&s, ROLE_COORD, 60, 7000, 100);           /* PENDING */
        CHECK_EQ(credential_is_valid(&s, 7000), 0, "A17 PENDING no valida");
        credential_activate(&s, ROLE_COORD, 60, 7000);
        credential_block(&s, ROLE_COORD, 60, 7000);                /* BLOCKED */
        CHECK_EQ(credential_is_valid(&s, 7000), 0, "A17 BLOCKED no valida");
    }

    /* A18: arreglos llenos (pros, exps, creds), no solo log */
    {
        educhain_state_t s;
        int k, r;
        memset(&s, 0, sizeof(s));
        for (k = 0; k < MAX_PROFESSIONALS; k++) {
            r = professional_register(&s, ROLE_HR, 50, 1000 + k);
            if (r != EDU_OK) { break; }
        }
        CHECK_EQ(s.pros_count, MAX_PROFESSIONALS, "A18 pros lleno exacto");
        CHECK_EQ(professional_register(&s, ROLE_HR, 50, 999999), ERR_FULL, "A18 pros lleno -> ERR_FULL");
    }
}
