/* EduChain - experience.h
 * Modulo 2: registro y validacion de experiencia.
 */
#ifndef EDUCHAIN_EXPERIENCE_H
#define EDUCHAIN_EXPERIENCE_H

#include "common.h"

/* Registra una experiencia en estado EXP_PENDING.
 * Permitido a ROLE_HR, y a ROLE_PRO solo si caller_id == owner_id.
 * Valida rango de fechas y start_date <= end_date; owner debe existir; sin duplicados.
 * Retorna EDU_OK o un codigo de error. */
int experience_register(educhain_state_t *st, role_t caller_role, int caller_id,
                        int exp_id, int owner_id, int start_date, int end_date);

/* Valida una experiencia PENDING: la pasa a EXP_VALIDATED y fija validator_id.
 * Permitido a ROLE_HR y ROLE_COORD. Prohibido validar la propia experiencia
 * (caller_id debe diferir del owner). No modifica fechas ni owner (inmutabilidad).
 * Retorna EDU_OK o ERR_*. */
int experience_validate(educhain_state_t *st, role_t caller_role, int caller_id, int exp_id);

/* Busca por exp_id. Retorna indice o -1. Solo lectura. */
int experience_find(const educhain_state_t *st, int exp_id);

/* Suma de duraciones (dias aproximados) de experiencias VALIDADAS de owner_id,
 * contando cada registro una sola vez. Retorna agregado >= 0. Solo lectura. */
long experience_total_days(const educhain_state_t *st, int owner_id);

#endif /* EDUCHAIN_EXPERIENCE_H */
