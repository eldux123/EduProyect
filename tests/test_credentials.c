/* EduChain - test_credentials.c */
#include <string.h>
#include "test_framework.h"
#include "professionals.h"
#include "credentials.h"

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
}
