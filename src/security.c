/* EduChain - security.c  (Fase 3: implementacion)
 * Punto unico de decision de autorizacion. Codifica la matriz de permisos (Fase 1 §3).
 */
#include "security.h"

int security_can(role_t caller_role, int op_code)
{
    switch (op_code) {
    case OP_REGISTER_PRO:
        return (caller_role == ROLE_ADMIN || caller_role == ROLE_HR) ? 1 : 0;
    case OP_REGISTER_EXP:
        /* HR siempre; PRO se permite aqui a nivel de rol, la restriccion
         * caller_id == owner_id se verifica en experience_register. */
        return (caller_role == ROLE_HR || caller_role == ROLE_PRO) ? 1 : 0;
    case OP_VALIDATE_EXP:
        return (caller_role == ROLE_HR || caller_role == ROLE_COORD) ? 1 : 0;
    case OP_ISSUE_CRED:
    case OP_ACTIVATE_CRED:
        return (caller_role == ROLE_COORD) ? 1 : 0;
    case OP_BLOCK_CRED:
    case OP_REACTIVATE_CRED:
    case OP_REVOKE_CRED:
        return (caller_role == ROLE_ADMIN || caller_role == ROLE_COORD) ? 1 : 0;
    default:
        return 0;
    }
}
