/* EduChain - main.c  (Fase 3)
 * Demo minima NO verificada. Muestra el flujo feliz y algunos rechazos.
 * No forma parte del alcance de WP/Eva.
 */
#include <stdio.h>
#include <string.h>
#include "professionals.h"
#include "experience.h"
#include "credentials.h"
#include "audit.h"

int main(void)
{
    educhain_state_t st;
    int rc;
    memset(&st, 0, sizeof(st));

    printf("== EduChain demo (simulacion academica, NO blockchain real) ==\n");

    /* caller_id 50 = administrador ejecutor (no es profesional registrado). */
    rc = professional_register(&st, ROLE_HR, 50, 1001);
    printf("register pro 1001 (HR, caller 50): %d\n", rc);

    rc = professional_register(&st, ROLE_PRO, 50, 1002);
    printf("register pro 1002 (PRO, debe fallar ERR_PERM=-2): %d\n", rc);

    rc = professional_register(&st, ROLE_HR, 50, 1002);
    printf("register pro 1002 (HR, caller 50): %d\n", rc);

    /* PRO registra su propia experiencia */
    rc = experience_register(&st, ROLE_PRO, 1001, 5001, 1001, 20200101, 20221231);
    printf("register exp 5001 owner 1001 (PRO propia): %d\n", rc);

    /* fechas invertidas -> ERR_BAD_DATE=-6 */
    rc = experience_register(&st, ROLE_HR, 9, 5002, 1002, 20221231, 20200101);
    printf("register exp fechas invertidas (debe -6): %d\n", rc);

    /* dueno intenta validar su propia experiencia (como HR) -> -3 */
    rc = experience_validate(&st, ROLE_HR, 1001, 5001);
    printf("self-validate exp 5001 (debe -3): %d\n", rc);

    /* otro actor valida */
    rc = experience_validate(&st, ROLE_COORD, 7, 5001);
    printf("validate exp 5001 (COORD): %d\n", rc);

    /* re-validar -> -11 */
    rc = experience_validate(&st, ROLE_COORD, 7, 5001);
    printf("re-validate exp 5001 (debe -11): %d\n", rc);

    /* credencial. caller_id: 60 = coordinador, 50 = admin. */
    rc = credential_issue(&st, ROLE_COORD, 60, 7001, 1001);
    printf("issue cred 7001 owner 1001 (COORD, caller 60): %d\n", rc);
    rc = credential_activate(&st, ROLE_COORD, 60, 7001);
    printf("activate cred 7001: %d (is_valid=%d)\n", rc, credential_is_valid(&st, 7001));
    rc = credential_block(&st, ROLE_ADMIN, 50, 7001);
    printf("block cred 7001 (ADMIN, caller 50): %d (is_valid=%d)\n", rc, credential_is_valid(&st, 7001));
    rc = credential_revoke(&st, ROLE_ADMIN, 50, 7001);
    printf("revoke cred 7001: %d (is_valid=%d)\n", rc, credential_is_valid(&st, 7001));
    rc = credential_reactivate(&st, ROLE_ADMIN, 50, 7001);
    printf("reactivate REVOKED cred 7001 (debe -9): %d\n", rc);

    printf("total eventos auditados: %d\n", audit_get_count(&st));
    printf("experiencias validadas owner 1001: %d\n", experience_validated_count(&st, 1001));
    return 0;
}
