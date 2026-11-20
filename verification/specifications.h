/* EduChain - specifications.h
 * RESERVADO PARA FASE 4: macros y predicados ACSL compartidos.
 * En Fase 2 es un stub documentado; aun no contiene especificaciones.
 */
#ifndef EDUCHAIN_SPECIFICATIONS_H
#define EDUCHAIN_SPECIFICATIONS_H

#include "common.h"

/* Fase 4 (correccion ACSL #1): predicado logico que refleja EXACTAMENTE la matriz
 * de permisos de security_can (src/security.c) y su contrato bicondicional en
 * include/security.h. No es una funcion C: es la MISMA regla expresada como
 * predicado logico para poder referenciar la autorizacion (P-1) dentro de los
 * contratos ACSL (p. ej. la auxiliar cred_transition, cuyo op_code es parametro).
 * No debilita ni presupone nada: es la definicion de la matriz §3, verificable
 * contra el bicondicional de security.h. */
/*@
  predicate role_can(role_t r, integer op) =
       (op == OP_REGISTER_PRO    ==> (r == ROLE_ADMIN || r == ROLE_HR))
    && (op == OP_REGISTER_EXP    ==> (r == ROLE_HR    || r == ROLE_PRO))
    && (op == OP_VALIDATE_EXP    ==> (r == ROLE_HR    || r == ROLE_COORD))
    && ((op == OP_ISSUE_CRED || op == OP_ACTIVATE_CRED) ==> (r == ROLE_COORD))
    && ((op == OP_BLOCK_CRED || op == OP_REACTIVATE_CRED || op == OP_REVOKE_CRED)
        ==> (r == ROLE_ADMIN || r == ROLE_COORD))
    && (!(op == OP_REGISTER_PRO || op == OP_REGISTER_EXP || op == OP_VALIDATE_EXP
          || op == OP_ISSUE_CRED || op == OP_ACTIVATE_CRED || op == OP_BLOCK_CRED
          || op == OP_REACTIVATE_CRED || op == OP_REVOKE_CRED) ==> \false)
    && (r == ROLE_VERIFIER ==> \false);
*/

#endif /* EDUCHAIN_SPECIFICATIONS_H */
