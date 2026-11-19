# Informe de revisión ACSL — EduChain (segunda ronda / re-revisión)

**Agente:** `revisor-acsl` (experto Frama-C/ACSL)
**Alcance:** especificaciones ACSL en `include/*.h` frente a la implementación real en `src/*.c` y `main.c`, con apoyo de `verification/` y `tests/`.
**Repositorio:** `C:\Users\erick\Downloads\Blockchain\EduProyect`

> **Límites de esta revisión (recordatorio de rol).** Solo reviso y reporto; no modifico archivos. No afirmo que ninguna propiedad esté *demostrada*: eso lo da Frama-C WP, no esta revisión. Donde digo "WP no la cierra" me baso en `verification/wp_results.txt` (salida real de una corrida) y en el estado de la *especificación*, no en una lectura propia de las obligaciones.

---

## 0. Nota de re-revisión: discrepancia con el informe anterior

El encargo indica que *«los headers con ACSL (include/) y la implementación (src/, main.c) NO han cambiado; solo se editaron documentos»* y pide confirmar o refinar los hallazgos previos. Al cotejar palabra por palabra el ACSL de `include/*.h` con el código, **varios hallazgos del informe anterior ya no se sostienen frente a los archivos actuales**. No afirmo por qué (puede ser que el informe previo se redactara sobre un estado anterior de los headers, o que describiera el contrato "ideal" en lugar del presente); me limito a reportar lo que hoy contienen los archivos. Las correcciones concretas:

- **`audit_record_event` (H-2 previo).** El `behavior ok` del header actual **sí** incluye `ensures \result == ERR_FULL ==> st->log_count == \old(st->log_count);` y **sí** incluye los `ensures` de `seq` creciente (`seq == seq[-1]+1` y el caso `log_count==0 ==> seq==1`). El informe previo los daba por faltantes. → Hallazgo previo **desactualizado**; ver §Audit abajo para lo que realmente queda pendiente.
- **`audit_verify_event` y `audit_get_count` (H-3 previo).** Ambas **ya tienen** contrato completo (`requires \valid_read`, `assigns \nothing`, `ensures` sobre `\result` y, en `verify_event`, los dos cuantificadores de hallazgo/no-hallazgo). → Hallazgo previo **desactualizado**.
- **`experience_total_days` (H-12 / tabla P-10 previos).** **Esta función no existe** en `src/experience.c`. La función real es `experience_validated_count`, que **ya tiene** contrato (`requires/assigns \nothing/ensures`) **y** `loop invariant/assigns/variant`. El acumulador que el informe previo describía (`long total`, resta de fechas `YYYYMMDD`, cota anti-overflow) no está presente. → Hallazgo previo **inválido** sobre el código actual.
- **`experience_find`** no tiene invariantes de bucle en el `.c` (confirmado), pero el bucle de **`experience_validated_count` sí** los tiene. El informe previo decía que solo `professional_find` tenía invariantes: hoy hay **tres** bucles anotados (`professional_find`, `audit_verify_event`, `experience_validated_count`).

Todo lo demás del informe previo **se confirma** y se detalla abajo.

---

## 1. Estado de la verificación (WP) — ahora con salida real

A diferencia de la ronda anterior, ahora existe `verification/wp_results.txt` con la salida de una corrida de `frama-c -wp`. Lectura honesta de ese archivo:

- Todas las obligaciones cerradas lo fueron por **Qed** o **Alt-Ergo**; **no aparece Z3** cerrando ninguna (coherente con lo observado en la sesión).
- Hay **Failures** y **Timeouts** reales. Resumen por archivo (proved/total):
  - `security.c`: 69/69, sin fallos.
  - `audit.c`: 68/74, **6 Timeout** (todos en `audit_verify_event`/`audit_get_count`, RTE y terminación).
  - `professionals.c`: 62/74, **5 Failure** + 7 Timeout.
  - `experience.c`: 75/106, **2 Failure** + 29 Timeout.
  - `credentials.c`: 52/86, **34 Timeout** (sin Failure).
- Un **Failure** es una obligación que el prover considera no demostrable con la especificación dada (típicamente spec incorrecta/insuficiente). Un **Timeout** es una obligación que el prover no resolvió en el tiempo dado (típicamente falta de invariantes/hints o `assigns` demasiado amplio que explota el contexto). Ninguno de los dos significa "propiedad refutada".

**No afirmo que ninguna propiedad esté demostrada.** Donde `wp_results.txt` marca `Proved`, eso lo estableció WP, no esta revisión.

---

## 2. Resumen ejecutivo (hallazgos por severidad)

**Anotación incorrecta / insuficiente (bloquean o falsean la verificación):**

1. `professional_register` — `assigns` sobre-aproximado: declara `pros[0..MAX-1]` y `log[0..MAX-1]` cuando el código escribe **una** celda de cada uno. Causa directa de los Failures de `professional_register` en WP. (H-5) **Confirmado.**
2. `experience_validate` — `assigns` sobre-aproximado sobre `exps[0..count-1].state/.validator_id` cuando el código toca `exps[idx]`; falta `ensures` de P-2 (no autovalidación), P-5 (inmutabilidad) y del efecto en éxito. Causa de los Failures `experience_validate_ensures` y `..._assigns_normal_part13`. (H-7, H-8) **Confirmado.**
3. `credential_is_valid` — contrato presente pero **incompleto**: falta el `<==>` que expresa P-6 (`válida ⇔ ACTIVE`); hoy solo acota `\result ∈ {0,1}`. (H previo) **Confirmado.**

**Falta anotación (contratos ausentes que el alcance exige):**

4. `experience_register` — **sin contrato ACSL** en la cabecera (solo prosa). (H-6) **Confirmado.**
5. `credentials` — **sin contrato** en `credential_issue`, `credential_activate`, `credential_block`, `credential_reactivate`, `credential_revoke` y `credential_status`. Solo `credential_is_valid` tiene ACSL. Deja P-6, P-7, P-8 sin especificar. (H-9) **Confirmado.**
6. `experience_find` — sin contrato y **sin invariante de bucle**. (parte de H-12) **Confirmado.**
7. Bucles sin invariantes: `experience_find`, `credential_status`, `cred_find`, `cred_find_by_owner`. (H-12, **refinado**: `audit_verify_event` y `experience_validated_count` **sí** los tienen ahora). **Confirmado con corrección.**
8. Base ACSL compartida ausente: `verification/specifications.h` e `invariants.h` siguen siendo **stubs** en prosa; faltan `valid_id`, `valid_date` y el invariante global `state_inv`. (H-13) **Confirmado.**

**Correcto pero WP probablemente no lo prueba sin invariante global / lemas:**

9. Unicidad de IDs (P-3/BR-3), "una credencial por owner", append-only con `seq` creciente a lo largo de todo el log (P-12/BR-11), inmutabilidad (P-5), terminalidad de REVOKED (P-8) requieren un **invariante global del estado** como predicado ACSL. No existe. (H-14) **Confirmado.**

---

## 3. Revisión por función (estado actual verificado)

### `security_can` (`security.h`) — **correcto**

- `assigns \nothing`, `ensures \result ∈ {0,1}`, bi-condicional de `OP_VALIDATE_EXP` (HR/COORD) y de `OP_ISSUE_CRED/OP_ACTIVATE_CRED` (COORD), y `VERIFIER ==> 0` coinciden exactamente con el `switch` de `src/security.c`.
- `requires \true;` es redundante (no incorrecto).
- **Cruce WP:** `security.c` cierra 69/69. Es el único módulo sin Failures ni Timeouts.
- **Refuerzo opcional (P-1 por composición):** el contrato no caracteriza `OP_REGISTER_PRO` (ADMIN|HR), `OP_REGISTER_EXP` (HR|PRO) ni bloqueo/revocación (ADMIN|COORD). Para que WP use `security_can` como lema al probar `ERR_PERM` en los llamadores, conviene añadir el bi-condicional de todas las operaciones:
  ```acsl
  ensures (op_code == OP_REGISTER_PRO) ==>
          (\result == 1 <==> (caller_role == ROLE_ADMIN || caller_role == ROLE_HR));
  ensures (op_code == OP_REGISTER_EXP) ==>
          (\result == 1 <==> (caller_role == ROLE_HR || caller_role == ROLE_PRO));
  ensures (op_code == OP_BLOCK_CRED || op_code == OP_REACTIVATE_CRED
           || op_code == OP_REVOKE_CRED) ==>
          (\result == 1 <==> (caller_role == ROLE_ADMIN || caller_role == ROLE_COORD));
  ```
  **Clasificación:** *falta anotación* (las existentes son correctas). Nota: en `professionals.c`/`experience.c`/`credentials.c` WP emite `[kernel:annot:missing-spec] include/security.h:24` — es el aviso de que el llamador consume el contrato de `security_can`; no es un error, pero refuerza por qué conviene que ese contrato sea completo.

---

### `audit_record_event` (`audit.h`)

**Bien (confirmado contra el header actual):**
- `requires \valid(st)` y `0 <= log_count <= MAX_EVENTS`.
- `behavior full` (`assumes log_count >= MAX_EVENTS; assigns \nothing; ensures ERR_FULL; log_count sin cambio`) coincide con el código.
- `behavior ok` **ya incluye**: `ensures EDU_OK ==> log_count+1`, `ensures ERR_FULL ==> log_count sin cambio`, y los dos `ensures` de `seq` creciente (`seq[n]==seq[n-1]+1` y `n==0 ==> seq==1`). **Esto corrige H-2 del informe anterior**: esas postcondiciones ya no faltan.
- `complete`/`disjoint behaviors` correctos (las dos ramas particionan por `log_count` vs `MAX_EVENTS`).

**Lo que realmente queda (refinado):**
- **`assigns` por behavior.** El `assigns` global `st->log[st->log_count], st->log_count` cubre la celda completa del arreglo, pero `behavior ok` no declara `assigns` propio: hereda el global, que incluye `log_count`, aunque en la sub-rama `seq` saturado (`last >= 2000000000`) el código retorna `ERR_FULL` **sin** escribir `log_count`. Es compatible (el `ensures ERR_FULL ==> sin cambio` ya lo fija), pero dar a cada behavior su `assigns` exacto ayuda a WP:
  ```acsl
  behavior ok:
    assumes st->log_count < MAX_EVENTS;
    assigns st->log[st->log_count], st->log_count;
  ```
  **Clasificación:** *correcta pero conviene afinar*. El `wp_results.txt` **no** reporta Failure/Timeout en `audit_record_event` mismo, así que esto es refuerzo, no bloqueo.
- **P-12 global.** Los `ensures` locales cubren solo el *último* paso; la monotonía de `seq` sobre **todo** el log es un invariante de estado (ver §4, H-13). *Correcto pero WP no lo cierra sin `state_inv`.*

### `audit_verify_event` y `audit_get_count` — **contratos ya presentes** (corrige H-3)

- `audit_verify_event`: `requires \valid_read`, `assigns \nothing`, `ensures -1 <= \result < log_count`, `ensures \result>=0 ==> log[\result].seq==seq`, `ensures \result==-1 ==> \forall k ... seq != seq`. El bucle en `src/audit.c` **sí** tiene `loop invariant/assigns/variant`. Especificación completa y coherente con el código.
- `audit_get_count`: `assigns \nothing`, `ensures \result == log_count`. Coherente. **Nota de desajuste spec↔código:** el código hace `if (st==0) return 0;` pero `requires \valid_read(st)` excluye el nulo. Es coherente con el estilo "query no-nula", pero inconsistente con otras consultas; decisión de diseño a fijar.
- **Cruce WP:** los **6 Timeout** de `audit.c` están todos aquí (`..._assert_rte_mem_access`, `..._assert_rte_index_bound`, `..._terminates`). Son obligaciones **RTE/terminación**, no de las postcondiciones funcionales. El `loop variant` ya está; el timeout de `_terminates` sugiere que Alt-Ergo no cerró la terminación en el tiempo dado pese al variant (candidato a `-wp-timeout` mayor o a un `loop invariant` de cota más explícito, no a un defecto de spec). **Clasificación:** *correcta; los Timeout son de rendimiento del prover, no de especificación faltante.*

---

### `professional_register` (`professionals.h`)

**Bien (confirmado):** `requires \valid(st)` y rangos; lista de `\result`; P-1 con atomicidad (`!rol ==> ERR_PERM && pros_count sin cambio`); `EDU_OK ==> pros_count+1`; `!=EDU_OK ==> pros_count sin cambio`; `pros_count <= MAX`. El código audita con `caller_id` (actor real), coherente con el comentario.

**H-5 — `assigns` sobre-aproximado (anotación incorrecta). CONFIRMADO y es la causa probable de los Failures.**
```acsl
assigns st->pros[0 .. MAX_PROFESSIONALS-1], st->pros_count,
        st->log[0 .. MAX_EVENTS-1], st->log_count;
```
El código solo escribe `st->pros[n]` (con `n = \old(pros_count)`) y, vía `audit_record_event`, `st->log[\old(log_count)]`. Sugerido:
```acsl
assigns st->pros[\old(st->pros_count)], st->pros_count,
        st->log[\old(st->log_count)], st->log_count;
```
**Por qué WP lo necesita / cruce con `wp_results.txt`:** los 5 Failures son
`typed_professional_register_ensures`, `_ensures_2`, `_ensures_3`, `_ensures_4` y `_assigns_normal_part13`.
El `assigns_normal_part13` es precisamente la obligación de marco: con el rango `pros[0..63]` declarado, WP debe demostrar que *todas* esas celdas pueden quedar como quedan, pero el código solo asigna una tras una llamada a `audit_record_event` cuyo efecto sobre `pros[]` es `\nothing`; la sobre-aproximación hace que las obligaciones `ensures` (que dependen de que el resto del estado no cambió y de que `pros_count` creció exactamente 1) no se cierren. Acotar el `assigns` es el primer cambio a hacer. También corrige los `[wp:pedantic-assigns]` implícitos. **Clasificación:** *anotación incorrecta (demasiado amplia)*.

**H-6 — Falta `ensures` puntual de registro + unicidad (falta / necesita invariante).**
```acsl
ensures \result == EDU_OK ==>
        st->pros[\old(st->pros_count)].id == new_id
        && st->pros[\old(st->pros_count)].active == 1;
```
La **unicidad global** (no hay dos `id` iguales) requiere `state_inv` (H-13); no es un `ensures` puntual. **Clasificación:** *falta anotación* (puntual) + *correcta pero WP no la prueba sin invariante* (unicidad).

### `professional_find` — **correcto**
Invariantes de bucle completos y correctos (`0<=i<=pros_count`, `\forall k<i: id != id`, `loop assigns i`, `loop variant pros_count-i`).
**Cruce WP:** aparecen 7 Timeout de `professional_find` (RTE `mem_access`/`index_bound`, `terminates`, `ensures`, `assigns_part3`). Son RTE y de rendimiento; la especificación es correcta. Refuerzo útil al header:
```acsl
ensures \result >= 0 ==> st->pros[\result].id == id;
ensures \result == -1 ==> \forall integer k; 0 <= k < st->pros_count ==> st->pros[k].id != id;
```

### `professional_is_active` — **correcto** (`assigns \nothing`, `\result ∈ {0,1}`), delega en `professional_find`.

---

### `experience_register` (`experience.h`) — **SIN contrato (H-6). CONFIRMADO.**
Solo prosa en la cabecera. Contrato sugerido (ajustado al código real, que valida rango de fechas y `start<=end`, y restringe PRO a `caller_id==owner_id`):
```acsl
/*@
  requires \valid(st);
  requires 0 <= st->exps_count  <= MAX_EXPERIENCES;
  requires 0 <= st->log_count   <= MAX_EVENTS;
  requires 0 <= st->pros_count  <= MAX_PROFESSIONALS;
  assigns st->exps[\old(st->exps_count)], st->exps_count,
          st->log[\old(st->log_count)], st->log_count;
  ensures \result == EDU_OK || \result == ERR_PERM || \result == ERR_BAD_ID
       || \result == ERR_BAD_DATE || \result == ERR_NOT_FOUND
       || \result == ERR_DUPLICATE || \result == ERR_FULL;
  ensures (caller_role != ROLE_HR && caller_role != ROLE_PRO) ==> \result == ERR_PERM;      // P-1
  ensures (caller_role == ROLE_PRO && caller_id != owner_id) ==> \result == ERR_PERM;
  ensures \result == EDU_OK ==>
          (DATE_MIN <= st->exps[\old(st->exps_count)].start_date
           <= st->exps[\old(st->exps_count)].end_date <= DATE_MAX);                          // P-4
  ensures \result == EDU_OK ==>
          (st->exps[\old(st->exps_count)].state == EXP_PENDING
           && st->exps[\old(st->exps_count)].validator_id == 0);
  ensures \result == EDU_OK ==> st->exps_count == \old(st->exps_count) + 1;                   // BR-10
  ensures \result != EDU_OK ==> st->exps_count == \old(st->exps_count);
  ensures st->exps_count <= MAX_EXPERIENCES;
*/
```
**Por qué WP lo necesita:** sin contrato, WP no verifica P-1/P-4/atomicidad y los llamadores carecen de postcondición. **Cruce WP:** los Timeout `experience_register_assert_rte_*` y `..._call_*_requires` surgen porque, sin `requires` que acoten el estado, WP no puede descargar las precondiciones de `professional_is_active`/`audit_record_event` que `experience_register` invoca. **Clasificación:** *falta anotación*.

### `experience_validate` (`experience.h`)

**Bien (confirmado):** `requires \valid(st)` y rangos; lista de `\result`; P-1 (`!HR && !COORD ==> ERR_PERM`).

**H-7 — `assigns` sobre-aproximado (incorrecta). CONFIRMADO; causa del Failure.**
```acsl
assigns st->exps[0 .. st->exps_count-1].state,
        st->exps[0 .. st->exps_count-1].validator_id,
        st->log[st->log_count], st->log_count;
```
El código modifica **solo** `st->exps[idx]`. Como `idx` no es visible en la cabecera, el patrón recomendado es mantener el `assigns` por campo pero añadir los `ensures` de marco (H-8) que fijan qué índices quedan intactos; o introducir un `ghost` para el índice. **Cruce WP:** el Failure `typed_experience_validate_assigns_normal_part13` es exactamente la obligación de marco de este `assigns`; los Timeout `..._assigns_normal_part06/11/12` y `..._assigns_exit_part3/4/5` también derivan del rango completo sobre dos campos de todo el arreglo (explosión del contexto). **Clasificación:** *anotación incorrecta (demasiado amplia)*.

**H-8 — Faltan `ensures` de P-2, P-5 y del efecto (incorrecta/insuficiente). CONFIRMADO.**
El header solo deja P-2 como **comentario** (`// P-2: no autovalidacion ...`), sin `ensures`. Un comentario no es obligación de prueba. Sugerido:
```acsl
// P-2: autovalidación observable (owner == caller ==> ERR_SELF_VALIDATE)
ensures (\exists integer k; 0 <= k < \old(st->exps_count)
         && st->exps[k].id == exp_id && st->exps[k].owner_id == caller_id)
        ==> \result == ERR_SELF_VALIDATE;
// Efecto en éxito: la experiencia queda VALIDATED con validator = caller_id (!= owner)
ensures \result == EDU_OK ==>
        (\exists integer k; 0 <= k < st->exps_count
         && st->exps[k].id == exp_id
         && st->exps[k].state == EXP_VALIDATED
         && st->exps[k].validator_id == caller_id
         && st->exps[k].validator_id != st->exps[k].owner_id);
// P-5 / BR-5: no cambian fechas ni owner de ninguna experiencia
ensures \forall integer k; 0 <= k < \old(st->exps_count) ==>
        (st->exps[k].start_date == \old(st->exps[k].start_date)
         && st->exps[k].end_date   == \old(st->exps[k].end_date)
         && st->exps[k].owner_id   == \old(st->exps[k].owner_id));
// Inmutabilidad de lo ya validado
ensures \result == ERR_ALREADY_VALID ==>
        \forall integer k; 0 <= k < \old(st->exps_count) ==>
          st->exps[k].state == \old(st->exps[k].state);
```
**Cruce WP:** el Failure `typed_experience_validate_ensures` y el Timeout `..._ensures_2` corresponden a las postcondiciones existentes que no se cierran mientras el `assigns` siga sobre-aproximado; sin los `ensures` de P-2/P-5, esas propiedades directamente **no están especificadas**. **Clasificación:** P-2 y efecto = *falta anotación*; P-5 universal = *correcta pero necesita `assigns` acotado por `idx` y/o `state_inv`*.

### `experience_find` — sin contrato y **sin invariante de bucle (H-12). CONFIRMADO.**
```acsl
/*@ requires \valid_read(st);
    requires 0 <= st->exps_count <= MAX_EXPERIENCES;
    assigns \nothing;
    ensures -1 <= \result < st->exps_count;
    ensures \result >= 0 ==> st->exps[\result].id == exp_id;
    ensures \result == -1 ==> \forall integer k; 0 <= k < st->exps_count ==> st->exps[k].id != exp_id;
*/
```
y en el `.c` (análogo a `professional_find`):
```acsl
/*@ loop invariant 0 <= i <= st->exps_count;
    loop invariant \forall integer k; 0 <= k < i ==> st->exps[k].id != exp_id;
    loop assigns i;
    loop variant st->exps_count - i; */
```
**Cruce WP:** Timeouts `experience_find_assert_rte_*` y `_terminates` provienen de la falta de invariante/variant. **Clasificación:** *falta anotación*.

### `experience_validated_count` — **contrato + invariantes ya presentes** (corrige la mención previa a `experience_total_days`)
`requires \valid_read`, rangos de `exps_count` y `pros_count`, `assigns \nothing`, `ensures \result ∈ {ERR_BAD_ID, ERR_NOT_FOUND} || 0 <= \result <= exps_count`. El bucle tiene `loop invariant 0<=i<=exps_count`, `0<=count<=i`, `loop assigns i,count`, `loop variant`. Coherente con el código (cuenta VALIDATED del owner, sin aritmética de fechas → **no hay riesgo de overflow** que especificar; el hallazgo previo de "cota anti-overflow del `long total`" no aplica). **Refuerzo opcional:** un `ensures` relacionando `\result` con el número de índices VALIDATED del owner requeriría un predicado de conteo (`\numof`), hoy no necesario para cerrar RTE.

---

### `credentials` — mayoría **sin contrato (H-9). CONFIRMADO.**

Solo `credential_is_valid` tiene ACSL. `credential_issue`, `credential_activate`, `credential_block`, `credential_reactivate`, `credential_revoke` y `credential_status` no lo tienen. Deja P-6 (incompleta), P-7 y P-8 **sin especificar**.

**Cruce WP global de `credentials.c`:** 52/86, **34 Timeout, 0 Failure**. La ausencia total de Failure con tantos Timeout es coherente con "faltan contratos y faltan invariantes de bucle": sin `requires`/`loop invariant` las obligaciones RTE (`cred_find`, `cred_find_by_owner`, `cred_transition`, `credential_status`, `credential_issue`, `credential_revoke`) no se cierran por tiempo, pero no hay postcondiciones funcionales que *fallen* porque casi no hay postcondiciones escritas.

#### `credential_is_valid` — contrato **incompleto** (falta P-6)
Bien: `requires \valid_read`, `assigns \nothing`, `\result ∈ {0,1}`. Falta el bi-condicional de P-6 (hoy solo comentario):
```acsl
ensures \result == 1 <==>
        (\exists integer k; 0 <= k < st->creds_count
         && st->creds[k].id == cred_id && st->creds[k].state == CRED_ACTIVE);
```
**Cruce WP:** Timeout `typed_credential_is_valid_assigns_normal_part1` (RTE del `assigns`, ligado a que `credential_status` que invoca no tiene contrato ni invariante). **Clasificación:** *anotación correcta pero incompleta*.

#### `credential_status` — sin contrato y **sin invariante de bucle**
```acsl
/*@ requires \valid_read(st);
    requires 0 <= st->creds_count <= MAX_CREDENTIALS;
    assigns \nothing;
    ensures \result == -1 || \result == CRED_PENDING || \result == CRED_ACTIVE
         || \result == CRED_BLOCKED || \result == CRED_REVOKED;
    ensures \result >= 0 ==>
            (\exists integer k; 0 <= k < st->creds_count
             && st->creds[k].id == cred_id && st->creds[k].state == \result);
*/
```
+ invariantes de bucle análogos a `professional_find`. **Cruce WP:** Timeouts `credential_status_*` y `_terminates`. **Clasificación:** *falta anotación*.

#### `cred_transition` (static) — anotarla habilita a los cuatro mutadores
```acsl
/*@
  requires \valid(st);
  requires 0 <= st->creds_count <= MAX_CREDENTIALS;
  requires 0 <= st->log_count   <= MAX_EVENTS;
  assigns st->creds[0 .. st->creds_count-1].state,
          st->log[\old(st->log_count)], st->log_count;
  ensures \result == EDU_OK || \result == ERR_NULL || \result == ERR_PERM
       || \result == ERR_BAD_ID || \result == ERR_NOT_FOUND
       || \result == ERR_BAD_TRANSITION || \result == ERR_FULL;
  // P-7: en éxito, la credencial objetivo pasó a 'to'
  ensures \result == EDU_OK ==>
          (\exists integer k; 0 <= k < st->creds_count
           && st->creds[k].id == cred_id && st->creds[k].state == to);
  // las demás credenciales no cambian de estado
  ensures \forall integer k; 0 <= k < st->creds_count ==>
          (st->creds[k].id != cred_id ==> st->creds[k].state == \old(st->creds[k].state));
  // transición ilegal => sin cambio de estado
  ensures \result == ERR_BAD_TRANSITION ==>
          \forall integer k; 0 <= k < st->creds_count ==>
            st->creds[k].state == \old(st->creds[k].state);
*/
```
Nota: el `assigns` ideal sería solo `st->creds[idx].state`, pero `idx` no es visible; el patrón `creds[0..count-1].state` + los `ensures` de marco es el habitual. **Cruce WP:** Timeouts `cred_transition_assert_rte_*` y `..._call_audit_record_event_requires*` provienen de la falta de este contrato y de invariantes en `cred_find`.

#### `credential_issue` — sin contrato
```acsl
/*@
  requires \valid(st);
  requires 0 <= st->creds_count <= MAX_CREDENTIALS;
  requires 0 <= st->log_count   <= MAX_EVENTS;
  requires 0 <= st->pros_count  <= MAX_PROFESSIONALS;
  assigns st->creds[\old(st->creds_count)], st->creds_count,
          st->log[\old(st->log_count)], st->log_count;
  ensures \result == EDU_OK || \result == ERR_NULL || \result == ERR_PERM
       || \result == ERR_BAD_ID || \result == ERR_NOT_FOUND
       || \result == ERR_DUPLICATE || \result == ERR_FULL;
  ensures (caller_role != ROLE_COORD) ==> \result == ERR_PERM;                 // P-1
  ensures \result == EDU_OK ==>
          (st->creds[\old(st->creds_count)].id == cred_id
           && st->creds[\old(st->creds_count)].owner_id == owner_id
           && st->creds[\old(st->creds_count)].state == CRED_PENDING);
  ensures \result == EDU_OK ==> st->creds_count == \old(st->creds_count) + 1;
  ensures \result != EDU_OK ==> st->creds_count == \old(st->creds_count);       // BR-10
  ensures st->creds_count <= MAX_CREDENTIALS;
*/
```
"Una credencial por owner" (unicidad por `owner_id`, que el código garantiza con `cred_find_by_owner`) requiere **invariante global** (H-13). **Cruce WP:** Timeouts `credential_issue_assert_rte_*` y `_call_*_requires`. **Clasificación:** *falta anotación*.

#### `credential_revoke` — sin contrato; lógica propia (no usa `cred_transition`). Expresa P-8.
```acsl
/*@
  requires \valid(st);
  requires 0 <= st->creds_count <= MAX_CREDENTIALS;
  requires 0 <= st->log_count   <= MAX_EVENTS;
  assigns st->creds[0 .. st->creds_count-1].state,
          st->log[\old(st->log_count)], st->log_count;
  ensures \result == EDU_OK || \result == ERR_NULL || \result == ERR_PERM
       || \result == ERR_BAD_ID || \result == ERR_NOT_FOUND
       || \result == ERR_BAD_TRANSITION || \result == ERR_FULL;
  // P-8: REVOKED es terminal -> revocar lo ya revocado falla
  ensures (\exists integer k; 0 <= k < st->creds_count
           && st->creds[k].id == cred_id
           && \old(st->creds[k].state) == CRED_REVOKED) ==> \result == ERR_BAD_TRANSITION;
  ensures \result == EDU_OK ==>
          (\exists integer k; 0 <= k < st->creds_count
           && st->creds[k].id == cred_id && st->creds[k].state == CRED_REVOKED);
*/
```
`credential_activate/block/reactivate` solo necesitan, además del contrato de `cred_transition`, un `ensures` que fije su par `from`/`to`. **Cruce WP:** Timeouts `credential_revoke_*`. **Clasificación:** *falta anotación*; terminalidad a lo largo de varias operaciones = *necesita invariante global*.

---

### `main.c` y `verification/eva_harness.c` — **correcto no anotarlos**
`main.c` es demo fuera del alcance de WP; `eva_harness.c` es para Eva (análisis de valores), no WP. La salida de Eva no es demostración WP.

---

## 4. Hallazgos transversales

### H-12 — Bucles sin invariantes (refinado)
**Ya tienen** `loop invariant/assigns/variant`: `professional_find`, `audit_verify_event`, `experience_validated_count`.
**Siguen sin invariantes:** `experience_find`, `credential_status`, `cred_find`, `cred_find_by_owner`.
Patrón para los de búsqueda (ejemplo `cred_find`):
```acsl
/*@ loop invariant 0 <= i <= st->creds_count;
    loop invariant \forall integer k; 0 <= k < i ==> st->creds[k].id != cred_id;
    loop assigns i;
    loop variant st->creds_count - i; */
```
**Por qué WP lo necesita / cruce:** la mayoría de los Timeout de `credentials.c` y los de `experience_find` son RTE (`index_bound`, `mem_access`) y `terminates` en estos bucles; sin `loop invariant` que acote `i` y sin `loop variant`, el prover no cierra ni el acceso seguro ni la terminación. **Clasificación:** *falta anotación*.

### H-13 — Base ACSL compartida vacía (`specifications.h`, `invariants.h`)
Ambos siguen siendo stubs en prosa. Se necesitan:
```acsl
/*@ predicate valid_id(integer x)   = ID_MIN   <= x <= ID_MAX; */
/*@ predicate valid_date(integer d) = DATE_MIN <= d <= DATE_MAX; */
/*@ predicate state_inv(educhain_state_t *st) =
      0 <= st->pros_count  <= MAX_PROFESSIONALS &&
      0 <= st->exps_count  <= MAX_EXPERIENCES   &&
      0 <= st->creds_count <= MAX_CREDENTIALS   &&
      0 <= st->log_count   <= MAX_EVENTS        &&
      (\forall integer a,b; 0 <= a < b < st->pros_count  ==> st->pros[a].id != st->pros[b].id)  &&
      (\forall integer a,b; 0 <= a < b < st->creds_count ==> st->creds[a].owner_id != st->creds[b].owner_id) &&
      (\forall integer a;   0 < a < st->log_count        ==> st->log[a-1].seq < st->log[a].seq);
*/
```
Uso previsto: `requires state_inv(st); ensures state_inv(st);` en cada mutador, para demostrar **preservación**. **Clasificación:** *falta anotación (infraestructura)* → habilita P-3, P-8, P-12 y "una credencial por owner".

### H-14 — Propiedades que necesitan invariante global y/o lemas
P-3 (unicidad), "una credencial por owner", P-12 (seq global), P-5 (inmutabilidad universal), P-8 (terminalidad a través de secuencias) son **invariantes de estado**, no postcondiciones puntuales. Requieren `state_inv` (H-13) y, en algún caso, lemas. **Clasificación:** *correctas pero WP no las cierra sin la infraestructura de H-13.*

---

## 5. Cruce Failure/Timeout (wp_results.txt) → anotación ACSL probable causa

| Obligación WP (archivo) | Tipo | Anotación ACSL que probablemente lo causa |
|---|---|---|
| `professional_register_assigns_normal_part13` | Failure | `assigns` sobre-aproximado `pros[0..63]`/`log[0..1023]` (H-5) |
| `professional_register_ensures`, `_2`, `_3`, `_4` | Failure | derivado de H-5: con el marco amplio WP no deduce que el resto del estado no cambió |
| `experience_validate_assigns_normal_part13` | Failure | `assigns` sobre-aproximado `exps[0..n-1].state/.validator_id` (H-7) |
| `experience_validate_ensures` | Failure | postcondición no cerrable con `assigns` amplio + faltan `ensures` P-2/P-5 (H-7/H-8) |
| `experience_validate_assigns_normal_part06/11/12`, `_assigns_exit_part3/4/5`, `_ensures_2` | Timeout | explosión de contexto por el `assigns` de dos campos sobre todo el arreglo (H-7) |
| `experience_register_assert_rte_*`, `_call_professional_is_active_requires*`, `_call_audit_record_event_requires_2` | Timeout | `experience_register` sin contrato: faltan `requires` para descargar precondiciones de llamadas (H-6) |
| `experience_find_assert_rte_*`, `_terminates` | Timeout | `experience_find` sin invariante/variant de bucle (H-12) |
| `audit_verify_event_*` (6 goals), `audit_get_count_assert_rte_mem_access` | Timeout | RTE/terminación; spec correcta, candidato a mayor `-wp-timeout` o invariante de cota más explícito |
| `credential_status_*`, `cred_find_*`, `cred_find_by_owner_*`, `cred_transition_*`, `credential_issue_*`, `credential_revoke_*`, `credential_is_valid_assigns_normal_part1` (34 Timeout) | Timeout | falta de contratos (H-9) y de invariantes de bucle (H-12) en credentials |
| `security.c` 69/69 | — | ninguna; módulo sin defectos de spec |

**Lectura:** los **Failures** se concentran donde el `assigns` está sobre-aproximado (H-5, H-7) — son los dos arreglos prioritarios. Los **Timeouts** se concentran donde faltan contratos (H-6, H-9) o invariantes de bucle (H-12). Ninguna de las dos categorías refuta una propiedad; indican especificación incorrecta (Failure) o incompleta/costosa (Timeout).

---

## 6. Tabla de trazabilidad propiedad → estado de la ANOTACIÓN (no de la prueba)

| Prop. | Dónde debería vivir | Estado de la anotación (verificado en esta ronda) |
|------|----------------------|-----------------------------------|
| P-1 (ERR_PERM por rol) | register_pro ✓, validate ✓; falta en issue/activate/block/reactivate/revoke y register_exp | Parcial: *falta* en credentials y experience_register |
| P-2 (no autovalidación) | experience_validate | *Falta anotación* (solo comentario, sin `ensures`) |
| P-3 (unicidad) | register_pro + invariante global | *Falta* invariante global (H-13) |
| P-4 (fechas válidas) | experience_register | *Falta anotación* (sin contrato) |
| P-5 (inmutabilidad validada) | experience_validate | *Falta* `ensures`; requiere `assigns` acotado/invariante |
| P-6 (válida⇔ACTIVE) | credential_is_valid | *Incompleta*: falta el `<==>` |
| P-7 (transición legal) | cred_transition/revoke | *Falta anotación* |
| P-8 (REVOKED terminal) | credential_revoke + invariante | *Falta anotación* + invariante |
| P-9 (acceso en rango) | requires de contadores | Presente donde hay contrato; falta donde no hay contrato |
| P-10 (sin overflow) | audit (seq) ✓ | `audit_record_event` **ya** acota `seq` (`last>=2e9 ==> ERR_FULL`); `experience_validated_count` no hace aritmética → no aplica cota del informe previo |
| P-11 (assigns vacío en lectura) | consultas | Presente en find/is_active/validated_count/verify_event/get_count/is_valid; *falta* en experience_find y credential_status |
| P-12 (log monótono) | audit + invariante | `ensures` locales **completos** (seq+1 y caso 0); monotonía global *necesita* invariante (H-13) |
| P-13 (ausencia UB) | Eva | Fuera del alcance de WP; harness presente |

---

## 7. Prioridades recomendadas (Fase 4)

1. **Acotar los `assigns` sobre-aproximados** de `professional_register` (H-5) y `experience_validate` (H-7). Son la causa directa de los **7 Failures** de WP; es el cambio de mayor impacto.
2. Definir `state_inv`, `valid_id`, `valid_date` en `verification/*.h` (H-13). Habilita P-3, P-8, P-12 y "una credencial por owner".
3. Escribir los contratos faltantes: `experience_register` (H-6), todas las de `credentials` + `cred_transition` (H-9), `experience_find` y `credential_status` (H-12/consultas).
4. Añadir `loop invariant/assigns/variant` a `experience_find`, `credential_status`, `cred_find`, `cred_find_by_owner` (H-12).
5. Completar P-6 (`<==>`) en `credential_is_valid` y añadir `ensures` de P-2/P-5 en `experience_validate` (H-8).
6. Para los Timeouts puramente RTE/terminación ya con invariante (p. ej. `audit_verify_event`, `professional_find`), probar mayor `-wp-timeout` o invariantes de cota más explícitos antes de concluir que falta especificación.

## 8. Recordatorio final

Ninguna observación de este informe afirma que algo esté *demostrado* o *refutado* por WP. Describen el estado de la **especificación** frente al **código real**, apoyadas en los `Failure`/`Timeout`/`Proved` que reporta `verification/wp_results.txt` (corrida con Qed + Alt-Ergo; **Z3 no cerró ninguna obligación** en esa salida). Para cerrar el ciclo honestamente: corregir primero los `assigns` (prioridad 1), reejecutar `frama-c -wp -wp-report` y comparar el nuevo conteo de goals probados.
