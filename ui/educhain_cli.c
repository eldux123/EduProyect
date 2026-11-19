/* EduChain - ui/educhain_cli.c
 * Interfaz interactiva de terminal (menu) para operar el contrato ficticio
 * EduChain de forma manual y ver estado, resultados e historial.
 *
 * CAPA DE PRESENTACION PURA: este archivo NO contiene logica de negocio.
 * Toda decision (permisos, transiciones, validacion, no-autovalidacion) la
 * toma el modulo correspondiente a traves de las funciones publicas de
 * include/. La UI solo recoge datos del usuario, llama y muestra el resultado,
 * reusando los codigos de retorno definidos en common.h.
 *
 * No modifica nada en src/, include/, verification/, tests/ ni docs/.
 * Solo C estandar (c99) + gcc. Sin ncurses ni dependencias nuevas.
 * Codigos ANSI estandar para color/limpieza de pantalla.
 *
 * Proyecto academico ficticio: NO es una blockchain real.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "security.h"
#include "audit.h"
#include "professionals.h"
#include "experience.h"
#include "credentials.h"

/* ------------------------------------------------------------------ */
/* Colores ANSI estandar                                               */
/* ------------------------------------------------------------------ */
#define C_RESET   "\033[0m"
#define C_BOLD    "\033[1m"
#define C_DIM     "\033[2m"
#define C_RED     "\033[31m"
#define C_GREEN   "\033[32m"
#define C_YELLOW  "\033[33m"
#define C_BLUE    "\033[34m"
#define C_MAGENTA "\033[35m"
#define C_CYAN    "\033[36m"

/* ------------------------------------------------------------------ */
/* Estado de la sesion de UI (ejecutor actual). El estado del contrato */
/* vive en educhain_state_t; aqui solo guardamos quien "ejecuta".      */
/* ------------------------------------------------------------------ */
static educhain_state_t g_state;
static role_t g_caller_role = ROLE_ADMIN;
static int    g_caller_id   = 50;

/* ------------------------------------------------------------------ */
/* Traduccion de codigos a texto legible (solo presentacion).          */
/* ------------------------------------------------------------------ */
static const char *rc_name(int rc)
{
    switch (rc) {
        case EDU_OK:              return "EDU_OK";
        case ERR_NULL:            return "ERR_NULL";
        case ERR_PERM:            return "ERR_PERM";
        case ERR_SELF_VALIDATE:   return "ERR_SELF_VALIDATE";
        case ERR_DUPLICATE:       return "ERR_DUPLICATE";
        case ERR_NOT_FOUND:       return "ERR_NOT_FOUND";
        case ERR_BAD_DATE:        return "ERR_BAD_DATE";
        case ERR_BAD_ID:          return "ERR_BAD_ID";
        case ERR_FULL:            return "ERR_FULL";
        case ERR_BAD_TRANSITION:  return "ERR_BAD_TRANSITION";
        case ERR_IMMUTABLE:       return "ERR_IMMUTABLE";
        case ERR_ALREADY_VALID:   return "ERR_ALREADY_VALID";
        default:                  return "(codigo desconocido)";
    }
}

static const char *rc_desc(int rc)
{
    switch (rc) {
        case EDU_OK:              return "operacion aceptada";
        case ERR_NULL:            return "puntero de estado nulo";
        case ERR_PERM:            return "el rol del ejecutor no tiene permiso para esta operacion";
        case ERR_SELF_VALIDATE:   return "no se puede validar la experiencia propia (regla de no-autovalidacion)";
        case ERR_DUPLICATE:       return "ya existe un registro con ese identificador";
        case ERR_NOT_FOUND:       return "el registro referenciado no existe";
        case ERR_BAD_DATE:        return "fecha fuera de rango o start_date > end_date";
        case ERR_BAD_ID:          return "identificador fuera del rango permitido";
        case ERR_FULL:            return "capacidad del registro agotada";
        case ERR_BAD_TRANSITION:  return "transicion de estado ilegal (p. ej. REVOKED es terminal)";
        case ERR_IMMUTABLE:       return "campo inmutable: no puede modificarse";
        case ERR_ALREADY_VALID:   return "la experiencia ya estaba validada";
        default:                  return "resultado no reconocido";
    }
}

static const char *role_name(role_t r)
{
    switch (r) {
        case ROLE_ADMIN:    return "ADMIN";
        case ROLE_HR:       return "HR";
        case ROLE_COORD:    return "COORD";
        case ROLE_PRO:      return "PRO";
        case ROLE_VERIFIER: return "VERIFIER";
        default:            return "?";
    }
}

static const char *exp_state_name(exp_state_t s)
{
    switch (s) {
        case EXP_PENDING:   return "PENDING";
        case EXP_VALIDATED: return "VALIDATED";
        default:            return "?";
    }
}

static const char *cred_state_name(int s)
{
    switch (s) {
        case CRED_PENDING: return "PENDING";
        case CRED_ACTIVE:  return "ACTIVE";
        case CRED_BLOCKED: return "BLOCKED";
        case CRED_REVOKED: return "REVOKED";
        default:           return "(no existe)";
    }
}

static const char *op_name(int op)
{
    switch (op) {
        case OP_REGISTER_PRO:    return "REGISTER_PRO";
        case OP_REGISTER_EXP:    return "REGISTER_EXP";
        case OP_VALIDATE_EXP:    return "VALIDATE_EXP";
        case OP_ISSUE_CRED:      return "ISSUE_CRED";
        case OP_ACTIVATE_CRED:   return "ACTIVATE_CRED";
        case OP_BLOCK_CRED:      return "BLOCK_CRED";
        case OP_REACTIVATE_CRED: return "REACTIVATE_CRED";
        case OP_REVOKE_CRED:     return "REVOKE_CRED";
        default:                 return "?";
    }
}

/* ------------------------------------------------------------------ */
/* Salida: muestra el resultado de una operacion diferenciado por color*/
/* ------------------------------------------------------------------ */
static void show_result(const char *titulo, int rc)
{
    if (rc == EDU_OK) {
        printf(C_GREEN C_BOLD "  [OK] " C_RESET C_GREEN "%s -> %s (%d): %s\n" C_RESET,
               titulo, rc_name(rc), rc, rc_desc(rc));
    } else {
        printf(C_RED C_BOLD "  [RECHAZADO] " C_RESET C_RED "%s -> %s (%d): %s\n" C_RESET,
               titulo, rc_name(rc), rc, rc_desc(rc));
    }
}

/* ------------------------------------------------------------------ */
/* Entrada robusta: nunca confia en que el usuario teclee algo valido. */
/* read_line devuelve 0 en EOF (para salir limpiamente por stdin).     */
/* ------------------------------------------------------------------ */
static int read_line(char *buf, size_t n)
{
    if (fgets(buf, (int)n, stdin) == NULL) {
        return 0; /* EOF o error: terminar limpiamente */
    }
    /* quitar salto de linea final si existe */
    size_t len = strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
        buf[--len] = '\0';
    }
    return 1;
}

/* Lee un entero. Devuelve 1 si ok, 0 si EOF. En entrada invalida avisa
 * y deja *out sin tocar (el llamador usa un valor por defecto o reintenta
 * segun convenga). Para mantener el flujo simple, en entrada invalida
 * devolvemos 1 con *out = fallback. */
static int read_int(const char *prompt, int fallback, int *out)
{
    char buf[128];
    printf(C_CYAN "%s" C_RESET, prompt);
    fflush(stdout);
    if (!read_line(buf, sizeof(buf))) {
        return 0; /* EOF */
    }
    if (buf[0] == '\0') {
        *out = fallback;
        return 1;
    }
    char *end = NULL;
    long v = strtol(buf, &end, 10);
    /* saltar espacios finales */
    while (end && (*end == ' ' || *end == '\t')) {
        end++;
    }
    if (end == buf || (end && *end != '\0')) {
        printf(C_YELLOW "  entrada no numerica; se usa el valor %d\n" C_RESET, fallback);
        *out = fallback;
        return 1;
    }
    if (v < -2147483647L || v > 2147483647L) {
        printf(C_YELLOW "  numero fuera de rango; se usa el valor %d\n" C_RESET, fallback);
        *out = fallback;
        return 1;
    }
    *out = (int)v;
    return 1;
}

static void clear_screen(void)
{
    /* Codigos ANSI estandar: limpiar pantalla + cursor arriba-izquierda. */
    printf("\033[2J\033[H");
}

static void pause_enter(void)
{
    char buf[128];
    printf(C_DIM "\n  (ENTER para continuar)" C_RESET);
    fflush(stdout);
    (void)read_line(buf, sizeof(buf));
}

/* ------------------------------------------------------------------ */
/* Cabecera con el ejecutor actual                                     */
/* ------------------------------------------------------------------ */
static void print_header(void)
{
    printf(C_BOLD C_BLUE "==============================================================\n" C_RESET);
    printf(C_BOLD C_BLUE "  EduChain CLI" C_RESET
           C_DIM "  (simulacion academica, NO blockchain real)\n" C_RESET);
    printf(C_BOLD C_BLUE "==============================================================\n" C_RESET);
    printf("  Ejecutor actual: " C_MAGENTA C_BOLD "caller_id=%d  rol=%s\n" C_RESET,
           g_caller_id, role_name(g_caller_role));
    printf("  Registros: profesionales=%d  experiencias=%d  credenciales=%d  eventos=%d\n",
           g_state.pros_count, g_state.exps_count, g_state.creds_count, g_state.log_count);
    printf(C_BOLD C_BLUE "--------------------------------------------------------------\n" C_RESET);
}

/* ================================================================== */
/* Operaciones del menu. Cada una recoge datos, llama al modulo y      */
/* muestra el codigo de retorno. Sin reglas de negocio aqui.           */
/* ================================================================== */

static void do_register_pro(void)
{
    int new_id;
    printf(C_BOLD "\n> Registrar profesional\n" C_RESET);
    printf(C_DIM "  (requiere rol ADMIN o HR; el contrato decide)\n" C_RESET);
    if (!read_int("  ID del nuevo profesional: ", 0, &new_id)) return;
    int rc = professional_register(&g_state, g_caller_role, g_caller_id, new_id);
    show_result("professional_register", rc);
}

static void do_register_exp(void)
{
    int exp_id, owner_id, start_date, end_date;
    printf(C_BOLD "\n> Registrar experiencia\n" C_RESET);
    printf(C_DIM "  (submitter: HR, o PRO solo para su propia experiencia)\n" C_RESET);
    if (!read_int("  ID de la experiencia: ", 0, &exp_id)) return;
    if (!read_int("  ID del titular (owner): ", 0, &owner_id)) return;
    if (!read_int("  Fecha inicio (YYYYMMDD): ", 0, &start_date)) return;
    if (!read_int("  Fecha fin    (YYYYMMDD): ", 0, &end_date)) return;
    int rc = experience_register(&g_state, g_caller_role, g_caller_id,
                                 exp_id, owner_id, start_date, end_date);
    show_result("experience_register", rc);
}

static void do_validate_exp(void)
{
    int exp_id;
    printf(C_BOLD "\n> Validar experiencia\n" C_RESET);
    printf(C_DIM "  (validador HR/COORD, distinto del titular: el contrato\n"
           "   rechaza la autovalidacion con ERR_SELF_VALIDATE)\n" C_RESET);
    if (!read_int("  ID de la experiencia a validar: ", 0, &exp_id)) return;
    int rc = experience_validate(&g_state, g_caller_role, g_caller_id, exp_id);
    show_result("experience_validate", rc);
}

static void do_issue_cred(void)
{
    int cred_id, owner_id;
    printf(C_BOLD "\n> Emitir credencial\n" C_RESET);
    printf(C_DIM "  (requiere rol COORD; nace en estado PENDING)\n" C_RESET);
    if (!read_int("  ID de la credencial: ", 0, &cred_id)) return;
    if (!read_int("  ID del titular (owner): ", 0, &owner_id)) return;
    int rc = credential_issue(&g_state, g_caller_role, g_caller_id, cred_id, owner_id);
    show_result("credential_issue", rc);
}

static void do_cred_transition(const char *label, int which)
{
    int cred_id;
    printf(C_BOLD "\n> %s\n" C_RESET, label);
    if (!read_int("  ID de la credencial: ", 0, &cred_id)) return;
    int rc = ERR_NULL;
    switch (which) {
        case OP_ACTIVATE_CRED:
            rc = credential_activate(&g_state, g_caller_role, g_caller_id, cred_id);
            show_result("credential_activate", rc);
            break;
        case OP_BLOCK_CRED:
            rc = credential_block(&g_state, g_caller_role, g_caller_id, cred_id);
            show_result("credential_block", rc);
            break;
        case OP_REACTIVATE_CRED:
            rc = credential_reactivate(&g_state, g_caller_role, g_caller_id, cred_id);
            show_result("credential_reactivate", rc);
            break;
        case OP_REVOKE_CRED:
            rc = credential_revoke(&g_state, g_caller_role, g_caller_id, cred_id);
            show_result("credential_revoke", rc);
            break;
        default:
            break;
    }
}

static void do_query_cred(void)
{
    int cred_id;
    printf(C_BOLD "\n> Consultar estado de credencial\n" C_RESET);
    if (!read_int("  ID de la credencial: ", 0, &cred_id)) return;
    int st = credential_status(&g_state, cred_id);
    int valid = credential_is_valid(&g_state, cred_id);
    if (st < 0) {
        printf(C_RED "  La credencial %d no existe.\n" C_RESET, cred_id);
    } else {
        printf("  Credencial %d: estado = " C_BOLD "%s" C_RESET
               "  | valida para uso: %s%s" C_RESET "\n",
               cred_id, cred_state_name(st),
               valid ? C_GREEN : C_YELLOW, valid ? "SI" : "NO");
    }
}

/* ------------------------------------------------------------------ */
/* Listados en forma de tabla alineada                                 */
/* ------------------------------------------------------------------ */
static void list_pros(void)
{
    printf(C_BOLD "\n> Profesionales registrados (%d)\n" C_RESET, g_state.pros_count);
    printf(C_DIM "  +--------+------------+----------+\n" C_RESET);
    printf(C_DIM "  | %-6s | %-10s | %-8s |\n" C_RESET, "#", "ID", "activo");
    printf(C_DIM "  +--------+------------+----------+\n" C_RESET);
    for (int i = 0; i < g_state.pros_count; i++) {
        printf("  | %-6d | %-10d | %-8s |\n",
               i, g_state.pros[i].id, g_state.pros[i].active ? "si" : "no");
    }
    printf(C_DIM "  +--------+------------+----------+\n" C_RESET);
}

static void list_exps(void)
{
    printf(C_BOLD "\n> Experiencias registradas (%d)\n" C_RESET, g_state.exps_count);
    printf(C_DIM "  +------------+------------+------------+------------+------------+------------+\n" C_RESET);
    printf(C_DIM "  | %-10s | %-10s | %-10s | %-10s | %-10s | %-10s |\n" C_RESET,
           "ID", "owner", "inicio", "fin", "estado", "validador");
    printf(C_DIM "  +------------+------------+------------+------------+------------+------------+\n" C_RESET);
    for (int i = 0; i < g_state.exps_count; i++) {
        experience_t *e = &g_state.exps[i];
        printf("  | %-10d | %-10d | %-10d | %-10d | %-10s | %-10d |\n",
               e->id, e->owner_id, e->start_date, e->end_date,
               exp_state_name(e->state), e->validator_id);
    }
    printf(C_DIM "  +------------+------------+------------+------------+------------+------------+\n" C_RESET);
}

static void list_creds(void)
{
    printf(C_BOLD "\n> Credenciales registradas (%d)\n" C_RESET, g_state.creds_count);
    printf(C_DIM "  +------------+------------+------------+----------+\n" C_RESET);
    printf(C_DIM "  | %-10s | %-10s | %-10s | %-8s |\n" C_RESET,
           "ID", "owner", "estado", "valida");
    printf(C_DIM "  +------------+------------+------------+----------+\n" C_RESET);
    for (int i = 0; i < g_state.creds_count; i++) {
        credential_t *c = &g_state.creds[i];
        printf("  | %-10d | %-10d | %-10s | %-8s |\n",
               c->id, c->owner_id, cred_state_name(c->state),
               c->state == CRED_ACTIVE ? "si" : "no");
    }
    printf(C_DIM "  +------------+------------+------------+----------+\n" C_RESET);
}

static void list_all(void)
{
    list_pros();
    list_exps();
    list_creds();
}

static void show_audit(void)
{
    int n = audit_get_count(&g_state);
    printf(C_BOLD "\n> Historial de auditoria (%d eventos)\n" C_RESET, n);
    printf(C_DIM "  +--------+-----------------+------------+------------+---------------------+\n" C_RESET);
    printf(C_DIM "  | %-6s | %-15s | %-10s | %-10s | %-19s |\n" C_RESET,
           "seq", "operacion", "actor", "objetivo", "resultado");
    printf(C_DIM "  +--------+-----------------+------------+------------+---------------------+\n" C_RESET);
    for (int i = 0; i < n; i++) {
        event_t *ev = &g_state.log[i];
        char res[48];
        snprintf(res, sizeof(res), "%s(%d)", rc_name(ev->result), ev->result);
        printf("  | %-6d | %-15s | %-10d | %-10d | %-19s |\n",
               ev->seq, op_name(ev->op_code), ev->actor_id, ev->target_id, res);
    }
    printf(C_DIM "  +--------+-----------------+------------+------------+---------------------+\n" C_RESET);
}

/* ------------------------------------------------------------------ */
/* Cambiar ejecutor (caller_id y rol)                                  */
/* ------------------------------------------------------------------ */
static void change_caller(void)
{
    int id, r;
    printf(C_BOLD "\n> Cambiar ejecutor\n" C_RESET);
    if (!read_int("  Nuevo caller_id: ", g_caller_id, &id)) return;
    printf("  Roles: 0=ADMIN  1=HR  2=COORD  3=PRO  4=VERIFIER\n");
    if (!read_int("  Nuevo rol (0-4): ", (int)g_caller_role, &r)) return;
    if (r < ROLE_ADMIN || r > ROLE_VERIFIER) {
        printf(C_YELLOW "  rol fuera de rango; sin cambios de rol\n" C_RESET);
    } else {
        g_caller_role = (role_t)r;
    }
    g_caller_id = id;
    printf(C_GREEN "  Ejecutor ahora: caller_id=%d rol=%s\n" C_RESET,
           g_caller_id, role_name(g_caller_role));
}

/* ------------------------------------------------------------------ */
/* Escenario precargado para demo (varios pros, exps y creds).         */
/* Usa unicamente las funciones publicas de los modulos.               */
/* ------------------------------------------------------------------ */
static void load_scenario(void)
{
    memset(&g_state, 0, sizeof(g_state));
    printf(C_BOLD "\n> Cargando escenario de ejemplo...\n" C_RESET);

    /* Profesionales (HR los registra; caller_id 50 = ejecutor HR ficticio). */
    professional_register(&g_state, ROLE_HR, 50, 1001);
    professional_register(&g_state, ROLE_HR, 50, 1002);
    professional_register(&g_state, ROLE_ADMIN, 50, 1003);

    /* Experiencias (PRO registra la propia; HR registra otras). */
    experience_register(&g_state, ROLE_PRO, 1001, 5001, 1001, 20190101, 20211231);
    experience_register(&g_state, ROLE_HR,  9,    5002, 1002, 20200101, 20221231);
    experience_register(&g_state, ROLE_HR,  9,    5003, 1003, 20180101, 20201231);

    /* Validaciones por un tercero (COORD, distinto del titular). */
    experience_validate(&g_state, ROLE_COORD, 7, 5001);
    experience_validate(&g_state, ROLE_COORD, 7, 5002);

    /* Credenciales: emitir (COORD) y llevar a distintos estados. */
    credential_issue(&g_state, ROLE_COORD, 60, 7001, 1001);
    credential_activate(&g_state, ROLE_COORD, 60, 7001);           /* ACTIVE  */

    credential_issue(&g_state, ROLE_COORD, 60, 7002, 1002);
    credential_activate(&g_state, ROLE_COORD, 60, 7002);
    credential_block(&g_state, ROLE_ADMIN, 50, 7002);              /* BLOCKED */

    credential_issue(&g_state, ROLE_COORD, 60, 7003, 1003);        /* PENDING */

    printf(C_GREEN "  Escenario cargado: 3 profesionales, 3 experiencias, 3 credenciales.\n" C_RESET);
    printf(C_DIM "  (cred 7001=ACTIVE, 7002=BLOCKED, 7003=PENDING)\n" C_RESET);
}

/* ------------------------------------------------------------------ */
/* Menu principal                                                      */
/* ------------------------------------------------------------------ */
static void print_menu(void)
{
    printf("\n" C_BOLD "  Operaciones:\n" C_RESET);
    printf("   1) Registrar profesional\n");
    printf("   2) Registrar experiencia\n");
    printf("   3) Validar experiencia\n");
    printf("   4) Emitir credencial\n");
    printf("   5) Activar credencial   (PENDING -> ACTIVE)\n");
    printf("   6) Bloquear credencial  (ACTIVE  -> BLOCKED)\n");
    printf("   7) Reactivar credencial (BLOCKED -> ACTIVE)\n");
    printf("   8) Revocar credencial   (-> REVOKED, terminal)\n");
    printf("   9) Consultar estado de credencial\n");
    printf("  10) Listar profesionales / experiencias / credenciales\n");
    printf("  11) Ver historial de auditoria\n");
    printf("  12) Cambiar ejecutor (caller_id y rol)\n");
    printf("  13) Cargar escenario de ejemplo\n");
    printf("   0) Salir\n");
}

int main(void)
{
    int running = 1;
    memset(&g_state, 0, sizeof(g_state));

    while (running) {
        clear_screen();
        print_header();
        print_menu();

        int choice;
        if (!read_int("\n  Opcion: ", -1, &choice)) {
            /* EOF: salir limpiamente (modo stdin/heredoc). */
            printf("\n" C_DIM "  (EOF) saliendo...\n" C_RESET);
            break;
        }

        switch (choice) {
            case 1:  do_register_pro(); break;
            case 2:  do_register_exp(); break;
            case 3:  do_validate_exp(); break;
            case 4:  do_issue_cred(); break;
            case 5:  do_cred_transition("Activar credencial", OP_ACTIVATE_CRED); break;
            case 6:  do_cred_transition("Bloquear credencial", OP_BLOCK_CRED); break;
            case 7:  do_cred_transition("Reactivar credencial", OP_REACTIVATE_CRED); break;
            case 8:  do_cred_transition("Revocar credencial", OP_REVOKE_CRED); break;
            case 9:  do_query_cred(); break;
            case 10: list_all(); break;
            case 11: show_audit(); break;
            case 12: change_caller(); break;
            case 13: load_scenario(); break;
            case 0:
                running = 0;
                printf("\n" C_DIM "  Saliendo. Hasta luego.\n" C_RESET);
                break;
            default:
                printf(C_YELLOW "\n  Opcion no valida: %d\n" C_RESET, choice);
                break;
        }

        if (running) {
            pause_enter();
        }
    }
    return 0;
}
