/* EduChain - tools/probar_profesional.c
 * Pequena CLI interactiva para PROBAR EN VIVO el registro de profesionales.
 * NO es parte del nucleo verificado: es una herramienta de demostracion que
 * consume las funciones publicas (professional_register / professional_is_active).
 *
 * Compilar y correr:
 *   gcc -std=c99 -Wall -Wextra -Iinclude \
 *       src/security.c src/audit.c src/professionals.c tools/probar_profesional.c \
 *       -o /tmp/probar_pro && /tmp/probar_pro
 */
#include <stdio.h>
#include <string.h>
#include "professionals.h"

static const char *nombre_rol(int r) {
    switch (r) {
    case ROLE_ADMIN: return "ADMIN";
    case ROLE_HR: return "HR";
    case ROLE_COORD: return "COORD";
    case ROLE_PRO: return "PRO";
    case ROLE_VERIFIER: return "VERIFIER";
    default: return "DESCONOCIDO";
    }
}

static const char *nombre_error(int rc) {
    switch (rc) {
    case EDU_OK: return "EDU_OK (registrado)";
    case ERR_NULL: return "ERR_NULL (estado nulo)";
    case ERR_PERM: return "ERR_PERM (rol sin permiso)";
    case ERR_BAD_ID: return "ERR_BAD_ID (id fuera de rango)";
    case ERR_DUPLICATE: return "ERR_DUPLICATE (ya existe)";
    case ERR_FULL: return "ERR_FULL (arreglo lleno)";
    default: return "(codigo inesperado)";
    }
}

int main(void)
{
    educhain_state_t st;
    memset(&st, 0, sizeof(st));

    printf("=====================================================\n");
    printf(" EduChain - probar registro de profesional (en vivo)\n");
    printf("=====================================================\n");
    printf("Roles: 0=ADMIN 1=HR 2=COORD 3=PRO 4=VERIFIER\n");
    printf("Solo ADMIN(0) y HR(1) pueden registrar.\n");
    printf("ID valido: entre %d y %d.\n\n", ID_MIN, ID_MAX);

    for (;;) {
        int rol = -1, caller_id = 0, new_id = 0, rc;
        char linea[128];

        printf("--- Nuevo intento (Ctrl+C para salir) ---\n");
        printf("Rol del ejecutor (0-4): ");
        if (!fgets(linea, sizeof(linea), stdin)) break;
        if (sscanf(linea, "%d", &rol) != 1) { printf("  entrada invalida\n\n"); continue; }

        printf("caller_id (quien ejecuta, p.ej. 50): ");
        if (!fgets(linea, sizeof(linea), stdin)) break;
        if (sscanf(linea, "%d", &caller_id) != 1) { printf("  entrada invalida\n\n"); continue; }

        printf("new_id (profesional a registrar, p.ej. 1001): ");
        if (!fgets(linea, sizeof(linea), stdin)) break;
        if (sscanf(linea, "%d", &new_id) != 1) { printf("  entrada invalida\n\n"); continue; }

        rc = professional_register(&st, (role_t)rol, caller_id, new_id);

        printf("\n  -> register(rol=%s, caller_id=%d, new_id=%d)\n",
               nombre_rol(rol), caller_id, new_id);
        printf("  -> resultado: %d = %s\n", rc, nombre_error(rc));
        if (rc == EDU_OK) {
            printf("  -> profesionales registrados: %d | %d activo? %s\n",
                   st.pros_count, new_id,
                   professional_is_active(&st, new_id) ? "SI" : "NO");
            printf("  -> auditoria: evento #%d actor_id=%d target_id=%d\n",
                   st.log[st.log_count - 1].seq,
                   st.log[st.log_count - 1].actor_id,
                   st.log[st.log_count - 1].target_id);
        }
        printf("\n");
    }

    printf("\nFin. Total registrados en esta sesion: %d\n", st.pros_count);
    return 0;
}
