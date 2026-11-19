/* EduChain - test_credentials.c */
#include <string.h>
#include "test_framework.h"
#include "professionals.h"
#include "credentials.h"
#include "requests.h"

/* Siembra una solicitud en un estado y categoria dados dentro de rs. */
static void seed_req(requests_state_t *rs, int req_id, int owner_id,
                     int category, request_state_t state)
{
    int n = rs->reqs_count;
    memset(&rs->reqs[n], 0, sizeof(rs->reqs[n]));
    rs->reqs[n].id = req_id;
    rs->reqs[n].owner_id = owner_id;
    rs->reqs[n].category = (request_category_t)category;
    rs->reqs[n].state = state;
    rs->reqs_count = n + 1;
}

void test_credentials(void)
{
    educhain_state_t st;
    printf("[test_credentials]\n");

    memset(&st, 0, sizeof(st));
    professional_register(&st, ROLE_HR, 50, 100);

    /* caller_id: 60 = coordinador ejecutor, 50 = admin ejecutor. */

    /* permisos de emision */
    CHECK_EQ(credential_issue(&st, ROLE_ADMIN, 50, 7000, 100), ERR_PERM, "ADMIN no emite");
    CHECK_EQ(credential_issue(&st, ROLE_HR, 50, 7000, 100), ERR_PERM, "HR no emite");
    CHECK_EQ(credential_issue(&st, ROLE_COORD, 60, 7000, 300), ERR_NOT_FOUND, "owner inexistente");
    CHECK_EQ(credential_issue(&st, ROLE_COORD, 0, 7000, 100), ERR_BAD_ID, "caller_id invalido");
    CHECK_EQ(credential_issue(&st, ROLE_COORD, 60, 7000, 100), EDU_OK, "COORD emite PENDING");
    CHECK_EQ(credential_status(&st, 7000), CRED_PENDING, "estado PENDING");
    CHECK_EQ(credential_is_valid(&st, 7000), 0, "PENDING no es valida");

    /* auditoria: actor_id = caller real (60), no el owner (100) */
    CHECK_EQ(st.log[st.log_count - 1].actor_id, 60, "issue audita caller_id real (60)");
    CHECK_EQ(st.log[st.log_count - 1].target_id, 7000, "issue audita target_id = cred_id");

    /* una por profesional */
    CHECK_EQ(credential_issue(&st, ROLE_COORD, 60, 7001, 100), ERR_DUPLICATE, "una credencial por profesional");

    /* transiciones invalidas desde PENDING */
    CHECK_EQ(credential_block(&st, ROLE_ADMIN, 50, 7000), ERR_BAD_TRANSITION, "PENDING->BLOCKED ilegal");
    CHECK_EQ(credential_reactivate(&st, ROLE_ADMIN, 50, 7000), ERR_BAD_TRANSITION, "PENDING->(react) ilegal");

    /* PENDING -> ACTIVE */
    CHECK_EQ(credential_activate(&st, ROLE_COORD, 60, 7000), EDU_OK, "activate");
    CHECK_EQ(credential_is_valid(&st, 7000), 1, "ACTIVE es valida");
    CHECK_EQ(credential_activate(&st, ROLE_COORD, 60, 7000), ERR_BAD_TRANSITION, "ACTIVE->ACTIVE ilegal");

    /* ACTIVE -> BLOCKED -> ACTIVE */
    CHECK_EQ(credential_block(&st, ROLE_COORD, 60, 7000), EDU_OK, "block");
    CHECK_EQ(credential_is_valid(&st, 7000), 0, "BLOCKED no valida");
    CHECK_EQ(credential_reactivate(&st, ROLE_ADMIN, 50, 7000), EDU_OK, "reactivate");
    CHECK_EQ(credential_is_valid(&st, 7000), 1, "ACTIVE de nuevo");

    /* revoke terminal */
    CHECK_EQ(credential_revoke(&st, ROLE_ADMIN, 50, 7000), EDU_OK, "revoke");
    CHECK_EQ(credential_status(&st, 7000), CRED_REVOKED, "REVOKED");
    CHECK_EQ(credential_is_valid(&st, 7000), 0, "REVOKED no valida");
    CHECK_EQ(credential_reactivate(&st, ROLE_ADMIN, 50, 7000), ERR_BAD_TRANSITION, "REVOKED->ACTIVE ilegal");
    CHECK_EQ(credential_block(&st, ROLE_ADMIN, 50, 7000), ERR_BAD_TRANSITION, "REVOKED->BLOCKED ilegal");
    CHECK_EQ(credential_revoke(&st, ROLE_ADMIN, 50, 7000), ERR_BAD_TRANSITION, "REVOKED->REVOKED ilegal");

    /* permisos de bloqueo/revocacion */
    memset(&st, 0, sizeof(st));
    professional_register(&st, ROLE_HR, 50, 100);
    credential_issue(&st, ROLE_COORD, 60, 7000, 100);
    credential_activate(&st, ROLE_COORD, 60, 7000);
    CHECK_EQ(credential_block(&st, ROLE_HR, 50, 7000), ERR_PERM, "HR no bloquea");
    CHECK_EQ(credential_revoke(&st, ROLE_PRO, 50, 7000), ERR_PERM, "PRO no revoca");

    /* credencial inexistente */
    CHECK_EQ(credential_status(&st, 9999), -1, "status de inexistente = -1");
    CHECK_EQ(credential_activate(&st, ROLE_COORD, 60, 9999), ERR_NOT_FOUND, "activate inexistente");

    /* ============ credential_eligible (Rev 5 §8-ter, SOLO LECTURA) ============ */
    {
        requests_state_t rs;
        request_init(&rs);
        /* NULL defensivo. */
        CHECK_EQ(credential_eligible(0, 100), 0, "eligible: rs NULL -> 0");
        /* Sin registros: no elegible. */
        CHECK_EQ(credential_eligible(&rs, 100), 0, "eligible: sin registros -> 0");
        /* RECORDED vigente en CAT_INSTITUTION_WORKED -> elegible. */
        seed_req(&rs, 9001, 100, CAT_INSTITUTION_WORKED, REQ_RECORDED);
        CHECK_EQ(credential_eligible(&rs, 100), 1, "eligible: RECORDED INSTITUTION_WORKED -> 1");
        /* Otro owner sigue sin elegibilidad. */
        CHECK_EQ(credential_eligible(&rs, 200), 0, "eligible: otro owner -> 0");
        /* RECORDED en CAT_LINK_PERIOD tambien habilita. */
        seed_req(&rs, 9002, 200, CAT_LINK_PERIOD, REQ_RECORDED);
        CHECK_EQ(credential_eligible(&rs, 200), 1, "eligible: RECORDED LINK_PERIOD -> 1");
        /* RECORDED pero en categoria NO habilitante -> no cuenta. */
        request_init(&rs);
        seed_req(&rs, 9003, 300, CAT_PUBLICATION, REQ_RECORDED);
        CHECK_EQ(credential_eligible(&rs, 300), 0, "eligible: RECORDED cat no habilitante -> 0");
        /* Estados no vigentes en categoria habilitante -> no cuentan. */
        request_init(&rs);
        seed_req(&rs, 9010, 400, CAT_INSTITUTION_WORKED, REQ_PENDING);
        seed_req(&rs, 9011, 400, CAT_INSTITUTION_WORKED, REQ_ENDORSED);
        seed_req(&rs, 9012, 400, CAT_INSTITUTION_WORKED, REQ_VERIFIED);
        seed_req(&rs, 9013, 400, CAT_INSTITUTION_WORKED, REQ_REJECTED);
        CHECK_EQ(credential_eligible(&rs, 400), 0, "eligible: PENDING/ENDORSED/VERIFIED/REJECTED no cuentan");
        /* SUPERSEDED en categoria habilitante NO cuenta (clave del requisito). */
        request_init(&rs);
        seed_req(&rs, 9020, 500, CAT_INSTITUTION_WORKED, REQ_SUPERSEDED);
        CHECK_EQ(credential_eligible(&rs, 500), 0, "eligible: SUPERSEDED no cuenta");
        /* SUPERSEDED + un RECORDED vigente -> elegible por el RECORDED. */
        seed_req(&rs, 9021, 500, CAT_INSTITUTION_WORKED, REQ_RECORDED);
        CHECK_EQ(credential_eligible(&rs, 500), 1, "eligible: SUPERSEDED+RECORDED -> 1 por el RECORDED");
    }

    /* ============ credential_issue_eligible (via aditiva de emision) ============ */
    {
        educhain_state_t st2;
        requests_state_t rs2;
        memset(&st2, 0, sizeof(st2));
        request_init(&rs2);
        professional_register(&st2, ROLE_HR, 50, 100);

        /* NULL defensivo. */
        CHECK_EQ(credential_issue_eligible(0, &rs2, ROLE_COORD, 60, 7100, 100), ERR_NULL, "issue_elig: st NULL");
        CHECK_EQ(credential_issue_eligible(&st2, 0, ROLE_COORD, 60, 7100, 100), ERR_NULL, "issue_elig: rs NULL");

        /* Sin trayectoria vigente: NO elegible -> ERR_PERM, sin mutar. */
        CHECK_EQ(credential_issue_eligible(&st2, &rs2, ROLE_COORD, 60, 7100, 100), ERR_PERM, "issue_elig: sin trayectoria ERR_PERM");
        CHECK_EQ(st2.creds_count, 0, "issue_elig: no elegible no muta creds_count");
        CHECK_EQ(credential_status(&st2, 7100), -1, "issue_elig: no elegible no crea credencial");

        /* Con RECORDED vigente habilitante: emite. */
        seed_req(&rs2, 7200, 100, CAT_LINK_PERIOD, REQ_RECORDED);
        CHECK_EQ(credential_issue_eligible(&st2, &rs2, ROLE_COORD, 60, 7100, 100), EDU_OK, "issue_elig: con trayectoria emite OK");
        CHECK_EQ(credential_status(&st2, 7100), CRED_PENDING, "issue_elig: credencial PENDING emitida");
        CHECK_EQ(st2.creds_count, 1, "issue_elig: creds_count = 1");
        /* Hereda la validacion de credential_issue: rol no COORD -> ERR_PERM (aunque elegible). */
        CHECK_EQ(credential_issue_eligible(&st2, &rs2, ROLE_ADMIN, 50, 7101, 100), ERR_PERM, "issue_elig: rol no COORD ERR_PERM");
        /* Consistencia: SUPERSEDED no habilita aunque exista traza. */
        {
            educhain_state_t st3;
            requests_state_t rs3;
            memset(&st3, 0, sizeof(st3));
            request_init(&rs3);
            professional_register(&st3, ROLE_HR, 50, 600);
            seed_req(&rs3, 7300, 600, CAT_INSTITUTION_WORKED, REQ_SUPERSEDED);
            CHECK_EQ(credential_issue_eligible(&st3, &rs3, ROLE_COORD, 60, 7301, 600), ERR_PERM, "issue_elig: SUPERSEDED no habilita");
            CHECK_EQ(st3.creds_count, 0, "issue_elig: SUPERSEDED no emite");
        }
    }
}
