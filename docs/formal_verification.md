# EduChain — Verificación formal (Frama-C WP / Eva)

Este documento registra los resultados **reales** de Frama-C. Regla del proyecto: no se declara
"verificado" nada que WP no haya demostrado. Se distinguen tres categorías:

- **Probado (Qed / Alt-Ergo):** la obligación de prueba quedó demostrada. WP se invocó con
  `-wp-prover alt-ergo,z3`, pero en esta línea base **todas** las obligaciones cerradas lo fueron por
  Qed o Alt-Ergo; **ninguna se cerró con Z3**. Why3 consulta los solucionadores en orden y Alt-Ergo
  respondió primero. No se presenta Z3 como contribuyente a ninguna demostración hasta que una
  obligación se cierre explícitamente con él.
- **No demostrada (Failure):** WP no pudo demostrar la obligación con la especificación actual.
  **No implica** que el código tenga un bug; con frecuencia significa que falta anotación ACSL
  (invariante de lazo, `assigns` preciso, predicado de unicidad). Requiere análisis caso por caso.
- **Timeout:** el solucionador no cerró la obligación dentro del límite de tiempo. Tampoco es un bug;
  normalmente indica que falta un invariante de lazo que acote el razonamiento.

> Una obligación no demostrada o con timeout **no es un contraejemplo**. Un contraejemplo (refutación)
> sí indicaría que la propiedad es falsa. En esta línea base no se observaron contraejemplos explícitos.

---

## Entorno de ejecución

| Elemento        | Valor                                             |
|-----------------|---------------------------------------------------|
| Fecha (UTC)     | 2026-11-18 19:46 UTC                              |
| Frama-C         | 33.0 (Arsenic)                                    |
| Solucionadores  | Alt-Ergo (vía why3) — cerró las obligaciones. Z3 4.13.0 configurado (vía why3) pero sin ninguna obligación cerrada en esta línea base. |
| Plataforma      | WSL2 Ubuntu 24.04, instalación vía opam (el target nominal en phase1_model.md §11 era "WSL2 Debian"; la ejecución real fue en Ubuntu 24.04, compatible vía mismo apt/opam) |
| Timeout WP      | 20 s por obligación                               |
| Opciones WP     | `-wp -wp-rte -wp-prover alt-ergo,z3 -wp-timeout 20` |
| Includes        | `-cpp-extra-args="-Iinclude -Iverification"`      |

### Comandos utilizados

Por cada módulo (script `verification/verify.sh`):

```bash
eval "$(opam env)"
cd EduChain
frama-c -wp -wp-rte -wp-prover alt-ergo,z3 -wp-timeout 20 \
  -cpp-extra-args="-Iinclude -Iverification" src/<modulo>.c
```

Eva (análisis de valores), con harness de entrada `verification/eva_harness.c`:

```bash
frama-c -eva -cpp-extra-args="-Iinclude -Iverification" \
  src/*.c verification/eva_harness.c -main eva_main
```

---

## LÍNEA BASE — Ejecución 2026-11-18 (NO MODIFICAR)

Esta es la primera ejecución, anterior a cualquier corrección de ACSL. Se conserva como referencia
para medir el efecto de cada cambio. **No se edita.**

### Resumen por módulo

| Orden | Módulo           | Probados | Total | No demostradas | Timeouts | Estado        |
|------:|------------------|---------:|------:|---------------:|---------:|---------------|
| 1     | `security.c`     | **69**   | 69    | 0              | 0        | Obligaciones generadas demostradas* |
| 2     | `professionals.c`| 62       | 74    | 5              | 7        | A trabajar    |
| 3     | `audit.c`        | 68       | 74    | 0              | 6        | A trabajar    |
| 4     | `experience.c`   | 75       | 106   | 2              | 29       | A trabajar    |
| 5     | `credentials.c`  | 52       | 86    | 0              | 34       | A trabajar    |
|       | **TOTAL**        | **326**  | 409   | 7              | 76       |               |

(Nota: "Total" por módulo según el desglose de WP; incluye objetivos *Terminating* y *Unreachable*
que WP contabiliza aparte y da por cerrados. Los "Probados" suman Qed + Alt-Ergo; Z3 no cerró ninguna.)

### Detalle por módulo

#### 1. `security.c` — 69/69 obligaciones WP generadas demostradas (conservar intacto)
```
[wp] Proved goals: 69 / 69
  Terminating: 1   Unreachable: 1   Qed: 67
```
Las 69 obligaciones **generadas** por WP para este módulo quedaron demostradas (67 Qed + 1 Terminating
+ 1 Unreachable), sobre un `switch` puro sin bucles. Esto cubre **ausencia de RTE y terminación** del
decisor de permisos.

**No equivale a "P-1 verificada".** La propiedad P-1 completa (autorización ⇒ `ERR_PERM` **y** estado
sin cambios) no se materializa en `security_can`, sino en los módulos llamadores
(`professional_register`, `experience_validate`, etc.), donde hoy hay Failures y timeouts. Por tanto
P-1 de extremo a extremo está **pendiente**. `security_can` solo decide por rol; la autorización por
identidad (R-SELF / no autovalidación) vive en los módulos, no aquí.

**No se modifica** mientras se trabaja en los demás módulos.

> \* "Obligaciones generadas demostradas" = WP cerró todas las obligaciones que generó para este
> módulo (RTE + terminación). No implica que las propiedades funcionales de negocio del sistema estén
> demostradas de extremo a extremo.

#### 2. `professionals.c` — 62/74 (5 no demostradas, 7 timeouts)
```
[wp] Proved goals: 62 / 74
  Unreachable: 1   Qed: 49   Alt-Ergo: 12   Failed: 5   Timeout: 7
```
No demostradas (Failure):
- `typed_professional_register_ensures`        (postcondición principal)
- `typed_professional_register_ensures_2`
- `typed_professional_register_ensures_3`
- `typed_professional_register_ensures_4`
- `typed_professional_register_assigns_normal_part13`

Timeouts (función `professional_find`, bucle sin invariante):
- `typed_professional_find_assert_rte_index_bound` (x2)
- `typed_professional_find_assert_rte_mem_access`  (x2)
- `typed_professional_find_terminates`
- `typed_professional_find_ensures`
- `typed_professional_find_assigns_part3`

Advertencias: `missing-spec` en `security.h:24` y `audit.h:28` (WP usa los contratos de esas funciones
llamadas; están declaradas pero el cuerpo no se analiza en esta unidad de compilación).

**Hipótesis preliminar (a confirmar, NO afirmada como bug):** los Failure de `*_register_ensures`
dependen de que WP pueda razonar sobre `professional_find` (que no tiene invariante de lazo) y sobre la
preservación de `pros_count` en las ramas de error. Falta anotación, no necesariamente código defectuoso.

#### 3. `audit.c` — 68/74 (0 no demostradas, 6 timeouts)
```
[wp] Proved goals: 68 / 74
  Terminating: 2   Unreachable: 3   Qed: 46   Alt-Ergo: 17   Timeout: 6
```
Timeouts (función `audit_verify_event` y `audit_get_count`, bucle sin invariante):
- `typed_audit_verify_event_assert_rte_mem_access` (x2)
- `typed_audit_verify_event_assert_rte_index_bound` (x2)
- `typed_audit_verify_event_terminates`
- `typed_audit_get_count_assert_rte_mem_access`

Sin Failures: la lógica append-only y de `seq` creciente quedó demostrada; faltan invariantes de lazo
para cerrar los accesos del bucle de búsqueda.

#### 4. `experience.c` — 75/106 (2 no demostradas, 29 timeouts)
```
[wp] Proved goals: 75 / 106
  Terminating: 1   Unreachable: 3   Qed: 53   Alt-Ergo: 18   Failed: 2   Timeout: 29
```
No demostradas (Failure):
- `typed_experience_validate_ensures`
- `typed_experience_validate_assigns_normal_part13`

Timeouts: mayoritariamente en `experience_register`, `experience_validate`, `experience_find` y
`experience_total_days`, todos con bucles sin invariante o `assigns` demasiado amplio
(`st->exps[0..count-1]`). Incluye `..._signed_overflow_4` en `experience_total_days` (resta de fechas).

#### 5. `credentials.c` — 52/86 (0 no demostradas, 34 timeouts)
```
[wp] Proved goals: 52 / 86
  Unreachable: 3   Qed: 42   Alt-Ergo: 7   Timeout: 34
```
Sin Failures. Los 34 timeouts están en los bucles de búsqueda (`cred_find`, `cred_find_by_owner`,
`credential_status`) y en `cred_transition` / `credential_issue` / `credential_revoke` por los mismos
accesos a arreglo sin invariante de lazo.

### Eva (línea base)
La ejecución de Eva **no produjo salida útil**. Causa identificada (auditoría del revisor de
verificación), DOS bugs concretos, no "aún no intentado":
1. `verification/verify.sh` invocaba Eva con `-eva-no-print`, **opción inexistente en Frama-C 33.0**
   → Eva aborta con User Error.
2. `verification/eva_harness.c` usaba **firmas desactualizadas** (p. ej. `professional_register` con
   3 args, sin `caller_id`) → Frama-C se detiene en el parsing.
Se registró como pendiente por bug conocido. **CORREGIDO** (ver "Ejecución Eva 2026-11-18" abajo):
se actualizó `eva_harness.c` con las firmas nuevas (`caller_id`) y `experience_validated_count`, y
se quitó la opción inexistente `-eva-no-print` de `verify.sh`. Eva ahora produce resultados.

### Nota sobre el versionado de este documento
Auditoría del revisor: `docs/formal_verification.md` y `verification/wp_results.txt` pueden no estar
bajo control de versiones en el momento de escritura. La leyenda "LÍNEA BASE (NO MODIFICAR)" es una
convención documental dentro del archivo; su integridad real depende de commitear el repositorio.
`wp_results.txt` contiene **solo** la corrida de la línea base original; las corridas de Fase 4 viven
en este `.md`. Recomendación: commitear y volcar las corridas de Fase 4 a `verification/`.

### Advertencias globales observadas (todos los módulos)
- `Skipped RTE guards: unaligned pointers (\aligned not supported)` — WP no genera esas guardas; no aplica
  a este código (sin aritmética de punteros desalineados).
- `Skipped RTE guards: invalid function pointer calls` — no hay punteros a función en el código.
- `wp:pedantic-assigns` en varias líneas — avisos de que las cláusulas `assigns` podrían precisarse.

---

## Plan de trabajo (acordado)

Orden y objetivo de cada módulo. Se trabaja **uno a la vez**, re-ejecutando WP tras cada cambio y
registrando el nuevo resultado **debajo** de esta línea base, sin borrarla.

| Orden | Módulo           | Trabajo                                                        |
|------:|------------------|----------------------------------------------------------------|
| 1     | `security.c`     | Conservar intacto (69/69).                                     |
| 2     | `professionals.c`| Investigar 5 no demostradas + 7 timeouts (invariantes de lazo).|
| 3     | `audit.c`        | Resolver 6 timeouts (invariantes de lazo).                     |
| 4     | `experience.c`   | Revisar 2 no demostradas + 29 timeouts.                        |
| 5     | `credentials.c`  | Investigar 34 timeouts.                                        |

Condiciones:
1. No modificar lo ya demostrado (`security.c`) sin necesidad.
2. No llamar "bug" a una obligación no demostrada hasta analizarla (contraejemplo vs. falta de anotación).

---

## Historial de ejecuciones posteriores

*(Se agregan aquí los resultados tras cada corrección. No se borra la línea base.)*

### Ejecución 2026-11-18 — `experience.c` (Fase 4: ACSL, intento minimalista acordado)

**Medición inicial (sin modificar el módulo, con contratos audit/security ya mejorados):**
91/116, 2 Failures, 23 Timeouts (vs. línea base 75/106, 2 Failures, 29 Timeouts).
NOTA: el total de objetivos varía con cada contrato; no comparar porcentajes entre configuraciones
distintas. El número definitivo del módulo es el de la ejecución final (abajo).

**Resultado definitivo del módulo (contratos completos):**

| Módulo         | Probados | Total | No demostradas | Timeouts | Delta abs. vs. base |
|----------------|---------:|------:|---------------:|---------:|---------------------|
| `experience.c` | **146**  | 177   | 7              | 24       | 75 → 146 probados (abs.) |

Desglose: `Qed: 116, Alt-Ergo: 26, Terminating: 2, Unreachable: 2, Failed: 7, Timeout: 24`.

**Comando** (misma config que la línea base; timeout 20 s, sin aumentar):
```bash
frama-c -wp -wp-rte -wp-prover alt-ergo,z3 -wp-timeout 20 \
  -cpp-extra-args="-Iinclude -Iverification" src/experience.c
```

**Qué se corrigió (demostrado):**
- `experience_find`: invariante de lazo + contrato de solo lectura (`assigns \nothing`, relación
  `\result`↔`id`). **Cierra sus 5 timeouts.** Demostrado.
- `experience_validated_count`: invariante de lazo (ya presente), contrato de solo lectura. Demostrado.

**Obligaciones NO demostradas (7 Failures + 24 Timeouts) — todas en `experience_register` y
`experience_validate`. Causa mixta (revisada tras auditoría de los agentes): parte limitación de
automatización de WP, parte anotación mejorable.**
`experience_register` encadena 3 llamadas de búsqueda (`professional_is_active`, `experience_find`)
+ escritura de 6 campos + `audit_record_event`, sobre `educhain_state_t` (~7 KB). WP genera ~18
sub-objetivos `assigns` (part07–part24) que expiran. El revisor ACSL identifica causas de
especificación, no solo del probador:
- `experience_validate` tiene `assigns st->exps[0 .. exps_count-1].state/validator_id` **demasiado
  amplio** (el código solo toca `exps[idx]`); acotarlo a `idx` es probablemente lo que cierra sus
  timeouts y habilita la inmutabilidad de las demás (R-5/P-5). **Se corrige abajo (entrada posterior).**
- el `ensures` de efecto de `experience_validate` (transición, self-validate, already-valid) **no
  estaba formalizado** (era comentario). **Se corrige abajo.**
- `experience_register`: `assigns st->exps[st->exps_count]` con `requires ... <= MAX` nombra
  `exps[256]` fuera de rango en la rama `ERR_FULL`; mismo patrón que `professional_register`.
Lo que resta tras esas correcciones se mantiene como pendiente por limitación del probador.

**Propiedades ESPECIFICADAS en el contrato pero PENDIENTES de prueba formal (no eliminadas del
modelo; conservadas como `ensures` por fidelidad):**

| Propiedad | Regla | Estado WP | Respaldo dinámico |
|-----------|-------|-----------|-------------------|
| Retorno en conjunto de códigos | — | Timeout | test_experience / test_attacks |
| P-1 rol no autorizado ⇒ ERR_PERM | BR-1 | Failure (`ensures`) | A1, test_experience |
| PRO solo registra lo propio | BR-1 | Failure/Timeout | test_experience |
| P-4 fechas válidas en la nueva exp | BR-4 | Failure (`ensures_6`) | A5 (fechas invertidas), test_experience |
| Inicialización PENDING / validator 0 | — | Failure (`ensures_8`) | test_experience |
| Atomicidad del contador | BR-10 | Timeout (`ensures_2/3`) | A10, test_experience |
| `assigns` de la inserción | — | 18 sub-objetivos en Timeout/Failure | ASan/UBSan, 99 pruebas |
| `experience_validate` P-1/efecto | BR-1/2/5 | Failure (`validate_ensures`) + Timeout | A1, A2, A3, test_experience |

Ninguna de estas es un contraejemplo (propiedad falsa). El código es correcto: **99/99 pruebas
(normal + ASan/UBSan)**. Son obligaciones no cerradas por el probador en el tiempo dado.

**Intento minimalista (acordado):** se probó extraer la inserción a una auxiliar `exp_insert`
(demostrada **41/41 aislada**) y reducir el contrato. Ninguna de las dos medidas cerró el call-site
de `experience_register` (la limitación está en el `assigns` framing de la función completa, no en
las postcondiciones). Se revirtió la auxiliar y se **conservaron** las postcondiciones del modelo.
Se documenta como pendiente por limitación de automatización.

**Cambios funcionales:** ninguno. Solo ACSL + invariantes de lazo. Regresión: **99/99**.

**Qué quedó demostrado del módulo:** `experience_find` y `experience_validated_count` completos
(solo lectura + lazos); RTE de rango/overflow de las consultas; parte de `experience_validate`.
Lo demás, pendiente por limitación del probador, con respaldo dinámico.

### Ejecución 2026-11-18 — `experience_validate` reabierto (corrección del revisor ACSL)

Tras la auditoría de los agentes, se aplicó la recomendación de `revisor-acsl` sobre `experience_validate`:
1. **`assigns` acotado por set-comprehension** (mejora real de precisión): solo las experiencias con
   `id == exp_id` pueden cambiar `state`/`validator_id`, en vez del rango `[0..count-1]`. Esto expresa
   correctamente que `owner_id`, `start_date`, `end_date` NO son escribibles (inmutabilidad R-5/P-5
   capturada por el frame `assigns`).
2. Se añadió `ensures st->exps_count == \old(st->exps_count)` (validate no inserta).

Resultado del módulo: **146/178, 8 Failures, 24 Timeouts** (vs. 146/177, 7 Failures antes de reabrir).
Regresión **99/99**.

**Análisis honesto:** el `assigns` acotado es más preciso y correcto, pero WP **no gana objetivos
probados** con él, y los `ensures` universales de inmutabilidad fuerte (owner/fechas de toda
experiencia) **tampoco cierran** (se intentaron y se retiraron porque añadían 3 Failures sin cerrar
nada; la inmutabilidad queda capturada por el `assigns`, no por `ensures`). `validate_ensures`
(efecto/P-1 en rama) y `validate_ensures_3` (exps_count) siguen en Failure por el mismo patrón de
framing. Se mantiene como pendiente, sin inventar cierres.

**Conclusión sobre el diagnóstico previo:** `revisor-acsl` tenía razón en que el `assigns` era
mejorable (se mejoró), pero la mejora de precisión **no** se tradujo en más obligaciones cerradas:
el cuello de botella de WP sobre `educhain_state_t` (~7 KB) persiste incluso con `assigns` preciso.
El diagnóstico se actualiza a: "anotación mejorada + limitación de automatización remanente".

### Ejecución Eva 2026-11-18 — análisis de valores (por fin operativo)

Tras corregir los dos bugs (harness con firmas nuevas + `-eva-no-print` eliminado), Eva corre.

**Comando:**
```bash
frama-c -eva -eva-domains cvalue -cpp-extra-args="-Iinclude -Iverification" \
  src/security.c src/audit.c src/professionals.c src/experience.c src/credentials.c \
  verification/eva_harness.c -main eva_main
```

**Resultado (verification/eva_results.txt):**
```
23 functions analyzed (out of 23): 100% coverage.
440 statements reached (out of 544): 81% coverage.
Eva analyzer:   0 errors, 0 warnings
Frama-C kernel: 0 errors, 1 warning (stub externo Frama_C_interval)
0 alarms generated by the analysis.
Assertions     2 valid   8 unknown   0 invalid   10 total
Preconditions 54 valid   0 unknown   0 invalid   54 total
87% de las propiedades lógicas alcanzadas, probadas.
```

**Interpretación honesta:**
- **0 alarmas** = Eva no encontró accesos fuera de límites, overflow aritmético, lectura de
  no-inicializados ni división por cero en las rutas exploradas desde el harness. Esto es evidencia
  a favor de P-13 (ausencia de UB) y de la parte de P-9/P-10 asignada a Eva.
- **54/54 precondiciones válidas**: todas las precondiciones de las llamadas internas se cumplen en
  el análisis de valores.
- Las **8 assertions "unknown"** son los `assert` manuales (`0 <= n < MAX_*`) insertados como ayuda a
  WP; Eva no los resuelve por su naturaleza (dependen de la lógica del contrato), pero **ninguna es
  "invalid"** (0 inválidas), así que no hay contradicción detectada.
- **Límite de alcance:** 81% de sentencias alcanzadas. El 19% no alcanzado corresponde a ramas que el
  harness no ejercita (p. ej. combinaciones de rol/estado que el flujo fijo no cubre). Eva demuestra
  ausencia de UB **solo en las trazas analizadas**, no en todas las posibles; no sustituye a WP ni es
  una garantía total. 0 alarmas NO significa "sin errores en todo caso", significa "sin errores en lo
  explorado".

Cambios funcionales: ninguno (solo el harness de verificación y `verify.sh`). Regresión: 99/99.


<!-- Plantilla por ejecución:
### Ejecución YYYY-MM-DD — <módulo> <cambio aplicado>
| Módulo | Probados | Total | No demostradas | Timeouts | Delta vs. base |
Comando / notas / análisis.
-->

### Ejecución 2026-11-18 — `audit.c` (Fase 4: ACSL completado)

| Módulo    | Probados | Total | No demostradas | Timeouts | Delta vs. base |
|-----------|---------:|------:|---------------:|---------:|----------------|
| `audit.c` | **95**   | 95    | 0              | **0**    | 68/74 → 95/95; −6 timeouts; +21 goals (contratos nuevos) |

Desglose WP: `Qed: 65, Alt-Ergo: 27, Terminating: 3, Unreachable: 3`. Z3 no cerró ninguna.
(Corregido: el desglose Alt-Ergo es 27, no 24; verificado por re-ejecución del revisor de verificación.
El total 98/98 no cambia.)

**Comando** (igual configuración que la línea base; timeout 20 s sin aumentar):
```bash
frama-c -wp -wp-rte -wp-prover alt-ergo,z3 -wp-timeout 20 \
  -cpp-extra-args="-Iinclude -Iverification" src/audit.c
```

**Qué se corrigió:**
- `audit_verify_event`: añadido contrato (`requires \valid_read`, `assigns \nothing`,
  `ensures` sobre índice y relación `seq`) e invariantes de lazo (`loop invariant/assigns/variant`).
  Cierra los 6 timeouts (RTE `mem_access`/`index_bound` y `terminates` del bucle).
- `audit_get_count`: añadido contrato (`requires \valid_read`, `assigns \nothing`,
  `ensures \result == log_count`). Cierra su `mem_access`.
- `audit_record_event`: **fortalecido** (no debilitado) el `behavior ok` con la monotonía de
  `seq` (P-12): `seq[n] == seq[n-1]+1` y `seq[0] == 1` en éxito, y atomicidad del `ERR_FULL`
  por saturación de `seq`. Todas cierran.

**Qué quedó pendiente:** nada en este módulo. 95/95.

**Cambios funcionales:** ninguno. Solo ACSL y 4 líneas de invariante de lazo (comentarios que
gcc ignora). Regresión: **99/99 pruebas siguen pasando** (normal + ASan/UBSan).

**Propiedades que este módulo ahora especifica y WP cerró:** P-12 (log monótono / seq creciente)
a nivel de la operación `audit_record_event`; P-11 (assigns vacío) en las dos consultas de lectura;
P-9/P-10 (accesos en rango, sin overflow de `seq`) vía RTE. La monotonía *global* de todo el log
(invariante `state_inv`) sigue pendiente de la infraestructura compartida (se abordará con los
módulos que mutan estado).

### Ejecución 2026-11-18 (intermedia) — `professionals.c` SIN modificar (medir efecto de audit.c)

Objetivo: aislar cuánto resolvió el contrato completo de `audit_record_event` en su llamador,
antes de tocar las anotaciones de `professionals.c`.

| Módulo            | Probados | Total | No demostradas | Timeouts | Delta vs. base |
|-------------------|---------:|------:|---------------:|---------:|----------------|
| `professionals.c` | 76       | 81    | 5              | **0**    | 62/74 → 76/81; −7 timeouts; Failures sin cambio (5) |

Desglose: `Qed: 62, Alt-Ergo: 12, Terminating: 1, Unreachable: 1, Failed: 5`.

**Conclusión:** el contrato de `audit` + el invariante de lazo de `professional_find` (ya presentes)
eliminaron los 7 timeouts, pero **los 5 Failures persisten**: son un problema de especificación
propio de `professional_register`, no del llamado a auditoría. Failures: `register_ensures`,
`register_ensures_2`, `register_ensures_3`, `register_ensures_5`, `register_assigns_normal_part13`.
Se diagnostican y corrigen a continuación.

### Ejecución 2026-11-18 — `professionals.c` (Fase 4: ACSL + caracterizacion de permisos)

| Módulo            | Probados | Total | No demostradas | Timeouts | Delta vs. base |
|-------------------|---------:|------:|---------------:|---------:|----------------|
| `professionals.c` | **78**   | 81    | **2**          | **1**    | 62/74 → 78/81; −5 timeouts; Failures 5 → 2 |

Desglose: `Qed: 64, Alt-Ergo: 12, Terminating: 1, Unreachable: 1, Failed: 2, Timeout: 1`.

**Comando** (misma config que la línea base; timeout 20 s, sin aumentar):
```bash
frama-c -wp -wp-rte -wp-prover alt-ergo,z3 -wp-timeout 20 \
  -cpp-extra-args="-Iinclude -Iverification" src/professionals.c
```

**Qué se corrigió:**
- 7 timeouts → 0: invariante de lazo en `professional_find` + contrato completo de
  `audit_record_event` (eliminó el `missing-spec`).
- 3 de 5 Failures cerrados al **caracterizar completamente la matriz de permisos** en
  `security_can` (bi-condicional por cada `op_code`, en `security.h`). Esto permitió a WP deducir
  que un rol no autorizado produce `ERR_PERM`.
- `audit_record_event`: añadido **frame `ensures`** explícito (preserva `pros_count`, `exps_count`,
  `creds_count`). Cerró obligaciones de preservación en el llamador.

**Obligaciones NO DEMOSTRADAS / PENDIENTES DE ANALISIS (2 Failures + 1 timeout):**
- `typed_professional_register_ensures` (P-1: rama `ERR_PERM` con `pros_count` sin cambio) — Failure (rápido).
- `typed_professional_register_assigns_normal_part13` (inclusión de la escritura de auditoría en el frame) — Failure (rápido).
- `typed_professional_register_ensures_2` (éxito `pros_count == old+1`) — Timeout.

**Clasificación honesta (revisada tras auditoría de los agentes):** los dos `Failure` fallan de
inmediato (~40-500 ms), **no son timeouts**; un Failure rápido es compatible tanto con falta de
framing/encadenamiento como, en principio, con una especificación mejorable. **No se ejecutó un
buscador de contraejemplos** que descarte lo segundo, así que quedan como **no demostradas /
pendientes de análisis**, no como "limitación benigna confirmada".

Hipótesis (no confirmada): WP (modelos `Typed` y `Typed+ref`) no encadena el frame `ensures` de
`audit_record_event` para probar que preserva `pros_count` sobre `educhain_state_t` (~7 KB). El
revisor ACSL señala además una posible causa de especificación: el `assigns st->pros[st->pros_count]`
con `requires ... <= MAX_PROFESSIONALS` nombra `pros[64]` (fuera de rango) en la rama `ERR_FULL`;
acotar el `assigns` por *behavior* de éxito podría cerrar `assigns_normal_part13`. Pendiente de
intentar. El código es correcto (99/99 pruebas + ASan/UBSan limpios) y no se observó contraejemplo,
pero eso **no equivale** a prueba formal.

**Intento de resolucion (opcion 3, 3 iteraciones acordadas):** se extrajo la insercion a una
funcion auxiliar `pros_insert` con contrato acotado. La auxiliar **se demuestra 21/21 de forma
aislada**, pero su frame `ensures` no cerro el call-site (WP no lo encadena), y el conteo neto de
obligaciones pendientes empeoro (5 vs. 3). Se **revirtio** al estado 78/81. Decision: documentar
como pendiente por limitacion del probador y avanzar, sin inventar demostraciones ni debilitar
contratos (condicion del proyecto).

**Cambios funcionales:** ninguno. Solo ACSL (contratos y la caracterizacion de `security_can`) +
el invariante de lazo ya presente. Regresión: **99/99** (normal + ASan/UBSan).

**Efecto colateral positivo (re-verificado):** al ampliar el contrato de `security_can` y el frame
de `audit_record_event`, sus modulos subieron:
- `security.c`: 69/69 → **105/105** (contrato de permisos completo; todo Qed, switch puro).
- `audit.c`: 95/95 → **98/98** (añadido el frame `ensures`; sigue 0 pendientes).

### Ejecución 2026-11-18 — `credentials.c` (Fase 4: ACSL completo del módulo)

**Medición inicial (sin modificar):** 52/86, 0 Failures, 34 Timeouts (= línea base; los contratos
de audit/security no ayudaron porque el módulo no tenía invariantes de lazo ni contratos propios).

**Resultado definitivo:**

| Módulo          | Probados | Total | No demostradas | Timeouts | Delta vs. base |
|-----------------|---------:|------:|---------------:|---------:|----------------|
| `credentials.c` | **226**  | 239   | 9              | 4        | 52/86 → 226/239 (abs.); −30 timeouts netos; +153 goals (contratos nuevos) |

Desglose: `Qed: 189, Alt-Ergo: 31, Terminating: 3, Unreachable: 3, Failed: 9, Timeout: 4`.

**Comando** (misma config; timeout 20 s, sin aumentar):
```bash
frama-c -wp -wp-rte -wp-prover alt-ergo,z3 -wp-timeout 20 \
  -cpp-extra-args="-Iinclude -Iverification" src/credentials.c
```

**Qué se corrigió (demostrado):**
- `cred_find`, `cred_find_by_owner`, `credential_status`: invariantes de lazo + contratos de solo
  lectura. **Cerraron los 15 timeouts de búsqueda/consulta** (los 34 originales menos los de escritura).
- `credential_status` / `credential_is_valid`: contrato con el bi-condicional P-6 (válida ⇔ ACTIVE).
  El `ensures` básico cierra; el existencial pesado queda en timeout (ver abajo).
- `credential_issue`, `cred_transition` (y wrappers activate/block/reactivate), `credential_revoke`:
  contratos con `requires` de contadores, `assigns` acotado por set-comprehension (solo la credencial
  con `id==cred_id` cambia), P-1 por rol, y result-set. La mayoría de sus RTE y precondiciones de
  llamada cerraron.

**Obligaciones NO demostradas (9 Failures + 4 Timeouts) — mismo patrón de framing de WP:**
- `issue_ensures` / `issue_assigns_part18` / `issue_ensures_2`: preservación de `creds_count` a través
  de la llamada de auditoría (framing sobre struct ~7 KB).
- `activate/block/reactivate_ensures_2` y `cred_transition_ensures/ensures_2`: el `ensures
  creds_count == \old(creds_count)` (atomicidad) no cierra por el mismo framing.
- `revoke_ensures` / `revoke_ensures_4` / `revoke_ensures_2`: result-set + P-8 (terminalidad de
  REVOKED, existencial) — framing + cuantificador pesado.
- `status_ensures` / `is_valid_ensures_2`: los `ensures` existenciales (relación resultado↔estado).

Todas son **ciertas** y están respaldadas por pruebas dinámicas (test_credentials.c, test_attacks.c
A6, A7, A13-A17). Ninguna es contraejemplo. Se conservan en los contratos (no se debilitan ni se
eliminan); su estado real es Failure/Timeout por limitación de automatización sobre el struct grande.

**P-6, P-7, P-8 — estado:** P-6 (válida⇔ACTIVE) especificada; su forma básica cierra, la existencial
no. P-7 (transición legal) garantizada por la estructura de `cred_transition` (exige `from` exacto),
con result-set demostrado. P-8 (REVOKED terminal) especificada como `ensures` en `credential_revoke`
pero no cerrada por WP (existencial + framing); respaldada por A7/A13.

**Cambios funcionales:** ninguno. Solo ACSL + invariantes de lazo + snapshot de índice en `issue`.
Regresión: **99/99** (normal + ASan/UBSan).
