/* EduChain - security.h
 * Punto unico de decision de autorizacion (matriz de permisos, Fase 1 §3).
 */
#ifndef EDUCHAIN_SECURITY_H
#define EDUCHAIN_SECURITY_H

#include "common.h"
#ifdef __FRAMAC__
/* Fase 4 (correccion ACSL #1): predicado logico role_can que refleja la matriz.
 * Solo bajo Frama-C (vive en verification/, fuera del -Iinclude del build gcc). */
#include "specifications.h"
#endif

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
  // Fase 4 (#1): equivalencia con el predicado logico role_can (misma matriz §3).
  // Permite que los contratos ACSL de otros modulos (p. ej. cred_transition, cuyo
  // op_code es parametro) deriven la autorizacion P-1 referenciando role_can.
  // No debilita: es la MISMA regla que las clausulas bicondicionales de arriba,
  // reagrupada en un solo predicado; WP la cierra sobre el switch puro.
  ensures \result == 1 <==> role_can(caller_role, op_code);
*/
int security_can(role_t caller_role, int op_code);

#endif /* EDUCHAIN_SECURITY_H */
