/* EduChain - credentials.h
 * Modulo 3: credenciales digitales como maquina de estados.
 * Estados: PENDING -> ACTIVE -> BLOCKED <-> ACTIVE -> REVOKED (terminal).
 */
#ifndef EDUCHAIN_CREDENTIALS_H
#define EDUCHAIN_CREDENTIALS_H

#include "common.h"

/* Emite una credencial en CRED_PENDING para owner_id (una por profesional).
 * Permitido a ROLE_COORD. */
int credential_issue(educhain_state_t *st, role_t caller_role, int cred_id, int owner_id);

/* PENDING -> ACTIVE. Permitido a ROLE_COORD. */
int credential_activate(educhain_state_t *st, role_t caller_role, int cred_id);

/* ACTIVE -> BLOCKED. Permitido a ROLE_ADMIN/ROLE_COORD. */
int credential_block(educhain_state_t *st, role_t caller_role, int cred_id);

/* BLOCKED -> ACTIVE. Permitido a ROLE_ADMIN/ROLE_COORD. */
int credential_reactivate(educhain_state_t *st, role_t caller_role, int cred_id);

/* {PENDING,ACTIVE,BLOCKED} -> REVOKED (terminal). Permitido a ROLE_ADMIN/ROLE_COORD. */
int credential_revoke(educhain_state_t *st, role_t caller_role, int cred_id);

/* Retorna el estado (cred_state_t) de la credencial, o -1 si no existe. Solo lectura. */
int credential_status(const educhain_state_t *st, int cred_id);

/* Retorna 1 si la credencial es valida para uso (solo CRED_ACTIVE), 0 en otro caso. Solo lectura. */
int credential_is_valid(const educhain_state_t *st, int cred_id);

#endif /* EDUCHAIN_CREDENTIALS_H */
