/* EduChain - test_requests.c
 * Pruebas del flujo de solicitudes (ETAPA 2, PRIMER ENTREGABLE).
 * Cubre: request_init, request_find, request_status, request_requires_endorsement,
 * request_submit, request_submit_correction, request_endorse, request_verify,
 * request_reject. Respaldo dinamico de las propiedades de los contratos ACSL.
 */
#include <string.h>
#include "test_framework.h"
#include "requests.h"

/* Siembra un vinculo de representacion ACTIVO user_id -> institution_id en auth[].
 * (La gestion publica de auth[] -authz_grant/revoke- es de un entregable
 * posterior; aqui se construye el estado directamente para probar la lectura.) */
static void seed_authz(requests_state_t *rs, int user_id, int institution_id)
{
    int n = rs->auth_count;
    rs->auth[n].user_id = user_id;
    rs->auth[n].institution_id = institution_id;
    rs->auth[n].active = 1;
    rs->auth_count = n + 1;
}

/* Siembra una solicitud RECORDED directamente (no hay operacion publica que
 * llegue a RECORDED en este entregable). Devuelve el indice. */
static int seed_recorded(requests_state_t *rs, int req_id, int owner_id,
                         int institution_id, int category)
{
    int n = rs->reqs_count;
    memset(&rs->reqs[n], 0, sizeof(rs->reqs[n]));
    rs->reqs[n].id = req_id;
    rs->reqs[n].owner_id = owner_id;
    rs->reqs[n].institution_id = institution_id;
    rs->reqs[n].category = (request_category_t)category;
    rs->reqs[n].state = REQ_RECORDED;
    rs->reqs[n].start_date = 20200101;
    rs->reqs[n].end_date = 20201231;
    rs->reqs_count = n + 1;
    return n;
}

void test_requests(void)
{
    educhain_state_t st;
    requests_state_t rs;
    printf("[test_requests]\n");

    /* ---- request_init + consultas sobre estado vacio ---- */
    memset(&rs, 0, sizeof(rs));
    rs.reqs_count = 999; rs.auth_count = 777;   /* basura previa */
    request_init(&rs);
    CHECK_EQ(rs.reqs_count, 0, "init pone reqs_count=0");
    CHECK_EQ(rs.auth_count, 0, "init pone auth_count=0");
    CHECK_EQ(request_find(&rs, 5000), -1, "find en estado vacio = -1");
    CHECK_EQ(request_status(&rs, 5000), -1, "status en estado vacio = -1");

    /* ---- request_requires_endorsement de las 14 categorias (§5.3) ---- */
    CHECK_EQ(request_requires_endorsement(CAT_INSTITUTION_WORKED), 1, "INSTITUTION_WORKED exige");
    CHECK_EQ(request_requires_endorsement(CAT_LINK_PERIOD), 1, "LINK_PERIOD exige");
    CHECK_EQ(request_requires_endorsement(CAT_LINK_TYPE_DEDICATION), 1, "LINK_TYPE exige");
    CHECK_EQ(request_requires_endorsement(CAT_COURSES_TAUGHT), 1, "COURSES exige");
    CHECK_EQ(request_requires_endorsement(CAT_ACADEMIC_PERIOD), 1, "ACADEMIC_PERIOD exige");
    CHECK_EQ(request_requires_endorsement(CAT_TEACHING_HOURS), 1, "TEACHING_HOURS exige");
    CHECK_EQ(request_requires_endorsement(CAT_ACADEMIC_ACTIVITY), 1, "ACADEMIC_ACTIVITY exige");
    CHECK_EQ(request_requires_endorsement(CAT_THESIS_SUPERVISION), 1, "THESIS exige");
    CHECK_EQ(request_requires_endorsement(CAT_OUTREACH), 1, "OUTREACH exige");
    CHECK_EQ(request_requires_endorsement(CAT_COMMITTEES), 1, "COMMITTEES exige");
    CHECK_EQ(request_requires_endorsement(CAT_RESEARCH_PROJECT), 0, "RESEARCH base no exige");
    CHECK_EQ(request_requires_endorsement(CAT_PUBLICATION), 0, "PUBLICATION no exige");
    CHECK_EQ(request_requires_endorsement(CAT_TRAINING), 0, "TRAINING no exige");
    CHECK_EQ(request_requires_endorsement(CAT_CERTIFICATION), 0, "CERTIFICATION no exige");

    /* ================= request_submit ================= */
    memset(&st, 0, sizeof(st));
    request_init(&rs);

    /* PROFESSOR presenta lo propio, categoria que exige respaldo -> PENDING. */
    CHECK_EQ(request_submit(&st, &rs, ROLE_PRO, 100, 5000, 100, 300,
                            CAT_TEACHING_HOURS, 20200101, 20201231, 42,
                            0, 0, 0, 0), EDU_OK, "PROFESSOR submit propio OK");
    CHECK_EQ(request_status(&rs, 5000), REQ_PENDING, "PROFESSOR+respaldo nace PENDING");
    {
        int ix = request_find(&rs, 5000);
        CHECK_EQ(rs.reqs[ix].endorser_id, 0, "PROFESSOR nace endorser_id=0");
        CHECK_EQ(rs.reqs[ix].submitter_id, 100, "submitter_id = caller_id");
        CHECK_EQ(rs.reqs[ix].prev_id, 0, "original prev_id=0");
    }
    /* auditoria: actor_id = caller real (100), target = req_id. */
    CHECK_EQ(st.log[st.log_count - 1].actor_id, 100, "submit audita caller_id (100)");
    CHECK_EQ(st.log[st.log_count - 1].target_id, 5000, "submit audita target=req_id");
    CHECK_EQ(st.log[st.log_count - 1].op_code, OP_SUBMIT_REQUEST, "submit op_code");

    /* PROFESSOR intenta presentar historial ajeno -> ERR_PERM. */
    CHECK_EQ(request_submit(&st, &rs, ROLE_PRO, 100, 5001, 200, 0,
                            CAT_PUBLICATION, 20200101, 20201231, 0,
                            0, 0, 0, 0), ERR_PERM, "PROFESSOR ajeno ERR_PERM");

    /* PROFESSOR, categoria sin respaldo -> PENDING. */
    CHECK_EQ(request_submit(&st, &rs, ROLE_PRO, 101, 5002, 101, 0,
                            CAT_PUBLICATION, 20200101, 20201231, 0,
                            0, 0, 0, 0), EDU_OK, "PROFESSOR submit sin respaldo OK");
    CHECK_EQ(request_status(&rs, 5002), REQ_PENDING, "sin respaldo nace PENDING");

    /* INSTITUTION sin vinculo de representacion -> ERR_PERM. */
    CHECK_EQ(request_submit(&st, &rs, ROLE_INSTITUTION, 7001, 5003, 102, 300,
                            CAT_INSTITUTION_WORKED, 20200101, 20201231, 0,
                            0, 0, 0, 0), ERR_PERM, "INSTITUTION sin authz ERR_PERM");

    /* INSTITUTION con vinculo activo + categoria que exige respaldo -> ENDORSED. */
    seed_authz(&rs, 7001, 300);
    CHECK_EQ(request_submit(&st, &rs, ROLE_INSTITUTION, 7001, 5003, 102, 300,
                            CAT_INSTITUTION_WORKED, 20200101, 20201231, 0,
                            0, 0, 0, 0), EDU_OK, "INSTITUTION submit autorizada OK");
    CHECK_EQ(request_status(&rs, 5003), REQ_ENDORSED, "INSTITUTION+respaldo nace ENDORSED");
    {
        int ix = request_find(&rs, 5003);
        CHECK_EQ(rs.reqs[ix].endorser_id, 300, "INSTITUTION fija endorser_id=institution_id");
    }

    /* Roles prohibidos en SUBMIT. */
    CHECK_EQ(request_submit(&st, &rs, ROLE_ADMIN, 50, 5010, 100, 0,
                            CAT_PUBLICATION, 20200101, 20201231, 0,
                            0, 0, 0, 0), ERR_PERM, "ADMIN no presenta");
    CHECK_EQ(request_submit(&st, &rs, ROLE_HR, 50, 5010, 100, 0,
                            CAT_PUBLICATION, 20200101, 20201231, 0,
                            0, 0, 0, 0), ERR_PERM, "HR no presenta");
    CHECK_EQ(request_submit(&st, &rs, ROLE_COORD, 60, 5010, 100, 0,
                            CAT_PUBLICATION, 20200101, 20201231, 0,
                            0, 0, 0, 0), ERR_PERM, "COORD no presenta");
    CHECK_EQ(request_submit(&st, &rs, ROLE_VERIFIER, 80, 5010, 100, 0,
                            CAT_PUBLICATION, 20200101, 20201231, 0,
                            0, 0, 0, 0), ERR_PERM, "VERIFIER no presenta");

    /* Duplicado, fecha mala, id invalido, categoria invalida. */
    CHECK_EQ(request_submit(&st, &rs, ROLE_PRO, 100, 5000, 100, 300,
                            CAT_TEACHING_HOURS, 20200101, 20201231, 0,
                            0, 0, 0, 0), ERR_DUPLICATE, "submit duplicado ERR_DUPLICATE");
    CHECK_EQ(request_submit(&st, &rs, ROLE_PRO, 101, 5020, 101, 0,
                            CAT_PUBLICATION, 20201231, 20200101, 0,
                            0, 0, 0, 0), ERR_BAD_DATE, "start>end ERR_BAD_DATE");
    CHECK_EQ(request_submit(&st, &rs, ROLE_PRO, 101, 5020, 101, 0,
                            CAT_PUBLICATION, 18000101, 20200101, 0,
                            0, 0, 0, 0), ERR_BAD_DATE, "fecha fuera de rango ERR_BAD_DATE");
    CHECK_EQ(request_submit(&st, &rs, ROLE_PRO, 0, 5020, 0, 0,
                            CAT_PUBLICATION, 20200101, 20200101, 0,
                            0, 0, 0, 0), ERR_BAD_ID, "caller_id invalido ERR_BAD_ID");
    CHECK_EQ(request_submit(&st, &rs, ROLE_PRO, 101, 5020, 101, 0,
                            99, 20200101, 20200101, 0,
                            0, 0, 0, 0), ERR_BAD_ID, "categoria invalida ERR_BAD_ID");
    /* NULL defensivo. */
    CHECK_EQ(request_submit(0, &rs, ROLE_PRO, 101, 5020, 101, 0,
                            CAT_PUBLICATION, 20200101, 20200101, 0,
                            0, 0, 0, 0), ERR_NULL, "st NULL ERR_NULL");

    /* RESEARCH_PROJECT condicional: con institucion -> exige (INSTITUTION->ENDORSED). */
    CHECK_EQ(request_submit(&st, &rs, ROLE_INSTITUTION, 7001, 5030, 103, 300,
                            CAT_RESEARCH_PROJECT, 20200101, 20201231, 0,
                            0, 0, 0, 0), EDU_OK, "RESEARCH institucional INSTITUTION OK");
    CHECK_EQ(request_status(&rs, 5030), REQ_ENDORSED, "RESEARCH con institucion -> ENDORSED");
    /* RESEARCH_PROJECT externo (institution_id=0) PROFESSOR -> PENDING, sin respaldo. */
    CHECK_EQ(request_submit(&st, &rs, ROLE_PRO, 104, 5031, 104, 0,
                            CAT_RESEARCH_PROJECT, 20200101, 20201231, 0,
                            0, 0, 0, 0), EDU_OK, "RESEARCH externo PROFESSOR OK");
    CHECK_EQ(request_status(&rs, 5031), REQ_PENDING, "RESEARCH sin institucion -> PENDING");

    /* ================= request_endorse ================= */
    /* 5000 es PENDING del PROFESSOR, cat TEACHING_HOURS, institution_id=300. */
    /* Rol no INSTITUTION -> ERR_PERM. */
    CHECK_EQ(request_endorse(&st, &rs, ROLE_VERIFIER, 80, 5000), ERR_PERM, "endorse rol!=INSTITUTION ERR_PERM");
    /* INSTITUTION sin authz para institution_id -> ERR_PERM. */
    CHECK_EQ(request_endorse(&st, &rs, ROLE_INSTITUTION, 9999, 5000), ERR_PERM, "endorse sin authz ERR_PERM");
    /* INSTITUTION autorizada -> ENDORSED. */
    CHECK_EQ(request_endorse(&st, &rs, ROLE_INSTITUTION, 7001, 5000), EDU_OK, "endorse autorizada OK");
    CHECK_EQ(request_status(&rs, 5000), REQ_ENDORSED, "endorse PENDING->ENDORSED");
    {
        int ix = request_find(&rs, 5000);
        CHECK_EQ(rs.reqs[ix].endorser_id, 300, "endorse fija endorser_id");
    }
    /* Re-endosar (ya ENDORSED) -> ERR_BAD_TRANSITION. */
    CHECK_EQ(request_endorse(&st, &rs, ROLE_INSTITUTION, 7001, 5000), ERR_BAD_TRANSITION, "endorse no-PENDING ERR_BAD_TRANSITION");
    /* Endosar inexistente -> ERR_NOT_FOUND. */
    CHECK_EQ(request_endorse(&st, &rs, ROLE_INSTITUTION, 7001, 8888), ERR_NOT_FOUND, "endorse inexistente ERR_NOT_FOUND");
    /* Endosar una categoria sin respaldo (5002 PUBLICATION PENDING) -> ERR_BAD_TRANSITION. */
    CHECK_EQ(request_endorse(&st, &rs, ROLE_INSTITUTION, 7001, 5002), ERR_BAD_TRANSITION, "endorse cat sin respaldo ERR_BAD_TRANSITION");

    /* ================= request_verify / request_reject ================= */
    /* 5000 ahora ENDORSED, submitter=100. VERIFIER 80 verifica -> VERIFIED. */
    CHECK_EQ(request_verify(&st, &rs, ROLE_PRO, 80, 5000), ERR_PERM, "verify rol!=VERIFIER ERR_PERM");
    CHECK_EQ(request_verify(&st, &rs, ROLE_VERIFIER, 80, 5000), EDU_OK, "verify ENDORSED OK");
    CHECK_EQ(request_status(&rs, 5000), REQ_VERIFIED, "verify ENDORSED->VERIFIED");
    {
        int ix = request_find(&rs, 5000);
        CHECK_EQ(rs.reqs[ix].verifier_id, 80, "verify fija verifier_id");
    }

    /* No autovalidacion: submitter (100) intenta verificar lo propio.
     * Se usa 5031 (PENDING, sin respaldo, submitter=104). */
    CHECK_EQ(request_verify(&st, &rs, ROLE_VERIFIER, 104, 5031), ERR_SELF_VALIDATE, "verify submitter==caller ERR_SELF_VALIDATE");
    /* VERIFIER distinto verifica 5031 (PENDING sin respaldo) directo -> VERIFIED. */
    CHECK_EQ(request_verify(&st, &rs, ROLE_VERIFIER, 80, 5031), EDU_OK, "verify PENDING sin respaldo OK");
    CHECK_EQ(request_status(&rs, 5031), REQ_VERIFIED, "PENDING sin respaldo -> VERIFIED");

    /* Exige respaldo pero aun PENDING: sembramos una PENDING del PROFESSOR con respaldo. */
    CHECK_EQ(request_submit(&st, &rs, ROLE_PRO, 105, 5040, 105, 300,
                            CAT_COURSES_TAUGHT, 20200101, 20201231, 0,
                            0, 0, 0, 0), EDU_OK, "submit PROFESSOR con respaldo OK");
    CHECK_EQ(request_verify(&st, &rs, ROLE_VERIFIER, 80, 5040), ERR_BAD_TRANSITION, "verify exige respaldo pero PENDING ERR_BAD_TRANSITION");

    /* reject de PENDING -> REJECTED. */
    CHECK_EQ(request_reject(&st, &rs, ROLE_VERIFIER, 80, 5040), EDU_OK, "reject PENDING OK");
    CHECK_EQ(request_status(&rs, 5040), REQ_REJECTED, "reject -> REJECTED");
    /* reject de estado terminal (VERIFIED) -> ERR_BAD_TRANSITION. */
    CHECK_EQ(request_reject(&st, &rs, ROLE_VERIFIER, 80, 5000), ERR_BAD_TRANSITION, "reject de VERIFIED ERR_BAD_TRANSITION");
    /* reject rol prohibido -> ERR_PERM. */
    CHECK_EQ(request_reject(&st, &rs, ROLE_ADMIN, 50, 5002), ERR_PERM, "reject rol!=VERIFIER ERR_PERM");

    /* ================= request_submit_correction ================= */
    memset(&st, 0, sizeof(st));
    request_init(&rs);
    seed_authz(&rs, 7001, 300);
    /* Registro vigente sembrado: 6000 RECORDED, owner 100, inst 300, TEACHING_HOURS. */
    seed_recorded(&rs, 6000, 100, 300, CAT_TEACHING_HOURS);

    /* Correccion valida por PROFESSOR titular: misma cat, misma institucion. */
    CHECK_EQ(request_submit_correction(&st, &rs, ROLE_PRO, 100, 6001, 100, 300,
                                       CAT_TEACHING_HOURS, 20200101, 20201231, 7, 6000,
                                       0, 0, 0, 0), EDU_OK, "correccion valida OK");
    CHECK_EQ(request_status(&rs, 6001), REQ_PENDING, "correccion PROFESSOR con respaldo PENDING");
    {
        int ix = request_find(&rs, 6001);
        CHECK_EQ(rs.reqs[ix].prev_id, 6000, "correccion guarda prev_id");
    }
    /* El registro anterior NO se toca (sigue RECORDED). */
    CHECK_EQ(request_status(&rs, 6000), REQ_RECORDED, "correccion no toca el anterior");

    /* prev_id inexistente -> ERR_NOT_FOUND. */
    CHECK_EQ(request_submit_correction(&st, &rs, ROLE_PRO, 100, 6002, 100, 300,
                                       CAT_TEACHING_HOURS, 20200101, 20201231, 0, 9999,
                                       0, 0, 0, 0), ERR_NOT_FOUND, "prev inexistente ERR_NOT_FOUND");
    /* anterior no RECORDED (6001 esta PENDING) -> ERR_BAD_TRANSITION. */
    CHECK_EQ(request_submit_correction(&st, &rs, ROLE_PRO, 100, 6003, 100, 300,
                                       CAT_TEACHING_HOURS, 20200101, 20201231, 0, 6001,
                                       0, 0, 0, 0), ERR_BAD_TRANSITION, "prev no RECORDED ERR_BAD_TRANSITION");
    /* owner distinto del anterior -> ERR_PERM (PROFESSOR 200 no es titular de 6000). */
    CHECK_EQ(request_submit_correction(&st, &rs, ROLE_PRO, 200, 6004, 200, 300,
                                       CAT_TEACHING_HOURS, 20200101, 20201231, 0, 6000,
                                       0, 0, 0, 0), ERR_PERM, "owner distinto ERR_PERM");
    /* categoria distinta del anterior -> ERR_BAD_TRANSITION. */
    CHECK_EQ(request_submit_correction(&st, &rs, ROLE_PRO, 100, 6005, 100, 300,
                                       CAT_COURSES_TAUGHT, 20200101, 20201231, 0, 6000,
                                       0, 0, 0, 0), ERR_BAD_TRANSITION, "categoria distinta ERR_BAD_TRANSITION");
    /* institucion cambiada -> ERR_BAD_TRANSITION. */
    seed_authz(&rs, 7001, 400);
    CHECK_EQ(request_submit_correction(&st, &rs, ROLE_PRO, 100, 6006, 100, 400,
                                       CAT_TEACHING_HOURS, 20200101, 20201231, 0, 6000,
                                       0, 0, 0, 0), ERR_BAD_TRANSITION, "institucion cambiada ERR_BAD_TRANSITION");

    /* Correccion de una categoria SIN institucion: anterior y nuevo con institution_id=0. */
    seed_recorded(&rs, 6100, 101, 0, CAT_PUBLICATION);
    CHECK_EQ(request_submit_correction(&st, &rs, ROLE_PRO, 101, 6101, 101, 0,
                                       CAT_PUBLICATION, 20200101, 20201231, 0, 6100,
                                       0, 0, 0, 0), EDU_OK, "correccion sin institucion OK");
    /* misma categoria sin institucion pero con institution_id!=0 -> incoherente T-1. */
    CHECK_EQ(request_submit_correction(&st, &rs, ROLE_PRO, 101, 6102, 101, 500,
                                       CAT_PUBLICATION, 20200101, 20201231, 0, 6100,
                                       0, 0, 0, 0), ERR_BAD_TRANSITION, "cat sin respaldo con institucion ERR_BAD_TRANSITION");
}
