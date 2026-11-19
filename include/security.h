/* EduChain - security.h
 * Punto unico de decision de autorizacion (matriz de permisos, Fase 1 §3).
 */
#ifndef EDUCHAIN_SECURITY_H
#define EDUCHAIN_SECURITY_H

#include "common.h"

/* Retorna 1 si caller_role puede ejecutar op_code, 0 si no.
 * Funcion pura de solo lectura. */
/*@
  requires \true;
  assigns \nothing;
  ensures \result == 0 || \result == 1;
  // P-1: caracterizacion completa de la matriz (§3), bi-condicional por operacion.
  // Cada clausula coincide exactamente con el switch de src/security.c.
  ensures (op_code == OP_REGISTER_PRO) ==>
          (\result == 1 <==> (caller_role == ROLE_ADMIN || caller_role == ROLE_HR));
  ensures (op_code == OP_REGISTER_EXP) ==>
          (\result == 1 <==> (caller_role == ROLE_HR || caller_role == ROLE_PRO));
  ensures (op_code == OP_VALIDATE_EXP) ==>
          (\result == 1 <==> (caller_role == ROLE_HR || caller_role == ROLE_COORD));
  ensures (op_code == OP_ISSUE_CRED || op_code == OP_ACTIVATE_CRED) ==>
          (\result == 1 <==> (caller_role == ROLE_COORD));
  ensures (op_code == OP_BLOCK_CRED || op_code == OP_REACTIVATE_CRED
           || op_code == OP_REVOKE_CRED) ==>
          (\result == 1 <==> (caller_role == ROLE_ADMIN || caller_role == ROLE_COORD));
  // Operacion desconocida: siempre 0.
  ensures !(op_code == OP_REGISTER_PRO || op_code == OP_REGISTER_EXP
            || op_code == OP_VALIDATE_EXP || op_code == OP_ISSUE_CRED
            || op_code == OP_ACTIVATE_CRED || op_code == OP_BLOCK_CRED
            || op_code == OP_REACTIVATE_CRED || op_code == OP_REVOKE_CRED)
          ==> \result == 0;
  // un VERIFIER nunca puede escribir nada.
  ensures (caller_role == ROLE_VERIFIER) ==> (\result == 0);
*/
int security_can(role_t caller_role, int op_code);

#endif /* EDUCHAIN_SECURITY_H */
