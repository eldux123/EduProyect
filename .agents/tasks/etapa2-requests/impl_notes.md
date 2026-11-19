# Notas de implementación — ETAPA 2, PRIMER ENTREGABLE: módulo `requests` (aislado)

Rama activa: `etapa2-requests` (sin worktree). Fuente de verdad: `docs/diseno_flujo_solicitudes.md`
(Rev 5). Plan: `.agents/tasks/etapa2-requests/plan.md`. **Sin commits** (todo en el working tree).

Entorno de verificación: WSL2 distro **Ubuntu** (no docker-desktop). Frama-C **33.0 (Arsenic)** vía
opam. GCC con `-std=c99 -Wall -Wextra`. Las invocaciones de Frama-C se hacen desde scripts `.sh`
lanzados con `wsl -d Ubuntu bash <script>` (PowerShell rompe el quoting de `$(opam env)`; confirmado:
una invocación directa falló con `opam no se reconoce`).

> **RIGOR.** Todas las cifras de abajo son salidas REALES copiadas de las ejecuciones. Las salidas
> completas de WP se conservan en este mismo directorio: `wp_requests_out.txt`, `wp_security_out.txt`,
> `wp_credentials_out.txt`. El script de WP usado es `wp_requests.sh` (acepta el `.c` como argumento).

---

## 1. Archivos creados / modificados

| Archivo | Acción | Contenido |
|---------|--------|-----------|
| `include/common.h` | MODIFICADO (aditivo) | `MAX_REQUESTS=128`, `MAX_AUTHZ=64`; `ROLE_INSTITUTION=5` + `#define ROLE_PROFESSOR ROLE_PRO`; `op_code_t` 9..15 (`OP_SUBMIT_REQUEST`..`OP_REVOKE_REPRESENTATION`); enums `request_state_t` (REQ_PENDING=0..REQ_SUPERSEDED=5), `request_category_t` (14 valores), structs `request_t`, `inst_authz_t`, `requests_state_t` SEPARADO. Ningún valor existente cambió. |
| `include/requests.h` | CREADO | Prototipos + contratos ACSL del entregable; predicado `req_role_can` bajo `#ifdef __FRAMAC__`; `evidence_ref` documentado opaco. |
| `src/requests.c` | CREADO | `request_init`, `request_find`, `request_status`, `request_requires_endorsement`, `request_submit`, `request_submit_correction`, `request_endorse`, `request_verify`, `request_reject`; helpers estáticos `request_role_can`, `date_in_range`, `req_needs_endorsement`, `req_find_authz_active`, `req_submit_checks`, `req_insert`. |
| `tests/test_requests.c` | CREADO | `test_requests()` con 79 comprobaciones de todas las operaciones del entregable. |
| `tests/test_main.c` | MODIFICADO | prototipo + llamada a `test_requests()`. |
| `run_tests.sh` | MODIFICADO | `src/requests.c` añadido a `SRC`; `tests/test_requests.c` a `TST`. |

**Confirmación de alcance (prohibiciones respetadas):** NO se tocó `src/security.c`,
`include/security.h`, `src/credentials.c`, `include/credentials.h`, `src/professionals.c`,
`src/experience.c`, `src/audit.c` ni sus headers, ni `ui/`, ni `verification/specifications.h`, ni
`verification/wp_results_baseline.txt`, ni la sección LÍNEA BASE de `docs/formal_verification.md`.
La única dependencia modificada es la ampliación ADITIVA de `include/common.h`. El módulo NO llama a
`security_can` (lleva su propia `request_role_can` interna) ni integra `credentials.c`.

---

## 2. Funciones implementadas (reglas de negocio Rev 5)

- `request_init` — pone `reqs_count = 0; auth_count = 0`.
- `request_find` / `request_status` — búsqueda lineal de solo lectura (`assigns \nothing`).
- `request_requires_endorsement` — switch puro §5.3 (10 categorías exigen respaldo; RESEARCH_PROJECT
  devuelve 0 base; PUBLICATION/TRAINING/CERTIFICATION 0).
- `request_submit` — original (`prev_id=0`). PROFESSOR solo lo propio (`caller_id==owner_id`);
  INSTITUTION solo con vínculo activo en `auth[]` (`req_find_authz_active`). Nacimiento §5.0:
  INSTITUTION + exige respaldo → `REQ_ENDORSED` + `endorser_id=institution_id`; resto → `REQ_PENDING`.
  `exige_respaldo` efectivo = `request_requires_endorsement(cat) || (cat==RESEARCH && institution_id!=0)`.
  Audita `OP_SUBMIT_REQUEST` con `actor_id = caller_id`.
- `request_submit_correction` — corrección (`prev_id!=0`). Validación-S (§5.5): prev existe, prev en
  `REQ_RECORDED`, mismo owner, coherencia T-1 (misma categoría; institución fijada: igual si la
  categoría la asocia, ambas 0 si no). NO toca el registro anterior.
- `request_endorse` — `REQ_PENDING → REQ_ENDORSED`, solo INSTITUTION que representa, solo categorías
  que exigen respaldo.
- `request_verify` — → `REQ_VERIFIED` (VERIFIER). No autovalidación (`submitter_id==caller_id →
  ERR_SELF_VALIDATE`). Estado origen: `REQ_ENDORSED` si exige respaldo, `REQ_PENDING` si no (P-14).
- `request_reject` — `{REQ_PENDING|REQ_ENDORSED} → REQ_REJECTED` (VERIFIER, no autovalidación).

`caller_id` (ejecutor, auditado como `actor_id`) se mantiene distinto de `institution_id`
(institución representada). `evidence_ref` es opaco y no criptográfico (documentado en header/struct).

---

## 3. Contratos ACSL agregados

- Predicado `req_role_can(role_t, integer op)` (header, bajo `__FRAMAC__`): matriz §3.3 del flujo
  nuevo (SUBMIT: INSTITUTION|PRO; ENDORSE: INSTITUTION; VERIFY/REJECT: VERIFIER; resto `\false`).
  `request_role_can` (src) lo refleja (`ensures \result==1 <==> req_role_can(...)`, proved en WP).
- Cada operación: `requires \valid` (→ sin `ERR_NULL` en el `ensures` de resultados, patrón
  `credential_issue`); `requires \separated(st, rs)` (estados separados Rev 5 §4.5); `ensures` de
  conjunto de resultados; **P-1 mitad negativa** (`!req_role_can(...) ==> ERR_PERM||ERR_BAD_ID`);
  atomicidad del contador; `assigns` partido por `behavior` (`space`/`no_space` en submit,
  `success`/`log_full` en endorse/verify/reject, patrón `credential_issue`/`cred_transition`).
- `request_verify`/`request_reject`: **P-3** (no autovalidación, mitad negativa) y **P-7** (estado
  destino en éxito).
- Consultas (`request_find`/`request_status`/`req_find_authz_active`): `assigns \nothing` con
  `loop invariant`/`loop variant` (gemelas de `experience_find`/`credential_status`).

Lenguaje honesto: las propiedades de transición/atomicidad/existenciales se declaran como objetivo
WP; donde no cierran en WP quedan cubiertas por prueba dinámica (§5), NO se afirman «demostradas».
No se debilitó ningún contrato ni se introdujeron `assumes` injustificados para pasar WP.

---

## 4. Compilación y pruebas dinámicas (resultados REALES)

### 4.1 Compilación aislada del módulo (cero warnings)
```
wsl -d Ubuntu bash: gcc -std=c99 -Wall -Wextra -Iinclude -Itests -c src/requests.c -o /tmp/requests.o
```
Resultado: **compila sin warnings**.

### 4.2 Suite completa (normal + ASan/UBSan)
```
wsl -d Ubuntu bash /mnt/c/Users/erick/Downloads/Blockchain/EduProyect/run_tests.sh
```
(internamente: `gcc -std=c99 -Wall -Wextra -Iinclude -Itests $SRC $TST` y el mismo con
`-g -fsanitize=address,undefined -fno-omit-frame-pointer`)

Resultado REAL (ambos builds):
```
### Build normal (-Wall -Wextra) ###
=== Resultado: 178 pruebas, 0 fallos ===
### Build con ASan + UBSan ###
=== Resultado: 178 pruebas, 0 fallos ===
```
- **Base heredada antes del cambio: 99 pruebas, 0 fallos** (medido con el `run_tests.sh` original).
- **Tras el entregable: 178 pruebas, 0 fallos** = 99 heredadas intactas + 79 nuevas de
  `test_requests.c`. Cero warnings en ambos builds. Las 99 heredadas NO regresan con el common.h
  aditivo (recompilan con el nuevo header y siguen pasando).

---

## 5. Verificación formal WP (resultados REALES)

Comando (vía script, por el quoting de `$(opam env)`):
```
# .agents/tasks/etapa2-requests/wp_requests.sh <archivo.c>, lanzado con:
wsl -d Ubuntu bash .agents/tasks/etapa2-requests/wp_requests.sh src/requests.c
# contenido: eval "$(opam env)"; frama-c -wp -wp-rte -wp-prover alt-ergo,z3 -wp-timeout 20 \
#            -cpp-extra-args="-Iinclude -Iverification" src/requests.c
```

### 5.1 `src/requests.c` — salida REAL (ver `wp_requests_out.txt`)
```
[wp] 605 goals scheduled
...
[wp] Proved goals:  597 / 619
  Terminating:     7
  Unreachable:     7
  Qed:           501
  Alt-Ergo :      82
  Failed:         19
  Timeout:         3
```

**Goals NO cerrados (19 Failed + 3 Timeout), con su clasificación honesta:**

| Goal | Qué es | Por qué no cierra |
|------|--------|-------------------|
| `req_insert_ensures`, `req_insert_assigns_normal_part22`, `req_insert_call_audit_record_event_requires_2` | atomicidad `+1` / frame del slot escrito / precondición `log_count<=MAX` del `audit_record_event` tras escribir en `rs` | **Limitación de framing conocida** (`docs/explicacionv1.md §9`): escritura de struct + llamada a `audit_record_event` que cruza dos regiones (`st`/`rs`). **Misma clase** que el baseline de `credential_issue` (ver §5.3). |
| `request_submit_ensures`, `request_submit_ensures_5` | conjunto de resultados / `reqs_count<=MAX` | dependen del contrato de `req_insert` (arriba). |
| `request_submit_correction_ensures`, `_ensures_5` | ídem | ídem. |
| `request_endorse_ensures`, `_ensures_3`, `_ensures_4`, `success_assigns_normal_part16` | conjunto de resultados / `reqs_count` invariante / P-7 (destino ENDORSED) / frame | transición + `audit_record_event` (dos regiones), clase baseline `cred_transition`. |
| `request_verify_ensures`, `_ensures_3`, `_ensures_4`, `_ensures_5`, `success_assigns_normal_part15` | conjunto / `reqs_count` invariante / **P-3** / **P-7** / frame | ídem. |
| `request_reject_ensures`, `_ensures_3`, `_ensures_4`, `_ensures_5` | conjunto / `reqs_count` invariante / P-3 / P-7 | ídem. |
| `request_status_ensures` (Timeout) | existencial positivo de la consulta | clase baseline `credential_status_ensures` (también Timeout en el baseline). |
| `req_submit_checks_assert_rte_mem_access_3` (Timeout) | guard RTE de acceso a `auth[]` en la búsqueda | clase baseline `*_assert_rte_mem_access` (Timeout en professionals/experience/credentials). |

**Propiedades que SÍ cierran en WP (relevantes de seguridad):**
- **P-1 (autorización, mitad negativa)** `!req_role_can(...) ==> ERR_PERM||ERR_BAD_ID`: **PROVED** en
  las cuatro operaciones de escritura (`_ensures_2` NO aparece en ningún Failed, verificado por
  función: submit 35/37, endorse 85/89, verify 76/81, reject 58/62; los fallos son los índices de
  resultado/estado/P-3/P-7/frame, nunca el `_ensures_2` de P-1).
- `request_role_can` ≡ `req_role_can` (equivalencia switch↔predicado): PROVED.
- `request_init`, `request_find`, `request_requires_endorsement`, `req_find_authz_active`,
  `date_in_range`, `req_needs_endorsement`: sus obligaciones cierran (no aparecen en Failed, salvo el
  Timeout RTE puntual listado arriba).

**Decisión (criterio honesto Fase 4, exigido por el brief):** las obligaciones de
atomicidad/framing/transición que no cierran NO se fuerzan ni se debilita el contrato; quedan
**pendientes de WP** y **cubiertas por prueba dinámica** en `tests/test_requests.c` (transiciones,
nacimiento PENDING/ENDORSED, no-autovalidación, estado-origen, auditoría `actor_id=caller_id`,
atomicidad en error). Son de la **misma naturaleza** que las ya aceptadas en el baseline para
`credential_issue`/`cred_transition`/`experience_validate`.

### 5.2 No-regresión de `security.c` — salida REAL (ver `wp_security_out.txt`)
```
wsl -d Ubuntu bash .agents/tasks/etapa2-requests/wp_requests.sh src/security.c
[wp] Proved goals:  114 / 114
  Unreachable:     1
  Qed:           103
  Alt-Ergo :       9
```
**114/114 proved**: el common.h aditivo NO degrada `security.c`.

### 5.3 No-regresión de `credentials.c` — salida REAL (ver `wp_credentials_out.txt`)
```
wsl -d Ubuntu bash .agents/tasks/etapa2-requests/wp_requests.sh src/credentials.c
[wp] Proved goals:  299 / 315
  Unreachable:     3
  Qed:           260
  Alt-Ergo :      33
  Failed:         12
  Timeout:         4
```

**Prueba de NO-regresión por el common.h aditivo (control directo).** Se re-ejecutó WP sobre
`src/credentials.c` con los MISMOS flags pero con el `common.h` del `HEAD` (previo al cambio)
delante del include real (`-I<copia_baseline> -Iinclude -Iverification`). Resultado REAL con el
common.h BASELINE:
```
  Qed:           260
  Alt-Ergo :      33
  Failed:         12
  Timeout:         4
```
Idéntico perfil (Qed 260, Alt-Ergo 33, **12 Failed, 4 Timeout**) al de la rama con el common.h
modificado. **Conclusión: el cambio aditivo de `common.h` NO introduce ninguna regresión en el WP de
`credentials.c`.** (La copia baseline y los scripts auxiliares de medición se eliminaron tras la
comprobación.)

> **Nota sobre comparabilidad con `verification/wp_results_baseline.txt`.** El fichero baseline del
> repo se generó con flags distintos (sin `-wp-prover alt-ergo,z3`), por lo que sus cifras absolutas
> (p. ej. credentials 52/86) NO son comparables 1:1 con las de aquí (que usan alt-ergo+z3 y dan más
> goals cerrados). La no-regresión se demostró con el control a flags idénticos de §5.3, no comparando
> contra ese fichero. **NO se modificó** `verification/wp_results_baseline.txt` ni la sección LÍNEA
> BASE de `docs/formal_verification.md`.

### 5.4 Eva
No se ejecutó Eva en este entregable: el flujo no introduce aritmética ni accesos que Eva cubra mejor
que WP+RTE (sin bucles de aritmética de fechas, sin memoria dinámica); WP con `-wp-rte` ya cubre los
guards de acceso/overflow. Queda disponible para el entregable que añada `request_record` y consultas.

---

## 5.5 Corrección menor post-revisión: `requires` de `req_insert` (hallazgo revisor-acsl/revisor-c)

**Qué se corrigió.** El contrato ACSL del helper estático `req_insert` (src/requests.c) admitía
`st->log_count <= MAX_EVENTS` en su `requires`, lo que permitía describir un slot `log[MAX_EVENTS]`
fuera de rango en la cláusula `assigns`. Se endureció esa precondición a **estrictamente menor**:

```
# requires ANTES (citado textual):
    requires 0 <= st->log_count  <= MAX_EVENTS;
# requires DESPUÉS:
    requires 0 <= st->log_count  < MAX_EVENTS;
```

Único cambio aplicado: ese `<=` → `<` en una sola línea. No se tocó ningún otro `requires`/`ensures`,
ninguna otra función, ni `common.h`, ni los módulos existentes, ni la CLI, ni el baseline de WP.

**Por qué NO altera el runtime (contrato fortalecido, no debilitado).** Los dos únicos llamadores de
`req_insert` (`request_submit` y `request_submit_correction`) invocan el helper SOLO después de que
`req_submit_checks` devuelve `EDU_OK`. `req_submit_checks` retorna `ERR_FULL` cuando
`st->log_count >= MAX_EVENTS` (y así lo declara su `ensures`: `\result == EDU_OK ==> st->log_count <
MAX_EVENTS`). Por tanto los llamadores ya garantizaban `log_count < MAX_EVENTS` antes de llamar; la
precondición más estricta precisa lo que el código ya cumple, sin cambiar ninguna ejecución real y
sin violar ningún llamador. Es un fortalecimiento (precondición más fuerte, obligación de los
llamadores ya satisfecha), no un debilitamiento.

**Re-verificación REAL tras la corrección:**
- Compilación aislada `gcc -std=c99 -Wall -Wextra -Iinclude -Itests -c src/requests.c`: **sin warnings**.
- Suite completa `run_tests.sh` (normal + ASan/UBSan): **178 pruebas, 0 fallos** en ambos builds.
- WP sobre `src/requests.c` (mismos flags §5): **Proved goals 597 / 619** — `Qed 501`, `Alt-Ergo 82`,
  **`Failed 19`**, **`Timeout 3`**, `Terminating 7`, `Unreachable 7`. **Perfil idéntico** al estado
  previo del entregable (597/619, 19 Failed + 3 Timeout): **ningún goal antes Proved pasó a
  Failed/Timeout** (sin regresiones). El conjunto exacto de Failed/Timeout es el mismo listado en §5.1.
- **El Timeout `typed_req_insert_call_audit_record_event_requires_2` NO cerró**: sigue en Timeout tras
  la corrección. Honestidad: el valor de este cambio es la precondición más precisa (ya no se puede
  describir el slot fuera de rango en `assigns`), no cerrar ese goal, que permanece limitado por el
  framing de la escritura de struct + `audit_record_event` que cruza las regiones `st`/`rs` (§5.1).

**No-regresión de `security.c` / `credentials.c` — vía elegida: razonamiento (no re-ejecución).**
Esta corrección toca ÚNICAMENTE el contrato de `req_insert` dentro de `src/requests.c`. No modifica
`common.h` ni ningún header o fuente compartido; `security.c` (114/114) y `credentials.c` (299/315)
no incluyen `requests.c` ni dependen de su contrato, de modo que su perfil WP no puede verse afectado
por un cambio local a una precondición de un helper estático de otro módulo. Por eso no se reejecutó
WP sobre esos dos módulos (sus cifras de §5.2 y §5.3 siguen vigentes).

**Sin commit.** La corrección queda en el working tree de `etapa2-requests`, pendiente de que el
usuario autorice el commit tras ver el diff.

## 6. Estado del entregable

Implementado, compilado (cero warnings), probado (178/178, 0 fallos, normal + ASan/UBSan) y verificado
(WP con el perfil honesto de §5) el PRIMER ENTREGABLE: estructuras + init + consultas +
`request_submit` + `request_submit_correction` + `request_endorse` + `request_verify` +
`request_reject`. **Punto de parada**: NO se implementó `request_record`, ni las consultas públicas
`*_for`, ni `authz_grant/revoke`, ni la integración con `security.c`/`credentials.c` (entregables
posteriores). Sin commits: todo en el working tree de `etapa2-requests`.

---

# SEGUNDO INCREMENTO (ETAPA 2) — `request_record` + integración de credenciales

> Las secciones de arriba describen el PRIMER entregable (9 funciones), ya revisado y APROBADO
> (`review-verificacion.md`) y COMMITEADO en `92f3ce4`. Esta sección documenta el incremento nuevo.
> Las salidas WP nuevas se guardan SIN pisar las previas: `wp_requests_record_out.txt` (el primer
> entregable usaba `wp_requests_out.txt`, intacto).

## N1. Línea base REMEDIDA antes de tocar nada (no supuesta)

- `run_tests.sh` (normal + ASan/UBSan): **178 / 178, 0 fallos** en ambos builds. ✔ coincide con la cifra heredada.
- `wp_requests.sh src/requests.c`: **597 / 619** (19 Failed + 3 Timeout) — artefacto previo `wp_requests_out.txt`. (No re-medido en esta sesión salvo tras el cambio; ver N3.)
- `security.c` 114/114 y `credentials.c` 299/315: NO re-medidos todavía (no se han tocado esos módulos en este punto).

## N2. `request_record` implementado (ciclo de vida A + correcciones trazables B)

- Archivos tocados: `include/requests.h` (contrato ACSL + prototipo + predicado `req_role_can` ampliado con `OP_RECORD_REQUEST ==> ROLE_ADMIN`), `src/requests.c` (función `request_record` + caso `OP_RECORD_REQUEST` en el `request_role_can` interno), `tests/test_requests.c` (43 comprobaciones nuevas).
- **SEGURIDAD — SOLO ADMIN sin reabrir `security.c`:** se usa el `request_role_can` interno (switch puro) añadiendo el caso `OP_RECORD_REQUEST -> ROLE_ADMIN`. **NO se tocó `security.c`/`security.h`** (opción de menor superficie, como pide el brief): su WP 114/114 no se reabre.
- **Variante A (simple, `prev_id==0`):** `REQ_VERIFIED -> REQ_RECORDED`; estado origen EXACTO VERIFIED (no re-incorporar: `REQ_RECORDED`/otros -> `ERR_BAD_TRANSITION`); contenido aprobado inmutable (solo cambia `state`).
- **Variante B (corrección, `prev_id!=0`):** validación-R COMPLETA antes de mutar (patrón "validar todo antes de mutar nada"): prev existe; prev vigente `REQ_RECORDED`; mismo owner; T-1 (misma categoría; institución fijada por categoría, igual si asocia o ambas 0); `req_id` verificada; hueco de log. Mutación atómica de ambos slots SOLO en `state` (`req_id->REQ_RECORDED`, `prev->REQ_SUPERSEDED`).
- **Atomicidad:** toda validación precede a la primera escritura; en cualquier error ninguno de los dos slots cambia (probado dinámicamente: "atomicidad: no se auditó en error", "nueva intacta", "anterior intacto").
- **Carrera de dos correcciones:** la 1.ª gana; la 2.ª revalida `prev_id`, lo ve ya `REQ_SUPERSEDED` y falla LIMPIO con `ERR_BAD_TRANSITION` sin mutar (probado dinámicamente).
- **T-2 (aciclicidad):** por transiciones de estado, SIN recorrer la cadena `prev_id` (un `REQ_SUPERSEDED` terminal nunca vuelve a ser `prev_id`). No se añadió recorrido de cadena.
- **Historial conservado:** `REQ_SUPERSEDED` no se borra (probado: "SUPERSEDED no se borra").

## N3. Resultados REALES tras `request_record`

### N3.1 Tests (normal + ASan/UBSan) — `run_tests.sh`
**221 / 221, 0 fallos** en AMBOS builds (178 heredadas + 43 nuevas de `request_record`). **Cero warnings** de compilación.

### N3.2 WP `src/requests.c` — artefacto `wp_requests_record_out.txt`
```
[wp] 750 goals scheduled
[wp] Proved goals:  739 / 764
  Terminating:     7
  Unreachable:     7
  Qed:           626
  Alt-Ergo :      99
  Failed:         22
  Timeout:         3
```
Perfil: **739/764** (antes 597/619). El delta = +3 Failed nuevos, TODOS de `request_record`:
- `typed_request_record_ensures` — conjunto de resultados.
- `typed_request_record_ensures_3` — invariante `reqs_count` (contador intacto).
- `typed_request_record_ensures_4` — P-7 doble transición (existencial positivo VERIFIED->RECORDED).

**`typed_request_record_ensures_2` (P-1, mitad negativa de autorización `!req_role_can(RECORD) ==> ERR_PERM||ERR_BAD_ID`) está PROVED** (no aparece en la lista de Failure/Timeout). Los `assigns` por behavior (`success`/`log_full`) de `request_record` también cierran (no hay `request_record_*_assigns_part*` en Failed): el frame acotado a `reqs[k].state` + log captura la inmutabilidad de owner/categoría/institución/fechas/evidence_ref/metadatos.

**Honestidad:** los 3 Failed de `request_record` son de la MISMA clase de framing/transición que el baseline aceptado (submit/endorse/verify/reject/`req_insert`/`credential_issue`): la postcondición existencial/invariante cruza la escritura de struct + `audit_record_event` sobre regiones `st`/`rs`. La doble transición atómica y su framing quedan **PENDIENTES de WP** y **cubiertos por prueba dinámica** (carrera, atomicidad en error, SUPERSEDED, T-1). NO se forzó con `assumes` injustificados ni se debilitó el contrato. Ningún goal antes Proved pasó a Failed (sin regresión).

## N4. Parte C — integración de credenciales (`credential_eligible` + `credential_issue_eligible`)

**Decisión del usuario: OPCIÓN 2 (vía de emisión ADITIVA).** El diseño Rev 5 §8-ter especificaba la
Opción 1 (ampliar la firma de `credential_issue` con `const requests_state_t *rs`), decisión tomada
bajo el supuesto de la Opción 3 completa. Al acotar a Opción 2 (no inflar el proyecto ni romper
call-sites), la contradicción se resuelve a favor del brief vigente: se deja `credential_issue`
**INTACTA** (y sus 4 call-sites en `main.c`, `ui/educhain_cli.c`, `tests/test_attacks.c`,
`tests/test_credentials.c` sin tocar) y se añade una vía nueva.

- Archivos tocados: `include/credentials.h` (decls + contratos de las 2 funciones nuevas + include de `requests.h`), `src/credentials.c` (impl. de las 2 funciones), `include/requests.h` (predicado ACSL `req_eligible`), `tests/test_credentials.c` (20 comprobaciones nuevas).
- **`credential_eligible(const requests_state_t *rs, int owner_id)`** — SOLO LECTURA (`assigns \nothing`). Devuelve 1 si el owner tiene ≥1 registro VIGENTE `REQ_RECORDED` en `CAT_INSTITUTION_WORKED` o `CAT_LINK_PERIOD`; 0 si no. Patrón gemelo de `experience_validated_count` (flag acotado `0<=found<=1`, `loop invariant` con el existencial, sin aritmética de fechas). Excluye por construcción `REQ_SUPERSEDED` (no es `REQ_RECORDED`), `PENDING/ENDORSED/VERIFIED/REJECTED`, y lo heredado de `experience.c` (vive en `educhain_state_t`, no en `rs`).
- **`credential_issue_eligible(st, rs, role, caller_id, cred_id, owner_id)`** — vía aditiva. Si `!credential_eligible(rs, owner_id)` devuelve `ERR_PERM` SIN mutar; si es elegible delega en `credential_issue` (que conserva toda su validación: rol COORD, ids, profesional activo, duplicados, capacidad, auditoría). `rs` es de solo lectura: no amplía el frame de escritura de `credential_issue`. Consistencia registros↔credenciales: solo `REQ_RECORDED` vigente habilita.
- **ACSL:** se añadió el predicado `req_eligible(rs, owner)` en `requests.h` (bajo `__FRAMAC__`), gemelo lógico de `credential_eligible`, análogo a cómo `role_can` refleja `security_can`. Fue necesario porque WP no puede referenciar una función C en un contrato (error inicial `unbound logic predicate`); los contratos de ambas funciones usan `req_eligible`.

### N4.1 Tests (normal + ASan/UBSan) — `run_tests.sh`
**241 / 241, 0 fallos** en AMBOS builds (221 + 20 nuevas). **Cero warnings.** Cubre: elegible (INSTITUTION_WORKED, LINK_PERIOD), no elegible (sin registros, otro owner, categoría no habilitante), estados no vigentes no cuentan, **SUPERSEDED no cuenta** (y SUPERSEDED+RECORDED sí por el RECORDED); `credential_issue_eligible` emite con trayectoria válida, rechaza `ERR_PERM` sin trayectoria (sin mutar), hereda la validación de rol, y SUPERSEDED no habilita.

### N4.2 WP `src/credentials.c` — artefacto `wp_credentials_eligible_out.txt` (REABIERTO)
```
[wp] Proved goals:  362 / 382
  Terminating:     4
  Unreachable:     4
  Qed:           311
  Alt-Ergo :      43
  Failed:         16
  Timeout:         4
```
**Cambio de perfil vs. baseline 299/315 (ESPERADO).** Comparación directa de los conjuntos
Failure/Timeout (ambos artefactos conservados: `wp_credentials_out.txt` = baseline,
`wp_credentials_eligible_out.txt` = nuevo):
- Las 12 Failed + 4 Timeout del baseline **siguen idénticas** (cred_transition, credential_activate/block/reactivate `_ensures_2`, credential_issue `ensures`/`space_assigns_part18`, credential_revoke, y los 4 Timeout). **Ningún goal antes Proved regresó a Failure.**
- **Delta = +4 Failed nuevos, TODOS de `credential_issue_eligible`:** `_ensures` (conjunto de resultados), `_ensures_2` (`!req_eligible ==> ERR_PERM`), `_ensures_3` (`EDU_OK ==> req_eligible`), `not_eligible_or_no_space_assigns_normal_part5` (frame del camino no-elegible).
- **`credential_eligible` (la función pura P-20) cierra COMPLETAMENTE en WP:** no aparece en ningún Failure/Timeout → su `assigns \nothing` y el bicondicional `\result==1 <==> req_eligible` están **PROVED**.

**Honestidad:** los 4 Failed de `credential_issue_eligible` son de la MISMA clase de framing ya
aceptada: la función delega en `credential_issue`, cuyo propio resultado/frame (a través de
`audit_record_event`) no permite a WP conectar la precondición `req_eligible` con las conclusiones
`EDU_OK`/`assigns` del wrapper. Quedan **PENDIENTES de WP** y **cubiertos por prueba dinámica**
(N4.1). NO se forzó con `assumes` injustificados ni se debilitó ningún contrato.

## N5. Resumen de archivos tocados en el incremento (working tree, sin commits)

| Archivo | Acción |
|---------|--------|
| `include/requests.h` | contrato+prototipo `request_record`; `req_role_can` ampliado (`OP_RECORD_REQUEST`); predicado `req_eligible` |
| `src/requests.c` | `request_record` (Variantes A/B); caso `OP_RECORD_REQUEST` en `request_role_can` |
| `include/credentials.h` | decls+contratos `credential_eligible` / `credential_issue_eligible`; include `requests.h`; nota de alcance Opción 1 vs 2 |
| `src/credentials.c` | impl. `credential_eligible` / `credential_issue_eligible` |
| `tests/test_requests.c` | +43 comprobaciones `request_record` |
| `tests/test_credentials.c` | +20 comprobaciones de elegibilidad/emisión |

**NO tocados:** `security.c`/`security.h` (ADMIN vía `request_role_can` interno → WP 114/114 intacto, no reabierto), `main.c`, `ui/educhain_cli.c`, `experience.c`, `professionals.c`, `audit.c` ni sus headers, `verification/specifications.h`, `verification/wp_results_baseline.txt`.

## N6. Cifras REALES finales del incremento

| Métrica | Resultado | Baseline | Nota |
|---------|-----------|----------|------|
| Tests normal | **241/241, 0 fallos** | 178/178 | +63 nuevas (43 record + 20 cred) |
| Tests ASan/UBSan | **241/241, 0 fallos** | 178/178 | 0 warnings compilación |
| WP `requests.c` | **739/764** (22 Failed, 3 Timeout) | 597/619 | +3 Failed (request_record); P-1 ADMIN PROVED |
| WP `credentials.c` | **362/382** (16 Failed, 4 Timeout) | 299/315 | +4 Failed (issue_eligible); credential_eligible PROVED |
| WP `security.c` | **no reabierto** | 114/114 | no se tocó security.c |

**Pendientes honestos (cubiertos por prueba dinámica, NO forzados):**
- `request_record`: doble transición atómica + framing (3 goals) — misma clase que el baseline vía `audit_record_event`.
- `credential_issue_eligible`: conexión elegibilidad↔emisión + frame del wrapper (4 goals) — delega en `credential_issue`, misma clase de framing.

**Sin commits.** Todo en el working tree de `etapa2-requests`. Artefactos WP nuevos conservados sin pisar los previos: `wp_requests_record_out.txt`, `wp_credentials_eligible_out.txt`.

---

# MEJORA ACSL post-revisión (revisor-acsl) sobre `request_record` — previa al commit Opción 2

> Dos observaciones MENORES del revisor-acsl, **de precisión del contrato, NO de comportamiento**:
> el código C de `request_record` NO cambia (verificado: la función `src/requests.c` no se tocó, solo
> el contrato ACSL en `include/requests.h`). Objetivo: un contrato MÁS PRECISO Y HONESTO. Artefactos
> nuevos conservados SIN pisar los previos: `wp_requests_record_v2_out.txt`,
> `wp_credentials_eligible_v2_out.txt` (los v1 `wp_requests_record_out.txt` /
> `wp_credentials_eligible_out.txt` quedan intactos como evidencia del estado aprobado 739/764 y
> 362/382).

## M1. Mejora #1 — `assigns` acotado a los DOS slots concretos (no todos los slots)

**Antes** (estado aprobado): el `behavior success` declaraba
`assigns { rs->reqs[k].state | integer k; 0 <= k < rs->reqs_count }, st->log[...], st->log_count;`
— es decir, el campo `.state` de TODOS los slots.

**Después:** el set-comprehension restringe los índices escribibles a los DOS slots involucrados por
su `id`: el slot cuyo `id == req_id` (el nuevo, `-> REQ_RECORDED`) y, en la Variante B, el slot cuyo
`id == prev_id` del propio `req_id` (el anterior, `-> REQ_SUPERSEDED`):
```
assigns { rs->reqs[k].state | integer k;
          0 <= k < rs->reqs_count
          && (rs->reqs[k].id == req_id
              || (\exists integer j; 0 <= j < rs->reqs_count
                  && rs->reqs[j].id == req_id
                  && rs->reqs[k].id == rs->reqs[j].prev_id)) },
        st->log[st->log_count], st->log_count;
```
Sigue siendo campo-`state`-only (la inmutabilidad de owner/institution_id/category/fechas/
evidence_ref/prev_id/num_a..d se deriva del frame) y además acota QUÉ slots pueden cambiar (los dos
concretos, no todos). **WP lo ACEPTA (Proved):** el goal del `assigns` de `request_record` NO aparece
en la lista de Failure/Timeout del artefacto v2 (ningún `request_record_*_assigns_*` falla). No fue
necesario recorrer índices problemáticos; el acotamiento por `id` es más preciso y correcto.

## M2. Mejora #2 — `ensures` explícitos de la doble transición e inmutabilidad-salvo-state

Se añadieron dos `ensures` (anclados en `\old` lo que corresponde):
- **(a) anterior -> REQ_SUPERSEDED** (Variante B): en éxito, si el `req_id` incorporado tenía
  `\old(prev_id) != 0`, el slot cuyo `id == \old(prev_id)` pasa de `REQ_RECORDED` a `REQ_SUPERSEDED`.
  En la Variante A (`prev_id == 0`) no hay anterior, por eso el antecedente exige `\old(prev_id)!=0`.
- **(b) nuevo -> REQ_RECORDED**: ya estaba el P-7 existencial (`_ensures_4`: `req_id` estaba
  `REQ_VERIFIED` y queda `REQ_RECORDED`); (a) y (c) lo complementan.
- **(c) inmutabilidad-salvo-state del anterior**: en éxito de la Variante B, el slot anterior conserva
  TODOS sus campos EXCEPTO `state`, anclados en `\old`: `owner_id`, `institution_id`, `submitter_id`,
  `verifier_id`, `endorser_id`, `category`, `start_date`, `end_date`, `evidence_ref`, `prev_id`,
  `num_a..d`.

## M3. Resultados REALES tras la mejora ACSL

### M3.1 Compilación (cero warnings)
`gcc -std=c99 -Wall -Wextra -Iinclude -Itests -c src/requests.c`: **compila sin warnings** (el código C
no cambió).

### M3.2 Tests (normal + ASan/UBSan) — `run_tests.sh`
**241 / 241, 0 fallos** en AMBOS builds. **Cero warnings.** Sin regresión (idéntico a N6).

### M3.3 WP `src/requests.c` — artefacto `wp_requests_record_v2_out.txt`
```
[wp] 752 goals scheduled
[wp] Proved goals:  739 / 766
  Terminating:     7
  Unreachable:     7
  Qed:           624
  Alt-Ergo :     101
  Failed:         24
  Timeout:         3
```
**Comparación con el estado aprobado 739/764** (artefacto v1 `wp_requests_record_out.txt`):
- **Proved: 739 -> 739** (sin cambio).
- **Total goals: 764 -> 766** (+2 obligaciones nuevas, las dos `ensures` añadidas).
- **Diff exacto de los conjuntos Failure/Timeout** (`comm` de los goals `typed_*`): el ÚNICO delta son
  **DOS Failed nuevos: `typed_request_record_ensures_5` y `typed_request_record_ensures_6`** (las
  cláusulas (a) y (c) nuevas). **NINGÚN goal que estaba Proved pasó a Failed/Timeout** (el conjunto
  "en v1 pero no en v2" es VACÍO). El `assigns` acotado de M1 **sigue Proved**.
- Failed 22 -> 24 (+2 nuevos), Timeout 3 -> 3 (sin cambio).

**Honestidad (lenguaje explícito, NO 'demostrado').** Los dos `ensures` nuevos (doble transición
anterior->SUPERSEDED e inmutabilidad-salvo-state) quedan **Failed en WP**, como se anticipó: son de la
MISMA CLASE de framing (escritura de struct + `audit_record_event` que cruza las regiones `st`/`rs`)
que ya NO cierra en el baseline aceptado (submit/endorse/verify/reject/`req_insert`/`credential_issue`/
`request_record_ensures`). Son **PENDIENTES de WP cubiertos por prueba dinámica** (los tests de
`request_record`: doble transición, SUPERSEDED, inmutabilidad del anterior, atomicidad en error,
carrera de dos correcciones). NO se debilitó ningún contrato, NO se introdujeron `assumes`
injustificados, NO se eliminaron aserciones, NO se forzó el cierre. El valor de la mejora es un
contrato MÁS PRECISO Y HONESTO (el `assigns` ahora nombra solo los dos slots; los `ensures` declaran
explícitamente lo que el código hace), no un número de goals mayor.

### M3.4 WP `src/credentials.c` — artefacto `wp_credentials_eligible_v2_out.txt` (no se tocó credentials.c)
```
[wp] Proved goals:  362 / 382
  Terminating:     4
  Unreachable:     4
  Qed:           311
  Alt-Ergo :      43
  Failed:         16
  Timeout:         4
```
**362/382, 16 Failed, 4 Timeout** — **idéntico** al estado aprobado. Diff exacto de los conjuntos
Failure/Timeout vs. el artefacto v1 `wp_credentials_eligible_out.txt`: **sin cambios** (0 nuevos, 0
desaparecidos). Sin regresión (esperado: esta tarea no modifica `credentials.c`; se reejecutó solo
para confirmar).

## M4. Resumen de la mejora ACSL

| Métrica | Resultado | Estado aprobado | Nota |
|---------|-----------|-----------------|------|
| Compilación | **0 warnings** | 0 warnings | código C sin cambios |
| Tests normal | **241/241, 0 fallos** | 241/241 | sin regresión |
| Tests ASan/UBSan | **241/241, 0 fallos** | 241/241 | sin regresión |
| WP `requests.c` | **739/766** (24 Failed, 3 Timeout) | 739/764 | +2 Failed = los 2 `ensures` nuevos; ningún Proved regresó; `assigns` acotado Proved |
| WP `credentials.c` | **362/382** (16 Failed, 4 Timeout) | 362/382 | idéntico, sin regresión |

**Archivo tocado por la mejora:** ÚNICAMENTE `include/requests.h` (contrato ACSL de `request_record`).
`src/requests.c` NO cambió (solo contrato). Nada de Opción 3. Sin tocar `fase4-acsl`.
