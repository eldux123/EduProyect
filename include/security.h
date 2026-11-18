/* EduChain - security.h
 * Punto unico de decision de autorizacion (matriz de permisos, Fase 1 §3).
 */
#ifndef EDUCHAIN_SECURITY_H
#define EDUCHAIN_SECURITY_H

#include "common.h"

/* Retorna 1 si caller_role puede ejecutar op_code, 0 si no.
 * Funcion pura de solo lectura (assigns \nothing). */
int security_can(role_t caller_role, int op_code);

#endif /* EDUCHAIN_SECURITY_H */
