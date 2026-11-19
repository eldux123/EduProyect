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
/*@
  requires \valid(st);
  requires 0 <= st->exps_count <= MAX_EXPERIENCES;
  requires 0 <= st->log_count  <= MAX_EVENTS;
  requires 0 <= st->pros_count <= MAX_PROFESSIONALS;
  assigns st->exps[st->exps_count], st->exps_count,
          st->log[st->log_count], st->log_count;
  ensures \result == EDU_OK || \result == ERR_PERM || \result == ERR_BAD_ID
       || \result == ERR_BAD_DATE || \result == ERR_NOT_FOUND
       || \result == ERR_DUPLICATE || \result == ERR_FULL;
  // P-1: rol no autorizado => ERR_PERM.
  ensures (caller_role != ROLE_HR && caller_role != ROLE_PRO) ==> \result == ERR_PERM;
  // Un PRO solo registra experiencia propia.
  ensures (caller_role == ROLE_PRO && caller_id != owner_id) ==> \result == ERR_PERM;
  // P-4 (BR-4): en exito, la nueva experiencia cumple rango y start <= end.
  ensures \result == EDU_OK ==>
          (DATE_MIN <= st->exps[\old(st->exps_count)].start_date
           && st->exps[\old(st->exps_count)].start_date <= st->exps[\old(st->exps_count)].end_date
           && st->exps[\old(st->exps_count)].end_date <= DATE_MAX);
  // Inicializacion: la nueva experiencia nace PENDING con validator_id == 0.
  ensures \result == EDU_OK ==>
          (st->exps[\old(st->exps_count)].state == EXP_PENDING
           && st->exps[\old(st->exps_count)].validator_id == 0);
  // Atomicidad (BR-10).
  ensures \result == EDU_OK ==> st->exps_count == \old(st->exps_count) + 1;
  ensures \result != EDU_OK ==> st->exps_count == \old(st->exps_count);
  ensures st->exps_count <= MAX_EXPERIENCES;
  // NOTA: varias de estas postcondiciones y los sub-objetivos `assigns` de esta
  // funcion quedan en Failure/Timeout por limitacion de automatizacion de WP
  // (struct ~7 KB + 3 llamadas de busqueda + auditoria). Se conservan por
  // fidelidad al modelo; su estado real se registra en docs/formal_verification.md.
*/
int experience_register(educhain_state_t *st, role_t caller_role, int caller_id,
                        int exp_id, int owner_id, int start_date, int end_date);

/* Valida una experiencia PENDING: la pasa a EXP_VALIDATED y fija validator_id.
 * Permitido a ROLE_HR y ROLE_COORD. Prohibido validar la propia experiencia
 * (caller_id debe diferir del owner). No modifica fechas ni owner (inmutabilidad).
 * Retorna EDU_OK o ERR_*. */
/*@
  requires \valid(st);
  requires 0 <= st->exps_count <= MAX_EXPERIENCES;
  requires 0 <= st->log_count  <= MAX_EVENTS;
  // `assigns` acotado por set-comprehension: solo las experiencias cuyo id == exp_id
  // pueden cambiar state/validator (el codigo toca exactamente esa, que es unica).
  assigns { st->exps[k].state | integer k;
            0 <= k < st->exps_count && st->exps[k].id == exp_id },
          { st->exps[k].validator_id | integer k;
            0 <= k < st->exps_count && st->exps[k].id == exp_id },
          st->log[st->log_count], st->log_count;
  ensures \result == EDU_OK || \result == ERR_PERM || \result == ERR_BAD_ID
       || \result == ERR_NOT_FOUND || \result == ERR_SELF_VALIDATE
       || \result == ERR_ALREADY_VALID || \result == ERR_FULL;
  // P-1: rol no autorizado nunca valida.
  ensures (caller_role != ROLE_HR && caller_role != ROLE_COORD)
          ==> \result == ERR_PERM;
  // El numero de experiencias no cambia (validate no inserta).
  ensures st->exps_count == \old(st->exps_count);
  // -------------------------------------------------------------------------
  // #3 P-2 (NO AUTOVALIDACION, regla de seguridad R-2) formalizada como ensures.
  // Si existe la experiencia exp_id y su titular (owner_id en el pre-estado) es el
  // propio llamador, el resultado es ERR_SELF_VALIDATE (nunca EDU_OK). Anclado en
  // \old: el owner no cambia (es inmutable por el `assigns`), pero se referencia el
  // pre-estado para dejar explicito que la decision se toma sobre el owner original.
  ensures (\exists integer k; 0 <= k < \old(st->exps_count)
           && st->exps[k].id == exp_id && \old(st->exps[k].owner_id) == caller_id)
          ==> \result == ERR_SELF_VALIDATE;
  // (El dual positivo "en exito validator != owner" se evaluo pero NO cierra en WP:
  //  requiere unicidad global de exp_id (state_inv) para ligar el idx hallado con
  //  la k del existencial. Se omite como ensures y se documenta como pendiente por
  //  el limite del probador; respaldo dinamico en test_attacks.c A2.)
  // -------------------------------------------------------------------------
  // P-5 (INMUTABILIDAD de owner/fechas): capturada POR EL `assigns` (esos campos
  // NO son escribibles: solo state/validator de la exp con id==exp_id).
  // El resto del efecto funcional (transicion a VALIDATED, ERR_ALREADY_VALID) se
  // intento formalizar como `ensures` universales/existenciales pero WP no los
  // cierra en esta configuracion (mismo patron de framing sobre struct ~7 KB).
  // Se documentan como PENDIENTES en docs/formal_verification.md, con respaldo
  // dinamico en test_attacks.c (A1, A2, A3) y test_experience.c.
  // -------------------------------------------------------------------------
*/
int experience_validate(educhain_state_t *st, role_t caller_role, int caller_id, int exp_id);

/* Busca por exp_id. Retorna indice o -1. Solo lectura. */
/*@ requires \valid_read(st);
    requires 0 <= st->exps_count <= MAX_EXPERIENCES;
    assigns \nothing;
    ensures -1 <= \result < st->exps_count;
    ensures \result >= 0 ==> st->exps[\result].id == exp_id;
    ensures \result == -1 ==>
            \forall integer k; 0 <= k < st->exps_count ==> st->exps[k].id != exp_id;
*/
int experience_find(const educhain_state_t *st, int exp_id);

/* Cuenta las experiencias en estado EXP_VALIDATED cuyo titular es owner_id.
 * Solo lectura; no realiza aritmetica de fechas.
 * Retorna: ERR_NULL si st es nulo; ERR_BAD_ID si owner_id fuera de rango;
 * ERR_NOT_FOUND si el profesional no existe; en otro caso el conteo (>= 0). */
/*@ requires \valid_read(st);
    requires 0 <= st->exps_count  <= MAX_EXPERIENCES;
    requires 0 <= st->pros_count  <= MAX_PROFESSIONALS;
    assigns \nothing;
    ensures \result == ERR_BAD_ID || \result == ERR_NOT_FOUND
         || (0 <= \result <= st->exps_count);
*/
int experience_validated_count(const educhain_state_t *st, int owner_id);

#endif /* EDUCHAIN_EXPERIENCE_H */
