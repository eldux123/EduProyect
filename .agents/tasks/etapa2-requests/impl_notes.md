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
