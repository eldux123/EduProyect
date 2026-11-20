/* EduChain - test_experience.c */
#include <string.h>
#include "test_framework.h"
#include "professionals.h"
#include "experience.h"

void test_experience(void)
{
    educhain_state_t st;
    int idx;
    printf("[test_experience]\n");

    memset(&st, 0, sizeof(st));
    professional_register(&st, ROLE_HR, 50, 100);  /* owner */
    professional_register(&st, ROLE_HR, 50, 200);  /* validador */

    /* fechas */
    CHECK_EQ(experience_register(&st, ROLE_HR, 9, 5000, 100, DATE_MIN - 1, 20200101),
             ERR_BAD_DATE, "start fuera de rango");
    CHECK_EQ(experience_register(&st, ROLE_HR, 9, 5000, 100, 20221231, 20200101),
             ERR_BAD_DATE, "fechas invertidas");

    /* PRO ajeno */
    CHECK_EQ(experience_register(&st, ROLE_PRO, 999, 5000, 100, 20200101, 20201231),
             ERR_PERM, "PRO no registra experiencia ajena");

    /* owner inexistente */
    CHECK_EQ(experience_register(&st, ROLE_HR, 9, 5000, 300, 20200101, 20201231),
             ERR_NOT_FOUND, "owner inexistente");

    /* feliz */
    CHECK_EQ(experience_register(&st, ROLE_HR, 9, 5000, 100, 20200101, 20201231),
             EDU_OK, "registro valido");
    CHECK_EQ(experience_register(&st, ROLE_HR, 9, 5000, 100, 20200101, 20201231),
             ERR_DUPLICATE, "exp_id duplicado");

    /* autovalidacion: owner == caller (como HR) */
    CHECK_EQ(experience_validate(&st, ROLE_HR, 100, 5000),
             ERR_SELF_VALIDATE, "autovalidacion rechazada");

    /* rol sin permiso */
    CHECK_EQ(experience_validate(&st, ROLE_PRO, 200, 5000),
             ERR_PERM, "PRO no valida");
    CHECK_EQ(experience_validate(&st, ROLE_VERIFIER, 200, 5000),
             ERR_PERM, "VERIFIER no valida");

    /* validacion correcta por otro actor */
    CHECK_EQ(experience_validate(&st, ROLE_COORD, 200, 5000),
             EDU_OK, "validacion valida");

    /* inmutabilidad: fechas y owner intactos, no re-validable */
    idx = experience_find(&st, 5000);
    CHECK(idx >= 0, "exp encontrada");
    CHECK_EQ(st.exps[idx].start_date, 20200101, "start intacto tras validar");
    CHECK_EQ(st.exps[idx].end_date, 20201231, "end intacto tras validar");
    CHECK_EQ(st.exps[idx].owner_id, 100, "owner intacto tras validar");
    CHECK_EQ(st.exps[idx].validator_id, 200, "validator registrado");
    CHECK_EQ(experience_validate(&st, ROLE_COORD, 200, 5000),
             ERR_ALREADY_VALID, "re-validacion rechazada");

    /* experiencia inexistente */
    CHECK_EQ(experience_validate(&st, ROLE_COORD, 200, 8888),
             ERR_NOT_FOUND, "validar exp inexistente");

    /* experience_validated_count (reemplazo de experience_total_days) */
    CHECK_EQ(experience_validated_count(0, 100), ERR_NULL, "count: null -> ERR_NULL");
    CHECK_EQ(experience_validated_count(&st, 0), ERR_BAD_ID, "count: id invalido -> ERR_BAD_ID");
    CHECK_EQ(experience_validated_count(&st, 300), ERR_NOT_FOUND, "count: owner inexistente -> ERR_NOT_FOUND");
    CHECK_EQ(experience_validated_count(&st, 100), 1, "count: owner 100 tiene 1 validada");
    CHECK_EQ(experience_validated_count(&st, 200), 0, "count: owner 200 sin validadas -> 0");
}
