/* EduChain - test_professionals.c */
#include <string.h>
#include "test_framework.h"
#include "professionals.h"

void test_professionals(void)
{
    educhain_state_t st;
    int rc, i;
    printf("[test_professionals]\n");

    memset(&st, 0, sizeof(st));

    /* caller_id 50 = ejecutor (admin/HR), no necesita existir en professionals[]. */
    CHECK_EQ(professional_register(0, ROLE_HR, 50, 1), ERR_NULL, "null state rechazado");
    CHECK_EQ(professional_register(&st, ROLE_HR, 50, 0), ERR_BAD_ID, "id 0 invalido");
    CHECK_EQ(professional_register(&st, ROLE_HR, 50, ID_MAX + 1), ERR_BAD_ID, "id > ID_MAX invalido");
    CHECK_EQ(professional_register(&st, ROLE_HR, 0, 100), ERR_BAD_ID, "caller_id 0 invalido");
    CHECK_EQ(professional_register(&st, ROLE_COORD, 50, 100), ERR_PERM, "COORD no registra");
    CHECK_EQ(professional_register(&st, ROLE_PRO, 50, 100), ERR_PERM, "PRO no registra");

    CHECK_EQ(professional_register(&st, ROLE_HR, 50, 100), EDU_OK, "HR registra 100");
    CHECK_EQ(professional_register(&st, ROLE_ADMIN, 50, 101), EDU_OK, "ADMIN registra 101");
    CHECK_EQ(professional_register(&st, ROLE_HR, 50, 100), ERR_DUPLICATE, "duplicado rechazado");
    CHECK_EQ(professional_is_active(&st, 100), 1, "100 activo");
    CHECK_EQ(professional_is_active(&st, 999), 0, "999 no existe -> inactivo");

    /* El actor real (50) queda en el log, no el new_id. */
    CHECK_EQ(st.log[0].actor_id, 50, "auditoria registra caller_id real");
    CHECK_EQ(st.log[0].target_id, 100, "auditoria registra target_id = new_id");

    /* Llenar el arreglo y comprobar ERR_FULL */
    memset(&st, 0, sizeof(st));
    for (i = 0; i < MAX_PROFESSIONALS; i++) {
        rc = professional_register(&st, ROLE_HR, 50, 1000 + i);
        if (rc != EDU_OK) { break; }
    }
    CHECK_EQ(st.pros_count, MAX_PROFESSIONALS, "arreglo lleno exacto");
    CHECK_EQ(professional_register(&st, ROLE_HR, 50, 999999), ERR_FULL, "registro en arreglo lleno -> ERR_FULL");
}
