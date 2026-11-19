# EduChain — Explicación v1 (qué hace el código actualmente)

Este documento describe lo que el código del repositorio **realmente contiene y hace** a la fecha
de redacción. Está escrito a partir de la lectura directa de las cabeceras (`include/*.h`), las
implementaciones (`src/*.c`), `main.c`, la CLI (`ui/educhain_cli.c`), `docs/formal_verification.md`
y los artefactos de verificación versionados (`verification/wp_results_fase4_*.txt`,
`verification/eva_results_fase4.txt`). Donde una propiedad está demostrada formalmente se dice; donde
solo hay respaldo dinámico (pruebas) o queda pendiente, se distingue con honestidad.

---

## 1. Qué es EduChain y qué NO es

EduChain es un **programa en C que modela la lógica de un contrato inteligente**. Simula cómo una
organización ficticia (**EduCorp**) acreditaría experiencia profesional y gestionaría credenciales
digitales internas: registrar profesionales, registrar y validar su experiencia, y emitir/operar una
credencial por profesional, dejando un historial de auditoría de todas las operaciones.

Lo que **NO es**, según el propio código y sus comentarios (`include/common.h`, `main.c`,
`ui/educhain_cli.c`):

- **No es una blockchain real.** No hay red, nodos, consenso ni persistencia distribuida. El estado
  vive en una sola estructura en memoria (`educhain_state_t`).
- **No se despliega en ninguna red** ni interactúa con una cadena.
- **No usa criptografía** (no hay hashes, firmas ni claves) ni **datos personales reales**.
- La credencial que gestiona es un **carné institucional ficticio**. Los registros guardan
  identificadores numéricos y estados, no nombres, títulos, foto, número de colegiatura ni ningún
  dato que la convierta en una **tarjeta profesional legal**. Es deliberadamente un modelo académico.

---

## 2. Stack técnico real

Tomado de lo que evidencian el repositorio, los scripts y `docs/formal_verification.md`:

| Elemento | Valor |
|----------|-------|
| Lenguaje | C estándar **C99** (`gcc -std=c99 -Wall -Wextra`) |
| Especificación formal | **ACSL** (anotaciones en comentarios `/*@ ... */`) |
| Verificador | **Frama-C 33.0 (Arsenic)** |
| Plugin de prueba deductiva | **WP** (`-wp -wp-rte`), timeout 20 s por obligación |
| Plugin de análisis de valores | **Eva** (dominio `cvalue`) |
| Solucionadores | **Alt-Ergo** y **Qed** cerraron las obligaciones. **Z3 4.13.0** está configurado vía why3 pero **no cerró ninguna obligación** en estas corridas |
| Build / pruebas | **gcc** con `-fsanitize=address,undefined` (**ASan/UBSan**) |
| Includes de verificación | `-cpp-extra-args="-Iinclude -Iverification"` |

Los predicados ACSL de Fase 4 (p. ej. `role_can`) viven en `verification/specifications.h` y se
incluyen solo bajo `#ifdef __FRAMAC__`, de modo que el build normal con gcc (que usa `-Iinclude`,
no `-Iverification`) no los ve. Honestidad sobre Z3: el documento de verificación afirma
explícitamente que, aunque WP se invoca con `-wp-prover alt-ergo,z3`, todas las obligaciones cerradas
lo fueron por Qed o Alt-Ergo. (Contenido reformulado para cumplir con restricciones de licencia.)

---

## 3. Arquitectura: los 5 módulos

El sistema se divide en cinco módulos. Cada uno tiene su cabecera en `include/` y su implementación
en `src/`. La lógica de negocio vive solo en estos módulos; `main.c` y la CLI son consumidores.

### 3.1 `security` — punto único de decisión de autorización
Codifica la **matriz de permisos** (rol × operación). Función pública:

- `int security_can(role_t caller_role, int op_code)` — función pura de solo lectura. Devuelve `1`
  si el rol puede ejecutar la operación, `0` si no. Es un `switch` sobre `op_code` sin bucles ni
  estado. Todas las demás funciones que mutan estado consultan aquí la autorización por rol.

### 3.2 `professionals` — registro de profesionales
- `int professional_register(educhain_state_t *st, role_t caller_role, int caller_id, int new_id)`
  — registra un profesional nuevo y **activo**. Permitido a `ROLE_ADMIN` y `ROLE_HR`. Valida rango
  de ids, evita duplicados, respeta capacidad y audita la operación. `caller_id` es quien ejecuta
  (no necesita existir en `pros[]`).
- `int professional_find(const educhain_state_t *st, int id)` — solo lectura; devuelve índice o `-1`.
- `int professional_is_active(const educhain_state_t *st, int id)` — solo lectura; `1` si existe y
  está activo, `0` si no.

### 3.3 `experience` — registro y validación de experiencia
- `int experience_register(educhain_state_t *st, role_t caller_role, int caller_id, int exp_id, int owner_id, int start_date, int end_date)`
  — registra una experiencia en estado `EXP_PENDING`. Permitido a `ROLE_HR`, y a `ROLE_PRO` **solo
  si `caller_id == owner_id`** (un profesional solo registra experiencia propia). Valida rango de
  fechas y `start_date <= end_date`; el titular debe existir y estar activo; sin duplicados.
- `int experience_validate(educhain_state_t *st, role_t caller_role, int caller_id, int exp_id)`
  — pasa una experiencia de `EXP_PENDING` a `EXP_VALIDATED` y fija `validator_id`. Permitido a
  `ROLE_HR` y `ROLE_COORD`. **Prohíbe la autovalidación** (`caller_id` no puede ser el `owner_id`).
  No modifica fechas ni titular (inmutabilidad). Rechaza re-validar lo ya validado.
- `int experience_find(const educhain_state_t *st, int exp_id)` — solo lectura; índice o `-1`.
- `int experience_validated_count(const educhain_state_t *st, int owner_id)` — solo lectura; cuenta
  experiencias `EXP_VALIDATED` del titular, o devuelve `ERR_BAD_ID` / `ERR_NOT_FOUND`.

### 3.4 `credentials` — credencial como máquina de estados
- `int credential_issue(educhain_state_t *st, role_t caller_role, int caller_id, int cred_id, int owner_id)`
  — emite una credencial en `CRED_PENDING`. Permitido a `ROLE_COORD`. **Una credencial por
  profesional** (rechaza si el titular ya tiene una).
- `int credential_activate(...)` — `PENDING → ACTIVE`. Permitido a `ROLE_COORD`.
- `int credential_block(...)` — `ACTIVE → BLOCKED`. Permitido a `ROLE_ADMIN` / `ROLE_COORD`.
- `int credential_reactivate(...)` — `BLOCKED → ACTIVE`. Permitido a `ROLE_ADMIN` / `ROLE_COORD`.
- `int credential_revoke(...)` — `{PENDING, ACTIVE, BLOCKED} → REVOKED` (terminal). Permitido a
  `ROLE_ADMIN` / `ROLE_COORD`.
- `int credential_status(const educhain_state_t *st, int cred_id)` — solo lectura; devuelve el
  `cred_state_t` o `-1` si no existe.
- `int credential_is_valid(const educhain_state_t *st, int cred_id)` — solo lectura; `1` solo si la
  credencial está en `CRED_ACTIVE`.

Internamente, las cuatro transiciones delegan en una auxiliar `cred_transition(...)` que valida
rango de ids, autorización, existencia y el estado de origen exacto antes de mutar.

### 3.5 `audit` — historial append-only
- `int audit_record_event(educhain_state_t *st, int actor_id, int op_code, int target_id, int result)`
  — añade un evento **al final** del log con `seq` estrictamente creciente; no sobrescribe eventos
  previos (append-only). Devuelve `ERR_FULL` si el log está lleno o si `seq` se saturaría.
- `int audit_verify_event(const educhain_state_t *st, int seq)` — solo lectura; busca por `seq`,
  devuelve índice o `-1`.
- `int audit_get_count(const educhain_state_t *st)` — solo lectura; devuelve `log_count`.

Cada operación de escritura de los otros módulos registra un evento con `actor_id = caller_id` (quien
ejecuta) y `result = EDU_OK`.

---

## 4. Modelo de datos real

### 4.1 Límites exactos (`#define` de `include/common.h`)

| Constante | Valor | Significado |
|-----------|------:|-------------|
| `MAX_PROFESSIONALS` | 64 | capacidad de `pros[]` |
| `MAX_EXPERIENCES` | 256 | capacidad de `exps[]` |
| `MAX_CREDENTIALS` | 64 | capacidad de `creds[]` |
| `MAX_EVENTS` | 1024 | capacidad del log de auditoría |
| `ID_MIN` | 1 | id mínimo válido |
| `ID_MAX` | 2000000000 | id máximo válido (dentro de `int` de 32 bits) |
| `DATE_MIN` | 19000101 | fecha mínima (YYYYMMDD) |
| `DATE_MAX` | 21001231 | fecha máxima (YYYYMMDD) |

### 4.2 Roles (`role_t`)

| Nombre | Valor |
|--------|------:|
| `ROLE_ADMIN` | 0 |
| `ROLE_HR` | 1 |
| `ROLE_COORD` | 2 |
| `ROLE_PRO` | 3 |
| `ROLE_VERIFIER` | 4 |

### 4.3 Operaciones / `op_code_t` (para auditoría y permisos)

| Nombre | Valor |
|--------|------:|
| `OP_REGISTER_PRO` | 1 |
| `OP_REGISTER_EXP` | 2 |
| `OP_VALIDATE_EXP` | 3 |
| `OP_ISSUE_CRED` | 4 |
| `OP_ACTIVATE_CRED` | 5 |
| `OP_BLOCK_CRED` | 6 |
| `OP_REACTIVATE_CRED` | 7 |
| `OP_REVOKE_CRED` | 8 |

### 4.4 Estructuras y qué guarda cada registro

- `professional_t`: `id` (`0` = slot libre), `active` (0/1).
- `experience_t`: `id`, `owner_id`, `start_date` (YYYYMMDD), `end_date` (YYYYMMDD, `>= start_date`),
  `state` (`exp_state_t`), `validator_id` (`0` si no validada; distinto de `owner_id` si validada).
- `credential_t`: `id`, `owner_id`, `state` (`cred_state_t`).
- `event_t`: `seq` (secuencial, estrictamente creciente), `actor_id`, `op_code`, `target_id`,
  `result`.

### 4.5 Estados de los enums

- `exp_state_t`: `EXP_PENDING = 0`, `EXP_VALIDATED = 1`.
- `cred_state_t`: `CRED_PENDING = 0`, `CRED_ACTIVE = 1`, `CRED_BLOCKED = 2`, `CRED_REVOKED = 3`.

### 4.6 Estado global

Todo el sistema vive en un único `educhain_state_t`:

```
professional_t pros[MAX_PROFESSIONALS];  int pros_count;
experience_t   exps[MAX_EXPERIENCES];    int exps_count;
credential_t   creds[MAX_CREDENTIALS];   int creds_count;
event_t        log[MAX_EVENTS];          int log_count;
```

Son arreglos de tamaño fijo con un contador cada uno. La estructura completa ocupa del orden de
~7 KB, dato relevante para la sección de limitaciones (framing de WP).

### 4.7 Matriz de permisos (tal como la implementa `security_can`)

| Operación | Roles autorizados |
|-----------|-------------------|
| `OP_REGISTER_PRO` | ADMIN, HR |
| `OP_REGISTER_EXP` | HR, PRO (PRO solo su propia experiencia, verificado en `experience_register`) |
| `OP_VALIDATE_EXP` | HR, COORD |
| `OP_ISSUE_CRED` | COORD |
| `OP_ACTIVATE_CRED` | COORD |
| `OP_BLOCK_CRED` | ADMIN, COORD |
| `OP_REACTIVATE_CRED` | ADMIN, COORD |
| `OP_REVOKE_CRED` | ADMIN, COORD |

`ROLE_VERIFIER` no tiene permiso de escritura en ninguna operación (devuelve siempre `0`). Una
operación desconocida también devuelve `0`.

---

## 5. Códigos de retorno (`include/common.h`)

| Código | Valor | Significado |
|--------|------:|-------------|
| `EDU_OK` | 0 | operación aceptada |
| `ERR_NULL` | -1 | puntero de estado nulo |
| `ERR_PERM` | -2 | el rol del ejecutor no tiene permiso |
| `ERR_SELF_VALIDATE` | -3 | no se puede validar la experiencia propia |
| `ERR_DUPLICATE` | -4 | ya existe un registro con ese identificador |
| `ERR_NOT_FOUND` | -5 | el registro referenciado no existe |
| `ERR_BAD_DATE` | -6 | fecha fuera de rango o `start_date > end_date` |
| `ERR_BAD_ID` | -7 | identificador fuera del rango permitido |
| `ERR_FULL` | -8 | capacidad del registro agotada |
| `ERR_BAD_TRANSITION` | -9 | transición de estado ilegal (p. ej. REVOKED es terminal) |
| `ERR_IMMUTABLE` | -10 | campo inmutable: no puede modificarse |
| `ERR_ALREADY_VALID` | -11 | la experiencia ya estaba validada |

Nota: `ERR_IMMUTABLE` está definido pero ninguna función del código lo devuelve actualmente; la
inmutabilidad de fechas/titular se garantiza no escribiendo esos campos, no con este código.

---

## 6. Máquina de estados de la credencial

Estados reales (`cred_state_t`) y transiciones que implementa `credentials.c`:

| Transición | Función | Estado origen → destino | Roles |
|------------|---------|-------------------------|-------|
| Emisión | `credential_issue` | (nueva) → `CRED_PENDING` | COORD |
| Activación | `credential_activate` | `CRED_PENDING` → `CRED_ACTIVE` | COORD |
| Bloqueo | `credential_block` | `CRED_ACTIVE` → `CRED_BLOCKED` | ADMIN, COORD |
| Reactivación | `credential_reactivate` | `CRED_BLOCKED` → `CRED_ACTIVE` | ADMIN, COORD |
| Revocación | `credential_revoke` | `CRED_PENDING` / `CRED_ACTIVE` / `CRED_BLOCKED` → `CRED_REVOKED` | ADMIN, COORD |

```
PENDING --activate--> ACTIVE <--reactivate-- BLOCKED
                        |  \                   /
                        |   \--block-->-------/
                        |
   (PENDING/ACTIVE/BLOCKED) --revoke--> REVOKED  [terminal]
```

`CRED_REVOKED` es **terminal**: `credential_revoke` sobre una credencial ya revocada devuelve
`ERR_BAD_TRANSITION`, y no hay ninguna transición de salida de `REVOKED`. Cualquier transición que
no case con el estado de origen exacto esperado devuelve `ERR_BAD_TRANSITION`. Solo `CRED_ACTIVE`
cuenta como "válida para uso" (`credential_is_valid`).

---

## 7. Reglas de seguridad / propiedades y su estado de verificación

Las reglas que el código pretende cumplir, y su estado según `docs/formal_verification.md` y los
artefactos versionados. Se distingue **Demostrada en WP** (Qed/Alt-Ergo), **Failure** (WP no cierra,
falla rápido), **Timeout** (expira a 20 s) y **límite genuino del probador** (p. ej. falta de
invariante global `state_inv`). Ninguna de las Failure/Timeout observadas es un contraejemplo: las
propiedades son ciertas y están respaldadas por pruebas dinámicas.

| Regla / propiedad | Dónde | Estado de verificación |
|-------------------|-------|------------------------|
| Autorización por rol (matriz §3) | `security_can` | **Demostrada en WP** a nivel del decisor puro (`security.c` 114/114, todo Qed/Alt-Ergo). La autorización de extremo a extremo en los llamadores queda parcialmente Failure/Timeout |
| No autovalidación (R-2) | `experience_validate` | Lógica presente en el código; respaldo dinámico (prueba A2). El `ensures` P-2 queda en **Failure** por requerir unicidad global (`state_inv`) — límite del probador, ya no contraejemplo |
| Unicidad (sin duplicados de id; una credencial por profesional) | `professional_register`, `experience_register`, `credential_issue` | Implementada con búsquedas previas; respaldo dinámico. No formalizada como invariante global en WP |
| Fechas válidas y `start <= end` (BR-4) | `experience_register` | Lógica presente; `ensures` P-4 en **Failure/Timeout** por framing del struct; respaldo dinámico (prueba A5) |
| Transiciones legales (P-7) | `cred_transition` y wrappers | `cred_transition` exige el estado origen exacto; result-set demostrado en parte, existenciales from→to en **Failure** por framing a través de la auditoría |
| REVOKED terminal (P-8) | `credential_revoke` | Garantizada por el código; `ensures` en **Failure** (requiere unicidad global `state_inv`); respaldo dinámico (A7, A13, A15) |
| Auditoría append-only + `seq` creciente (P-12) | `audit_record_event` | **Demostrada en WP** a nivel de la operación (`audit.c` 95/95 en su corrida, 0 pendientes). La monotonía *global* de todo el log queda pendiente de un invariante compartido |
| Atomicidad (en error, contadores sin cambio; BR-10) | módulos de escritura | Parcial: varios `ensures` en **Failure/Timeout** por framing del struct ~7 KB a través de `audit_record_event`; respaldo dinámico |
| Ausencia de UB (RTE) | todos | **Eva: 0 alarmas** en las trazas del harness; WP cierra la mayoría de las guardas RTE de rango/overflow en las funciones de solo lectura |

### Cifras WP (artefactos de Fase 4 versionados — estado "después")

| Módulo | Probados / Total | Failure | Timeout |
|--------|-----------------:|--------:|--------:|
| `security.c` | **114 / 114** | 0 | 0 |
| `experience.c` | **146 / 179** | 9 | 24 |
| `credentials.c` | **298 / 315** | 12 | 5 |

Fuente: `verification/wp_results_fase4_security.txt`, `..._experience.txt`, `..._credentials.txt`
(línea `[wp] Proved goals:`). Los módulos `professionals.c` y `audit.c` no se re-volcaron como
artefacto "fase4" separado; sus últimos resultados registrados en `docs/formal_verification.md` son
`professionals.c` 78/81 (2 Failure, 1 Timeout) y `audit.c` 95/95 (0 pendientes). Como referencia, la
**línea base** original (`verification/wp_results_baseline.txt`, no modificada) fue: `security.c`
69/69, `audit.c` 68/74, `professionals.c` 62/74, `experience.c` 75/106, `credentials.c` 52/86.

> Nota sobre totales: las especificaciones nuevas **generan más obligaciones**, por lo que los
> totales por módulo cambian entre configuraciones y no se deben comparar como porcentajes. Lo
> relevante es que ningún objetivo previamente probado pasó a Failure al fortalecer la spec.

### Cifras Eva (`verification/eva_results_fase4.txt`)

- 23 funciones analizadas de 23 (100% de cobertura de funciones).
- 443 de 547 sentencias alcanzadas (81% de cobertura de sentencias).
- **0 alarmas** generadas por el análisis.
- Assertions: 5 válidas, 12 unknown, 0 inválidas (17 total).
- Preconditions: 91 válidas, 0 unknown, 0 inválidas (91 total).

Las 12 assertions "unknown" son `assert` manuales y aserciones derivadas de los contratos que Eva no
resuelve por su naturaleza; **ninguna es inválida**, así que no hay contradicción detectada. Eva
prueba ausencia de UB solo en las trazas que el harness ejercita, no en todos los casos posibles.

---

## 8. Cómo se ejecuta y se prueba

Los scripts asumen ejecución en WSL/Linux con gcc. Compilan con `-std=c99 -Wall -Wextra -Iinclude`.

- **`run_tests.sh`** — compila y ejecuta la batería de pruebas (`tests/test_professionals.c`,
  `test_experience.c`, `test_credentials.c`, `test_attacks.c`, `test_main.c`) en **dos builds**:
  normal y con **ASan + UBSan**. Según `docs/formal_verification.md`, el resultado es **99/99 pruebas,
  0 fallos** en ambas builds.
- **`run_demo.sh`** — compila `main.c` con los cinco módulos y ejecuta una demo del flujo feliz y
  varios rechazos (registrar profesional, registrar/validar experiencia, emitir y operar una
  credencial, intento de autovalidación, revocación, etc.).
- **`ui/run_ui.sh`** — compila y ejecuta la **CLI interactiva de terminal** (`ui/educhain_cli.c`).
  Soporta `--asan` (build con sanitizers) y `--build` (solo compilar). La CLI es una capa de
  presentación pura: ofrece un menú (registrar/validar/emitir/transicionar credenciales, consultar
  estado, listar registros, ver el historial de auditoría, cambiar el ejecutor `caller_id`/rol y
  cargar un escenario de ejemplo). No contiene lógica de negocio; toda decisión la toman los módulos
  a través de sus funciones públicas, reusando los códigos de retorno de `common.h`.

La verificación formal se ejecuta con Frama-C (`verification/verify.sh` y scripts auxiliares),
usando los comandos WP y Eva de la sección 2.

---

## 9. Limitaciones honestas

- **Obligaciones no demostradas (Failure/Timeout).** Varias propiedades funcionales están
  especificadas como `ensures` en los contratos ACSL pero WP no las cierra. Las dos causas reales
  documentadas son: (a) **framing del struct `educhain_state_t` (~7 KB) a través de
  `audit_record_event`**, que bloquea la atomicidad de contadores y las relaciones de post-estado en
  los call-sites; y (b) **ausencia de un invariante global `state_inv`** (unicidad de `id` y
  append-only global), que hace que WP no pueda ligar el índice encontrado por una búsqueda con la
  variable del existencial (afecta P-2 no autovalidación, P-8 positiva, y los existenciales de
  consulta). Son límites de automatización/modelado, no bugs: no se observó ningún contraejemplo y
  las 99/99 pruebas (normal + ASan/UBSan) pasan.
- **Propiedades con solo respaldo dinámico.** No autovalidación (P-2), REVOKED terminal (P-8),
  unicidad y atomicidad de contadores se sostienen hoy por las pruebas (`test_attacks.c`,
  `test_experience.c`, `test_credentials.c`), no por una demostración WP cerrada de extremo a extremo.
- **Smoke-tests de código muerto benigno.** Con `-wp-smoke-tests`, WP marca como inalcanzables las
  guardas defensivas `if (st == 0) return ERR_NULL;`, porque el `requires \valid(st)` ya garantiza
  que `st` no es nulo. Es benigno: la guarda se conserva para el build real y no se elimina.
- **Mejoras no implementadas (fuera de alcance):**
  - **S-12 / campo `result` de auditoría:** `event_t.result` siempre vale `EDU_OK` hoy (código
    muerto). Usarlo para auditar intentos fallidos sería un cambio funcional; queda como mejora
    futura.
  - **Deuda de prueba:** faltan pruebas de `actor_id` real en transiciones, de arreglos llenos
    (`creds[]`/`exps[]`) y de log lleno. Son pruebas dinámicas adicionales, no corrección de spec.
- **`ERR_IMMUTABLE`** está definido pero ninguna función lo devuelve.

---

> **Nota de procedencia.** Las cifras de verificación (WP y Eva) de este documento provienen de los
> artefactos versionados en el repositorio a la fecha de redacción: `verification/wp_results_fase4_security.txt`
> (114/114), `verification/wp_results_fase4_experience.txt` (146/179), `verification/wp_results_fase4_credentials.txt`
> (298/315), `verification/eva_results_fase4.txt` (0 alarmas; 91 precondiciones / 17 assertions) y
> `verification/wp_results_baseline.txt` (línea base, no modificada), junto con el registro narrado en
> `docs/formal_verification.md`. Si esos artefactos se regeneran, estas cifras deben releerse de ellos.
