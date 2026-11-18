# EduChain — Fase 2: Arquitectura y firmas de funciones en C

Esta fase define **cómo se organiza el código** y **qué firma tiene cada función**, con su contrato de
comportamiento en lenguaje natural (precondición / postcondición / qué modifica / errores). Todavía **no
hay implementación** (`src/*.c` vacíos de lógica) y **no hay ACSL** (eso es Fase 4). Las decisiones de
diseño de la Fase 1 quedan fijadas:

- Fechas: validación de **rango + `start <= end`** (no calendario completo).
- Reactivación `BLOCKED -> ACTIVE`: permitida, solo `ROLE_ADMIN` / `ROLE_COORD`.
- Una credencial por profesional (unicidad por `owner_id`).

---

## 1. Principios de arquitectura

1. **Núcleo puro y determinista.** Toda función recibe explícitamente el estado (`educhain_state_t *`),
   el rol/ID del invocador y los parámetros. No hay variables globales mutables, ni I/O, ni reloj, ni
   `malloc`. Esto hace cada función una unidad cerrada, ideal para WP.
2. **Estado explícito, no oculto.** El estado completo vive en una struct que el llamador posee. El
   núcleo nunca guarda punteros entre llamadas.
3. **Un punto de verificación de permisos.** La autorización se decide en una tabla/función central
   (`security.c`), no dispersa. Separación de responsabilidades.
4. **Atomicidad por función.** Cada función valida *todo* antes de mutar *nada*. Si algo falla, retorna
   error con el estado intacto (BR-10).
5. **Append-only para auditoría.** El log solo crece; escribir un evento es la última acción de una
   operación exitosa.
6. **Sin dependencias externas.** Solo C estándar (`<stdint.h>`, `<stddef.h>`). Facilita compilar bajo
   Frama-C y con ASan/UBSan.

---

## 2. Mapa de módulos

```
include/            verification/        src/              tests/
  common.h            specifications.h     professionals.c   test_professionals.c
  professionals.h     invariants.h         experience.c      test_experience.c
  experience.h                             credentials.c     test_credentials.c
  credentials.h                            audit.c           test_attacks.c
  audit.h                                  security.c
  security.h
                                          main.c  (demo CLI mínima, no verificada)
```

Diferencias respecto a la estructura sugerida en el enunciado (§8) y su justificación técnica:
- Se añade **`common.h`**: tipos compartidos (`role_t`, códigos de error, `educhain_state_t`, constantes).
  Evita dependencias circulares entre headers de módulo.
- Se añade **`security.[ch]`**: centraliza la matriz de permisos (principio #3). El enunciado ficticio
  no lo listaba, pero la verificación de autorización necesita un único punto demostrable.
- `verification/specifications.h` e `invariants.h` se reservan para **macros/predicados ACSL** compartidos
  (Fase 4); en Fase 2 se crean como stubs documentados.

### Grafo de dependencias (sin ciclos)

```
common.h  <--- security.h <--- professionals.h
   ^              ^                 experience.h
   |              |                 credentials.h
   +--------------+---------------- audit.h
```

Todos dependen de `common.h`. `experience`, `credentials` y `professionals` dependen de `security` (para
autorizar) y de `audit` (para registrar eventos). `audit` depende solo de `common`.

---

## 3. Flujo típico de una operación de escritura

```
caller -> función de módulo
            1. ¿punteros no nulos?            -> si no: ERR_NULL
            2. ¿parámetros en rango? (IDs/fechas) -> si no: ERR_BAD_ID / ERR_BAD_DATE
            3. ¿rol autorizado? (security)    -> si no: ERR_PERM
            4. reglas específicas (dup, self, transición, capacidad)
            5. aplicar mutación (único punto que escribe)
            6. audit_record_event(...)        -> si log lleno: revertir? No: se decide abajo
            7. return EDU_OK
```

**Decisión sobre el paso 6:** si el log está lleno (`ERR_FULL` en auditoría) *después* de aplicar la
mutación, se rompería la atomicidad. Para preservar BR-10 + BR-11, el orden real será: **verificar
capacidad del log ANTES de mutar** (paso 0.5), de modo que una operación que no puede auditarse no se
aplica. Así "toda operación exitosa queda auditada" y "un error no muta el estado" se cumplen juntas.

---

## 4. Firmas y contratos (lenguaje natural; ACSL en Fase 4)

Convenciones: todas devuelven `int` (código de §6 de Fase 1). `st` es `educhain_state_t *`. `caller_role`
y `caller_id` identifican al invocador (dato confiable provisto por la capa externa).

### 4.1 `security.h`

```c
int security_can(role_t caller_role, int op_code);
```
- **Pre:** `caller_role` es un valor válido del enum.
- **Post:** retorna 1 si el rol puede ejecutar `op_code` según la matriz (§3 Fase 1), 0 si no.
- **Assigns:** nada (función pura de solo lectura).
- Es el único lugar que codifica la matriz de permisos.

### 4.2 `professionals.h`

```c
int professional_register(educhain_state_t *st, role_t caller_role, int new_id);
int professional_find(const educhain_state_t *st, int id);   /* retorna índice o -1 */
int professional_is_active(const educhain_state_t *st, int id);
```
- `professional_register`
  - **Pre:** `st != NULL`.
  - **Reglas:** rol ∈ {ADMIN, HR} (BR-1); `new_id ∈ ID_MIN..ID_MAX` (BR-9); no duplicado (BR-3);
    `pros_count < MAX_PROFESSIONALS` (BR-8); log con espacio.
  - **Post (éxito):** se añade un profesional activo con `new_id`; `pros_count` crece en 1; evento
    registrado; retorna `EDU_OK`.
  - **Post (error):** estado idéntico (BR-10); retorna `ERR_NULL/ERR_PERM/ERR_BAD_ID/ERR_DUPLICATE/ERR_FULL`.
  - **Assigns:** `st->pros[...]`, `st->pros_count`, `st->log[...]`, `st->log_count`.
- `professional_find` / `professional_is_active`: solo lectura, **assigns nada**; find retorna índice
  válido `0..pros_count-1` o `-1`.

### 4.3 `experience.h`

```c
int experience_register(educhain_state_t *st, role_t caller_role, int caller_id,
                        int exp_id, int owner_id, int start_date, int end_date);
int experience_validate(educhain_state_t *st, role_t caller_role, int caller_id, int exp_id);
int experience_find(const educhain_state_t *st, int exp_id);   /* índice o -1 */
long experience_total_days(const educhain_state_t *st, int owner_id); /* agregado solo lectura */
```
- `experience_register`
  - **Reglas:** rol ∈ {HR, PRO} y si PRO entonces `caller_id == owner_id` (un PRO solo registra lo propio);
    IDs válidos; `owner_id` existe (BR-? / `ERR_NOT_FOUND`); fechas en rango y `start<=end` (BR-4);
    `exp_id` no duplicado (BR-3); capacidad (BR-8); log con espacio.
  - **Post (éxito):** experiencia nueva en estado `EXP_PENDING`, `validator_id = 0`; evento; `EDU_OK`.
  - **Assigns:** `st->exps[...]`, `st->exps_count`, `st->log[...]`, `st->log_count`.
- `experience_validate`
  - **Reglas:** rol ∈ {HR, COORD} (BR-1); experiencia existe; `caller_id != owner` de esa experiencia
    (BR-2, `ERR_SELF_VALIDATE`); estado actual `EXP_PENDING` (si ya `VALIDATED` → `ERR_ALREADY_VALID`,
    y nunca se modifica, BR-5); log con espacio.
  - **Post (éxito):** `state = EXP_VALIDATED`, `validator_id = caller_id`; `start/end/owner` **no cambian**;
    evento; `EDU_OK`.
  - **Assigns:** solo `st->exps[i].state`, `st->exps[i].validator_id`, y el log.
- `experience_total_days`: solo lectura; suma de duraciones de experiencias **validadas** del `owner_id`,
  sin contar dos veces el mismo registro; retorna agregado no negativo. **Assigns nada.** (La propiedad de
  no-doble-conteo se demuestra por construcción: itera índices distintos una sola vez.)

### 4.4 `credentials.h`

```c
int credential_issue(educhain_state_t *st, role_t caller_role, int cred_id, int owner_id);
int credential_activate(educhain_state_t *st, role_t caller_role, int cred_id);
int credential_block(educhain_state_t *st, role_t caller_role, int cred_id);
int credential_reactivate(educhain_state_t *st, role_t caller_role, int cred_id);
int credential_revoke(educhain_state_t *st, role_t caller_role, int cred_id);
int credential_status(const educhain_state_t *st, int cred_id);   /* retorna cred_state_t o -1 */
int credential_is_valid(const educhain_state_t *st, int cred_id); /* 1 sii ACTIVE */
```
- Todas las de escritura comparten el patrón: validar punteros/ID/rol, localizar credencial, **verificar
  que la transición solicitada está en la tabla §5.1** (BR-7), aplicar, auditar.
  - `credential_issue`: rol COORD; crea en `CRED_PENDING`; `owner_id` existe; una por `owner_id` (BR-3).
  - `credential_activate`: rol COORD; `PENDING->ACTIVE`.
  - `credential_block`: rol ADMIN/COORD; `ACTIVE->BLOCKED`.
  - `credential_reactivate`: rol ADMIN/COORD; `BLOCKED->ACTIVE`.
  - `credential_revoke`: rol ADMIN/COORD; `{PENDING,ACTIVE,BLOCKED}->REVOKED`. `REVOKED` terminal (P-8).
  - Cualquier transición fuera de tabla → `ERR_BAD_TRANSITION`, estado intacto.
- `credential_status` / `credential_is_valid`: solo lectura, **assigns nada**. `is_valid` retorna 1
  únicamente si el estado es `CRED_ACTIVE` (BR-6, P-6).

### 4.5 `audit.h`

```c
int audit_record_event(educhain_state_t *st, int actor_id, int op_code, int target_id, int result);
int audit_verify_event(const educhain_state_t *st, int seq);   /* índice o -1 */
int audit_get_count(const educhain_state_t *st);
```
- `audit_record_event`
  - **Pre:** `st != NULL`.
  - **Reglas:** `log_count < MAX_EVENTS` (BR-8/11) si no `ERR_FULL`; `seq` del nuevo evento =
    `log_count == 0 ? 1 : log[log_count-1].seq + 1` (estrictamente creciente, P-12) sin desbordar
    (`seq <= INT_MAX`, P-10).
  - **Post (éxito):** se **añade** un evento al final; eventos previos **no se modifican** (append-only);
    `log_count` crece en 1; `EDU_OK`.
  - **Assigns:** solo `st->log[st->log_count]` y `st->log_count`.
- `audit_verify_event`: solo lectura; busca por `seq`, retorna índice o -1. **Assigns nada.**

---

## 5. Módulo `main.c`

Demo mínima no verificada: construye un `educhain_state_t`, ejecuta una secuencia feliz
(registrar profesional → registrar experiencia → validar con otro actor → emitir/activar credencial →
imprimir estado e historial). Sirve para humanos, **no** forma parte de la verificación formal. Se marca
explícitamente como fuera del alcance de WP/Eva.

---

## 6. Trazabilidad reglas → funciones

| Regla  | Funciones que la hacen cumplir                                        |
|--------|-----------------------------------------------------------------------|
| BR-1   | `security_can` + chequeo de rol en cada escritura                     |
| BR-2   | `experience_validate` (caller != owner)                               |
| BR-3   | `*_register`, `credential_issue` (búsqueda previa de duplicado)       |
| BR-4   | `experience_register` (rango + start<=end)                            |
| BR-5   | `experience_validate` (solo toca state/validator; nunca fechas)       |
| BR-6   | `credential_is_valid` / `credential_status`                           |
| BR-7   | las 5 `credential_*` de escritura (tabla de transiciones)             |
| BR-8   | chequeo `count < MAX_*` en cada inserción + log                       |
| BR-9   | validación de IDs/fechas/punteros al inicio de cada función           |
| BR-10  | patrón "validar todo antes de mutar"                                  |
| BR-11  | `audit_record_event` (append-only, seq creciente)                     |

---

## 7. Reporte de Fase 2

- **Qué se desarrolló:** arquitectura modular (6 headers + 5 src + main), grafo de dependencias sin
  ciclos, firmas completas de todas las funciones, contrato de comportamiento por función, flujo de
  operación con la decisión de "auditar antes de mutar" para conciliar atomicidad y append-only.
- **Decisiones tomadas:** añadir `common.h` y `security.[ch]`; núcleo sin estado global ni `malloc`;
  verificar capacidad de log antes de mutar; PRO puede registrar solo su propia experiencia.
- **Problemas / abiertos:** ninguno bloqueante. Punto menor a confirmar en Fase 4: si `experience_total_days`
  usa `long` para el agregado (propuesta) o se omite por no ser parte del alcance mínimo — se incluye
  porque aporta una propiedad de "no doble conteo" interesante de verificar, pero puede recortarse.
- **Qué falta:** crear los archivos de cabecera reales (stubs sin lógica) y `common.h`, para que Fase 3
  tenga dónde implementar. Eso se hace a continuación como cierre tangible de Fase 2.
- **Resultados obtenidos:** diseño listo; sin verificación aún (correcto).
- **Para continuar:** validar arquitectura → Fase 3 (implementación de la lógica en `src/*.c`).
