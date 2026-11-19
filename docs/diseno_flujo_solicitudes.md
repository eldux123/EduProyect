# Diseño detallado — Flujo de solicitudes (presentador → verificador → administrador)

> **Revisión 5 — ETAPA 1, SOLO DISEÑO (READ-ONLY).** Este documento no implementa nada. No se
> modificó ningún `.c`, `.h`, contrato ACSL, CLI (`ui/`), tests, scripts de verificación ni línea
> base / resultados de WP·Eva. El único archivo escrito es este `docs/diseno_flujo_solicitudes.md`.
> Todas las citas de funciones, contratos y cifras provienen de la lectura directa del repositorio en
> la rama `fase4-acsl` (cabeceras `include/*.h`, fuentes `src/*.c`, `verification/*`, `tests/*`,
> `ui/educhain_cli.c`, `docs/explicacionv1.md` y `docs/formal_verification.md`).
>
> **Revisión 5 (cierre DEFINITIVO de las tres decisiones de negocio abiertas, SIGUE READ-ONLY sobre
> el código).** Toma la Revisión 4 como base y cierra, sin dejar ninguna abierta, las tres decisiones
> que la Rev 4 marcaba como pendientes/tensiones. Cambios respecto de la Rev 4:
> - **1. Regla DEFINITIVA de elegibilidad de credenciales (§8-ter, §8 P-20, §11, §14, apéndices).** Un
>   profesor es elegible para recibir o renovar una credencial cuando tiene **al menos un** registro
>   vigente `REQ_RECORDED` de categoría `CAT_INSTITUTION_WORKED` **o** `CAT_LINK_PERIOD`. Se formaliza
>   con la función conceptual de SOLO LECTURA `credential_eligible(rs, owner_id)` (`assigns \nothing`),
>   gemela de `experience_validated_count` (`src/experience.c`). Deja de ser «parámetro de diseño»: el
>   criterio queda fijado. `request_recorded_count(rs, owner) > 0` genérico **ya no aplica** como
>   criterio (era un ejemplo insuficiente). Es una regla del **modelo académico EduChain**, no una
>   exigencia legal profesional; no se añaden requisitos de horas, años ni publicaciones.
> - **2. Cierre de la tensión T-1 (categoría/institución en correcciones, §5.5, §7.1).** La categoría
>   de la corrección **siempre** coincide con la del registro original; si la categoría asocia
>   institución, ambos registros tienen **exactamente** el mismo `institution_id`; si no la asocia,
>   ambos tienen `institution_id = 0`. **No** se permite cambiar la institución mediante una
>   corrección. Se actualiza la validación en `request_submit_correction` (validación-S) y en
>   `request_record` (validación-R). **T-1 se retira de los asuntos pendientes.**
> - **3. Cierre de la tensión T-2 (cadena de `prev_id`, §5.5).** Decisión definitiva: **NO** se
>   recorre explícitamente la cadena `prev_id`. La aciclicidad de las correcciones que llegan al
>   historial oficial se sostiene en las transiciones de estado (`prev_id` apunta a un registro
>   existente y vigente; al incorporar, el anterior pasa a `REQ_SUPERSEDED`; un `REQ_SUPERSEDED` no
>   vuelve a ser `prev_id`; la revalidación al incorporar impide dos sustituciones del mismo registro),
>   no del orden de ids ni de recorrer la cadena. **T-2 se retira de los asuntos pendientes.**
> - **Estado final:** NO queda pendiente ninguna de las tres decisiones de negocio que estaban
>   abiertas. Se mantienen aprobadas: integración directa con `credentials.c`, gestión de `auth[]`
>   exclusiva de ADMIN, reglas de respaldo institucional, sustitución atómica vía `REQ_SUPERSEDED`
>   obligatoria, y `requests_state_t` separado de `educhain_state_t`. Lenguaje ASPIRACIONAL para los
>   resultados WP·Eva de las propiedades nuevas (se espera / objetivo, nunca «demostrado»).
>
> **Procedencia de esta revisión.** El cierre 1 se fundamenta en la lectura directa de
> `src/experience.c` (`experience_validated_count`: búsqueda lineal con `loop invariant 0<=count<=i`,
> `loop assigns i, count`, `assigns \nothing`, sin aritmética de fechas) como patrón gemelo de
> `credential_eligible`, y de `src/credentials.c` (`credential_issue(educhain_state_t *st, role_t,
> int caller_id, int cred_id, int owner_id)`, hoy SIN `requests_state_t`; `behaviors space`/`no_space`).
> Los cierres 2 y 3 se fundamentan en la máquina de estados ya descrita (§4.1, §5.4, §5.5) y en el
> patrón `cred_transition` (estado origen exacto, `assigns` acotado por `behavior`).
>
> **Revisión 4 (correcciones finales, SIGUE READ-ONLY sobre el código).** Toma la Revisión 3 como
> base y aplica seis correcciones, con revisión de coherencia global. Cambios respecto de la Rev 3:
> - **1. Correcciones y `prev_id` (§4.2, §5.5, §5.6, §6, §7).** Se decide **no** añadir `prev_id` a la
>   firma de `request_submit`: se diseña una función específica `request_submit_correction` para crear
>   la solicitud de corrección (justificación en §5.6). `prev_id` se valida **dos veces**: al
>   PRESENTAR y, de nuevo, al INCORPORAR (`request_record` revalida porque el estado del anterior pudo
>   cambiar entre medias). Se resuelve la carrera de dos correcciones verificadas sobre el mismo
>   registro (la primera incorporación gana; la segunda falla al revalidar). La aciclicidad se
>   garantiza por las **transiciones de estado** (no por el orden numérico de ids). Se define el caso
>   de una corrección ya verificada cuyo anterior dejó de estar vigente.
> - **2. Flujo de presentación (§5, §6, §7 unificadas).** Las secciones máquina-de-estados, flujo y
>   firmas se reorganizan en una narrativa única sin solapamientos, con reglas de nacimiento idénticas
>   en los tres sitios: PROFESSOR con respaldo obligatorio → `PENDING`; INSTITUTION que presenta
>   directamente una categoría que exige respaldo → `ENDORSED` (fija `endorser_id`), sin segunda
>   operación; sin respaldo obligatorio → `PENDING`. El respaldo nunca sustituye al VERIFIER.
> - **3. Credenciales (§8-ter, §14).** La integración DIRECTA de `credentials.c` con `requests_state_t`
>   (solo lectura) queda como **decisión tomada**, no como pregunta: se modifica `credential_issue`
>   en modo solo-lectura sobre `rs`, lo que REABRE y obliga a re-verificar WP de `credentials.c`
>   contra la línea base. Se ELIMINA de §14 la pregunta «modificar `credential_issue` vs capa nueva».
>   Queda pendiente SOLO el criterio concreto de elegibilidad; `request_recorded_count > 0` se marca
>   como ejemplo insuficiente, no como regla aprobada.
> - **4. Consultas y permisos (§3.3 bis, §7.3).** Se distinguen las **primitivas internas** de solo
>   lectura (sin chequeo de rol) de una **capa de consulta pública** con contrato de rol + identidad +
>   ámbito. Matriz de consultas por rol (PROFESSOR/INSTITUTION/VERIFIER/ADMIN).
> - **5. ACSL de `authz_grant`/`authz_revoke` (§3.2 bis).** Se corrige el contrato: `ERR_NULL` deja de
>   figurar como resultado alcanzable (los `requires \valid` lo hacen inalcanzable, igual que en
>   `credential_issue`); el `assigns` deja de nombrar `auth[auth_count]` incondicionalmente (se parte
>   por `behavior`, patrón `credential_issue`). Se diferencian inserción / reactivación / duplicado /
>   capacidad-llena con `behaviors complete+disjoint`.
> - **6. Consistencia general.** Se actualizan las referencias obsoletas a «Revisión 2»; se corrige la
>   afirmación de «no hay contradicciones» dejando las tensiones residuales como puntos abiertos
>   honestos (§14); se alinean firmas, estados, flujo, ACSL y plan de etapas; lenguaje aspiracional
>   para los resultados WP·Eva nuevos.
>
> **Procedencia de esta revisión.** Las correcciones 1–5 se fundamentan en la lectura directa de
> `src/credentials.c` (`credential_issue` con `behavior space`/`no_space`, `cred_transition` con
> `assigns` partido y `!role_can ==> (ERR_PERM || ERR_BAD_ID)`), `src/experience.c`
> (`experience_validate`, `experience_validated_count`), `src/security.c`, `include/common.h` (enums
> y `ERR_*`), `include/security.h` y `verification/specifications.h` (predicado `role_can`).
>
> **Revisión 2 (ajustes del usuario, sigue READ-ONLY sobre el código).** Esta versión
> incorpora seis ajustes pedidos tras la aprobación de la arquitectura: (1) metadatos por
> categoría dentro de un struct genérico; (2) `caller_id` vs `institution_id` con autorización
> de representación; (3) integración `requests.c ↔ credentials.c` basada solo en `REQ_RECORDED`;
> (4) `requests_state_t` separado vs ampliar `educhain_state_t`; (5) `evidence_ref` declarado
> opaco y no criptográfico; (6) criterio por categoría de respaldo institucional previo (propuesta
> a confirmar por el usuario). Y un requisito transversal: separación estricta entre el historial
> oficial nuevo (`REQ_RECORDED`) y los registros heredados de `experience.c`. Las cuatro
> recomendaciones originales se conservan; donde un ajuste cambia una decisión, se explica por qué.
>
> **Revisión 3 (cierre de decisiones del usuario, SIGUE READ-ONLY sobre el código).** Esta
> versión CIERRA las decisiones que la Revisión 2 dejaba marcadas como propuesta y AÑADE un
> requisito nuevo. Cambios respecto de la Rev 2:
> - **A. Respaldo institucional por categoría — cerrado.** La tabla §5.3 pasa de propuesta a
>   decisión. Proyectos de investigación: respaldo **condicional** según `institution_id` presente
>   o ausente. Y regla nueva: una **INSTITUTION que presenta directamente** una solicitud de una
>   categoría que exige respaldo deja constancia de ese respaldo en el mismo acto (nace `ENDORSED`,
>   sin segunda operación `request_endorse`); una del **PROFESSOR** queda `PENDING` hasta respaldo.
> - **B. Gestión de `auth[]` por ADMIN en runtime — cerrado.** Se diseñan tres operaciones nuevas
>   (autorizar, revocar, consultar) con estados activo/inactivo, errores y contratos (§3.2 bis).
> - **C. Integración con `credentials.c` — cerrada como integración directa de solo lectura** sobre
>   el historial vigente `REQ_RECORDED` (excluye `REQ_SUPERSEDED` y las rutas heredadas), con la
>   regla de elegibilidad dejada explícitamente como **parámetro de diseño** (§8-ter).
> - **D. Capacidades cerradas:** `MAX_REQUESTS = 128`, `MAX_AUTHZ = 64` (§4.5, §10-D6), no
>   definitivas para producción.
> - **REQUISITO NUEVO — corrección del historial con `REQ_SUPERSEDED` (§4.1, §5, §6, §7, §8, §8-bis,
>   §8-ter, §9, §11).** La incorporación de una corrección y la sustitución del registro anterior son
>   **una sola operación atómica**; `request_record` adquiere dos variantes de efecto (incorporación
>   simple y incorporación-con-sustitución); `request_recorded_count()` excluye `REQ_SUPERSEDED`.
>
> **Las piezas del entregable de la Rev 5** (documento etiquetado Revisión 5; resumen de las
> decisiones CERRADAS; lista de secciones modificadas; confirmación EXPLÍCITA de que NO quedan
> decisiones de negocio pendientes; confirmación de que solo se modificó este Markdown) están al
> final, en los apéndices §12–§15.

---

## Resumen ejecutivo y las cuatro recomendaciones

El flujo nuevo (solicitud → verificación → incorporación oficial, con separación estricta
PROFESSOR/INSTITUTION → VERIFIER → ADMIN) encaja de forma natural sobre la arquitectura actual:
el patrón "validar todo antes de mutar nada", el decisor único `security_can`, la auditoría
append-only con `actor_id = caller_id` y la máquina de estados con transición genérica
(`cred_transition`) ya son exactamente las piezas que el flujo nuevo necesita. Las cuatro
decisiones de diseño, con su recomendación fundamentada (**todas conservadas tras los ajustes**):

| # | Decisión | Recomendación | Fundamento en una línea |
|---|----------|---------------|-------------------------|
| 1 | **Arquitectura** | **Módulo nuevo `requests.c` + `requests.h`**, sin tocar `experience.c`/`credentials.c` como lógica, y **cerrar** las rutas viejas vía la matriz de permisos. | Preserva intactas las 5 unidades ya verificadas; el nuevo flujo se demuestra de cero sin rehacer lo conseguido; el riesgo de "saltarse el flujo" se neutraliza en `security_can`, el único punto de decisión. |
| 2 | **Roles** | **Migrar al modelo de 4 actores** (ADMIN, INSTITUTION, VERIFIER, PROFESSOR), **conservando los valores enum existentes** y **negando a HR/COORD toda operación del flujo nuevo**. | La separación estricta exige que ningún rol pueda saltar el flujo; dejar a HR/COORD con sus escrituras *solo sobre el circuito viejo* evita romper tests/CLI mientras les niega toda ruta al nuevo historial oficial. |
| 3 | **Evidencias/cripto** | **Identificador/compromiso opaco** (`int evidence_ref`), **sin** SHA-256 ni criptografía; declarado explícitamente como NO-hash y SIN garantía de integridad documental. | Un hash real introduce aritmética/bucles que disparan el cuello de botella de WP; el alcance original excluye criptografía; "un hash tampoco demuestra veracidad". |
| 4 | **Modelo de datos** | **Un struct genérico `request_t` con `enum request_category`** y un **conjunto ACOTADO de campos numéricos reinterpretados por categoría** (sin inflar a 14 variantes), en un arreglo de tamaño fijo. | El tamaño del struct ya es el cuello de botella de WP; N structs por categoría multiplican el framing y las obligaciones; los metadatos caben en pocos campos genéricos. |

**Novedad transversal de esta revisión — estado nuevo AISLADO.** Se recomienda un
**`requests_state_t` independiente** (§4.5 y §10, Decisión 5 nueva) en lugar de ampliar el
`educhain_state_t` global (~7 KB). Motivo central: ampliar el struct global **degradaría** las
obligaciones de WP que hoy cierran en los módulos ya verificados (el framing a través de
`audit_record_event` empeora al crecer el struct, `docs/explicacionv1.md` §9); un estado separado
aísla el peso nuevo y **protege lo ya conseguido**. `credentials.c` (que vive en `educhain_state_t`)
leería del estado de requests mediante una **función de consulta de solo lectura** (`assigns
\nothing`), sin reintroducir el framing (§4.5, §8-bis).

**Qué se conserva intacto de la verificación formal:** `security.c` (114/114, todo Qed/Alt-Ergo),
`audit.c` (append-only + `seq` creciente demostrado a nivel de operación), y todos los `ensures`
hoy cerrados de los módulos existentes — porque **no se edita ninguno de esos archivos** (salvo la
ampliación aditiva de `security_can`/`role_can`, §9).

A continuación, las 11 secciones solicitadas, actualizadas con los ajustes.

---

## 1. Diagnóstico de la arquitectura actual

EduChain es un modelo en C (C99) de la lógica de un contrato inteligente; **no es una blockchain
real, no usa criptografía y no maneja datos personales** (`include/common.h`;
`docs/explicacionv1.md` §1). Todo el estado vive en un único `educhain_state_t` (arreglos de tamaño
fijo + un contador por arreglo), de ~7 KB (`include/common.h`, struct `educhain_state_t`).

Cinco módulos, cada uno con cabecera en `include/` e implementación en `src/`:

| Módulo | Archivo | Rol en la arquitectura |
|--------|---------|------------------------|
| `security` | `src/security.c`, `include/security.h` | **Punto único de autorización.** `security_can(role, op)` es un `switch` puro, sin estado ni bucles. |
| `professionals` | `src/professionals.c`, `include/professionals.h` | Registro de profesionales (`pros[]`). |
| `experience` | `src/experience.c`, `include/experience.h` | Registro y validación de experiencia (`exps[]`). Máquina mínima `EXP_PENDING → EXP_VALIDATED`. |
| `credentials` | `src/credentials.c`, `include/credentials.h` | Credencial como máquina de estados vía `cred_transition` genérica. |
| `audit` | `src/audit.c`, `include/audit.h` | Log append-only, `seq` estrictamente creciente. |

**Patrones clave que el flujo nuevo debe reutilizar (no reinventar):**

1. **"Validar TODO antes de mutar NADA"** — `experience_register` / `credential_issue`
   (`src/experience.c`, `src/credentials.c`) validan ids, permiso, existencia, duplicados y capacidad
   **antes** de escribir. Garantiza atomicidad (BR-10): en cualquier error los contadores no cambian.
2. **Decisor único** — toda función de escritura consulta `security_can(caller_role, OP_*)` antes de
   mutar (`if (!security_can(...)) return ERR_PERM;`). No hay segunda ruta de autorización.
3. **`actor_id = caller_id`, nunca el titular** — `experience_register`, `experience_validate`,
   `credential_issue`, `cred_transition` y `credential_revoke` auditan siempre con `caller_id`
   (quien ejecuta), con comentarios explícitos ("actor_id = caller_id ... NO el titular owner_id",
   `src/credentials.c`). Es exactamente la propiedad 12 pedida.
4. **No autovalidación** — `experience_validate` (`src/experience.c`) rechaza con `ERR_SELF_VALIDATE`
   cuando `st->exps[idx].owner_id == caller_id`, **con independencia del rol**. Es el modelo directo
   de "ningún solicitante aprueba su propia solicitud".
5. **Máquina de estados genérica** — `cred_transition(st, role, caller_id, op, cred_id, from, to)`
   (`src/credentials.c`) valida rango de ids → permiso → existencia → **estado origen exacto** → hueco
   de log, y solo entonces muta. Es la plantilla para PENDING→VERIFIED/REJECTED→RECORDED.
6. **Consulta de solo lectura con `assigns \nothing`** — `experience_find`, `cred_find`,
   `cred_find_by_owner`, `credential_status`, `professional_is_active` son búsquedas lineales con
   `loop invariant` + `loop variant` y `assigns \nothing`; **cierran limpio en WP** y no sufren el
   framing del struct (no llaman a `audit_record_event`). Este patrón es la base de la integración
   `credentials → requests` (§4.5, ajuste 3) sin reintroducir el problema de framing.

**`ROLE_VERIFIER` hoy existe pero no puede escribir nada:** `security_can` no tiene ninguna rama que
lo autorice (`src/security.c`, `default: return 0;`), y el contrato de `security.h` lo fija con un
`ensures (caller_role == ROLE_VERIFIER) ==> (\result == 0)`, reforzado por el predicado
`role_can` (`verification/specifications.h`, cláusula `r == ROLE_VERIFIER ==> \false`). Es decir, hoy
VERIFIER es un rol *reservado y demostrado inerte*: un asiento vacío que el flujo nuevo llenará.

---

## 2. Inventario de funciones y archivos afectados

Funciones y archivos reales del repo, clasificados por el impacto que tendría la implementación.

### 2.1 Se LEEN como patrón, NO se modifican

| Archivo | Símbolos | Para qué sirven de patrón |
|---------|----------|---------------------------|
| `src/security.c` | `security_can` | Plantilla del `switch` de permisos; se le añaden **casos nuevos** (OPs nuevas), sin tocar los existentes. |
| `src/experience.c` | `experience_validate`, `experience_find` | `ERR_SELF_VALIDATE` (no autovalidación) y búsqueda lineal con invariante de bucle. |
| `src/credentials.c` | `cred_transition`, `cred_find`, `cred_find_by_owner`, `credential_status` | Transición genérica con estado origen exacto; **consulta `assigns \nothing`** (modelo para la lectura `credentials → requests`). |
| `src/professionals.c` | `professional_register`, `professional_is_active` | Inserción atómica + `assert 0 <= n < MAX`; consulta `assigns \nothing`. |
| `src/audit.c` | `audit_record_event`, `audit_verify_event` | Append-only + `seq` creciente; el nuevo flujo audita igual. |
| `verification/specifications.h` | `predicate role_can` | Se **extiende** con las OPs nuevas (ver §8); no se reescribe lo existente. |
| `ui/educhain_cli.c` | `role_name`, `op_name`, `do_*` | Consume `role_t`/`op_code_t`/funciones públicas; prohibido modificarlo en esta etapa. |

### 2.2 Se MODIFICARÍAN (en etapas posteriores, no ahora)

| Archivo | Cambio previsto | Naturaleza del cambio |
|---------|-----------------|------------------------|
| `include/common.h` | Añadir `ROLE_INSTITUTION`, alias `ROLE_PROFESSOR`, nuevas `OP_*`, enums `request_state_t`/`request_category_t`, struct `request_t`, struct `requests_state_t` (arreglos `reqs[]`, `auth[]` + contadores) y nuevos `ERR_*`. | **Aditivo y append-only de enums.** El estado nuevo va en `requests_state_t` **separado**, NO dentro de `educhain_state_t` (§4.5, §10-D5). |
| `src/security.c` / `include/security.h` | Casos nuevos en `security_can` + cláusulas bicondicionales nuevas en el contrato. | Aditivo: no altera las ramas probadas. |
| `verification/specifications.h` | Cláusulas nuevas de `role_can` para las OPs del flujo. | Aditivo. |

### 2.3 Se CREARÍAN

| Archivo | Contenido |
|---------|-----------|
| `src/requests.c` | Lógica del flujo: `request_submit`, `request_endorse`, `request_verify`, `request_reject`, `request_record`, consultas (`request_find`, `request_status`, `request_is_recorded`, `request_recorded_count`). |
| `include/requests.h` | Prototipos + contratos ACSL del módulo. Define `requests_state_t`. |
| `tests/test_requests.c` | Pruebas unitarias y adversariales del flujo (patrón `tests/test_attacks.c`). |

### 2.4 Lo que NO se elimina (y por qué)

Ninguna función ni módulo se elimina. `experience_validate` y `credential_*` **permanecen** pero su
acceso al "historial oficial nuevo" queda cerrado por aislamiento de estado + ausencia de ruta de
escritura (ver §3, §7 y el **invariante transversal** de §8-bis): escriben en `exps[]`/`creds[]`
(dentro de `educhain_state_t`), nunca en `reqs[]` (dentro de `requests_state_t`). Eliminarlos rompería
`tests/*` (99/99) y la CLI, y destruiría especificaciones ya demostradas, contra la restricción dura.

---

## 3. Nuevo modelo de actores y permisos

### 3.1 Actores

| Actor | Enum propuesto | Origen |
|-------|----------------|--------|
| ADMIN | `ROLE_ADMIN` (= 0, sin cambio) | existente |
| INSTITUTION | `ROLE_INSTITUTION` (valor **nuevo**, p. ej. 5) | nuevo |
| VERIFIER | `ROLE_VERIFIER` (= 4, sin cambio, **ahora con escritura en el flujo nuevo**) | existente, reactivado |
| PROFESSOR | `ROLE_PRO` (= 3) usado como PROFESSOR; alias opcional `ROLE_PROFESSOR` | existente |

**Por qué no renumerar:** la CLI (`ui/educhain_cli.c`) y los tests comparan contra los valores
literales. Cambiar los valores numéricos existentes rompería esa capa. Se **añaden** valores al final
del enum (append-only), táctica idéntica a cómo ya conviven las OPs.

### 3.2 `caller_id` vs `institution_id` y la autorización de representación (ajuste 2)

**Problema que resuelve el ajuste.** El diseño anterior asumía implícitamente que una INSTITUTION
presenta "en su ámbito" verificando `caller_id == institution_id`. Eso **confunde** dos identidades
distintas:

- **`caller_id`** — identificador del **usuario que ejecuta** la operación (autenticado por la capa
  externa; se audita como `actor_id`, igual que hoy en todos los módulos).
- **`institution_id`** — identificador de la **institución representada** por esa solicitud.

Un usuario (p. ej. un funcionario con `caller_id = 7001`) actúa **en nombre de** una institución
(`institution_id = 300`). **No** son el mismo número y no deben igualarse. Se necesita modelar
explícitamente la **relación de autorización usuario → institución**.

**Modelo de la relación (dato, no código nuevo de autenticación).** Se añade a `requests_state_t`
una tabla de vínculos de representación, con el mismo patrón de arreglo fijo + contador + búsqueda
lineal `assigns \nothing` que ya usa todo el repo:

```c
typedef struct {
    int user_id;         /* caller_id autorizado a representar...        */
    int institution_id;  /* ...a esta institución                        */
    int active;          /* 0/1 (permite revocar un vínculo sin borrar)  */
} inst_authz_t;
```

```c
inst_authz_t auth[MAX_AUTHZ];  int auth_count;   /* MAX_AUTHZ p. ej. 64 */
```

Y una consulta de solo lectura (patrón `cred_find_by_owner`, `assigns \nothing`):

```c
/* 1 si existe un vínculo activo user_id -> institution_id; 0 si no. */
int requests_is_representative(const requests_state_t *rs, int user_id, int institution_id);
```

**Comprobación en `request_submit`.** Cuando el presentador es INSTITUTION, la autorización ya no es
`caller_id == institution_id` sino:

```
if (role == ROLE_INSTITUTION &&
    !requests_is_representative(rs, caller_id, institution_id))  return ERR_PERM;
```

Para PROFESSOR se mantiene la regla propia existente (`caller_id == owner_id`), análoga a
`experience_register` (`caller_role == ROLE_PRO && caller_id != owner_id ==> ERR_PERM`).

**Quién puebla `auth[]` (Rev 3 — DECIDIDO: ADMIN en runtime).** La Revisión 2 dejaba abierto si
ADMIN gestionaba los vínculos en runtime o si la tabla se precargaba. **La Revisión 3 cierra la
decisión: ADMIN gestiona `auth[]` en runtime** mediante las tres operaciones de §3.2 bis. La tabla
puede además precargarse en el escenario inicial, pero la vía normativa es la operación auditada.

Esta separación se eleva a **propiedad de seguridad P-13** (§8), análoga a la no-autovalidación.

### 3.2 bis — Operaciones de gestión de `auth[]` por ADMIN (ajuste B, Rev 3)

Se diseñan **tres operaciones nuevas** sobre `requests_state_t.auth[]`. Todas distinguen `caller_id`
(el ADMIN que ejecuta, auditado como `actor_id`) del par `(user_id, institution_id)` que se gestiona.

```c
/* Autoriza a user_id a representar a institution_id. ADMIN exclusivo. */
int authz_grant (educhain_state_t *st, requests_state_t *rs,
                 role_t caller_role, int caller_id, int user_id, int institution_id);
/* Revoca la autorización: marca active=0 SIN borrar el registro (trazabilidad). ADMIN exclusivo. */
int authz_revoke(educhain_state_t *st, requests_state_t *rs,
                 role_t caller_role, int caller_id, int user_id, int institution_id);
/* Consulta de solo lectura: 1 si hay vínculo ACTIVO user_id -> institution_id; 0 si no. */
int requests_is_representative(const requests_state_t *rs, int user_id, int institution_id);
```

`requests_is_representative` es el **gemelo exacto** de `cred_find_by_owner` (`src/credentials.c`):
búsqueda lineal con `loop invariant`/`loop variant` y `assigns \nothing`; **cierra en WP** (patrón ya
probado). Es la consulta de "consultar si una autorización está activa" pedida en el ajuste B.

**OPs de auditoría nuevas:** `OP_GRANT_REPRESENTATION`, `OP_REVOKE_REPRESENTATION` (append-only al
enum `op_code_t` de `include/common.h`, que hoy termina en `OP_REVOKE_CRED = 8`). `security_can`
concede ambas **solo a `ROLE_ADMIN`** (rama nueva del `switch`, misma forma pura que las existentes).

**Estados del registro de autorización.** `inst_authz_t.active ∈ {0,1}`:

| `active` | Significado | Cómo se alcanza |
|:--------:|-------------|-----------------|
| `1` | Vínculo ACTIVO: `user_id` puede representar a `institution_id`. | `authz_grant` con éxito. |
| `0` | Vínculo INACTIVO (revocado): se conserva para trazabilidad, no habilita operar. | `authz_revoke` sobre un vínculo activo. |

**Reglas de las operaciones (todas comprobadas en el cuerpo antes de mutar, patrón "validar todo
antes de mutar nada"):**

1. **ADMIN exclusivo.** `if (!security_can(caller_role, OP_GRANT_REPRESENTATION)) return ERR_PERM;`
   (ídem revoke). `security_can` solo responde 1 para `ROLE_ADMIN`.
2. **Sin duplicados de autorizaciones ACTIVAS.** `authz_grant` rechaza con `ERR_DUPLICATE` si ya
   existe un `auth[k]` con `user_id == user_id && institution_id == institution_id && active == 1`.
   Si existe el mismo par pero `active == 0`, el diseño **reactiva** ese registro (`active = 1`) en
   lugar de insertar uno nuevo, para no multiplicar filas del mismo par (evita además acercarse a
   `MAX_AUTHZ` con duplicados lógicos). Alternativa simétrica — insertar siempre una fila nueva — se
   descarta: complicaría la invariante "a lo sumo un activo por par" en la que se apoya P-13.
3. **La revocación NO borra.** `authz_revoke` localiza el vínculo activo y pone `active = 0`; nunca
   decrementa `auth_count` ni compacta el arreglo. Si no hay vínculo activo para ese par devuelve
   `ERR_NOT_FOUND`. Así la traza "existió esta autorización" se conserva.
4. **La revocación NO es retroactiva.** `active = 0` impide **nuevas** `request_submit`/
   `request_endorse` en nombre de esa institución (porque `requests_is_representative` devolverá 0),
   pero **no** altera ninguna solicitud ya presentada: `authz_revoke` no toca `reqs[]` (su `assigns`
   no lo nombra). Las solicitudes previas conservan su `submitter_id`/`endorser_id`/`state`.
5. **Capacidad y atomicidad.** `authz_grant` que necesita insertar y encuentra `auth_count >=
   MAX_AUTHZ` devuelve `ERR_FULL` sin mutar nada; igual que el hueco de log (`ERR_FULL` si
   `log_count >= MAX_EVENTS`). En cualquier error, `auth_count` y `auth[]` quedan intactos (BR-10).

**Errores alcanzables (Rev 4 — corregidos).** `ERR_BAD_ID` (rango de `caller_id`/`user_id`/
`institution_id`), `ERR_PERM` (no ADMIN), `ERR_DUPLICATE` (grant de un activo ya existente),
`ERR_NOT_FOUND` (revoke sin activo), `ERR_FULL` (`auth[]` llena o log lleno) y `EDU_OK`. **`ERR_NULL`
NO es un resultado alcanzable bajo el contrato** y por eso NO figura en el `ensures` de resultados:
igual que `credential_issue`/`cred_transition`/`experience_validate` (`src/credentials.c`,
`src/experience.c`), el cuerpo conserva un `if (st==0) return ERR_NULL;` defensivo, pero los
`requires \valid(st) && \valid(rs)` hacen esa rama **inalcanzable** para WP, de modo que incluir
`ERR_NULL` en la postcondición sería un resultado que el probador nunca podría alcanzar. Todos los
códigos existen ya en `include/common.h`.

**Contrato ACSL propuesto de `authz_grant` (Rev 4 — partido por `behavior`).** Se parte en cuatro
comportamientos `complete`+`disjoint` que diferencian **inserción nueva**, **reactivación**,
**duplicado** y **capacidad llena**, siguiendo el patrón `space`/`no_space` de `credential_issue`
(que existe precisamente para no nombrar `creds[MAX]` fuera de rango). El `assigns` **nunca** nombra
`rs->auth[rs->auth_count]` de forma incondicional: solo el comportamiento que inserta (con hueco
garantizado) escribe ese slot y `auth_count`; el que reactiva escribe solo un `active`; los de error
no escriben nada del estado de `auth[]`.

```c
/*@ requires \valid(st) && \valid(rs);
    requires 0 <= rs->auth_count <= MAX_AUTHZ;
    requires 0 <= st->log_count  <= MAX_EVENTS;
    // ERR_NULL NO aparece: \valid(st)&&\valid(rs) lo hacen inalcanzable (patrón credential_issue).
    ensures \result == EDU_OK || \result == ERR_PERM || \result == ERR_BAD_ID
         || \result == ERR_DUPLICATE || \result == ERR_FULL;
    // P-13 (mitad negativa, se espera demostrar en WP): rol no-ADMIN => ERR_PERM o ERR_BAD_ID.
    // Admite ERR_BAD_ID porque el rango de ids se chequea ANTES que el permiso (patrón
    // cred_transition: `!role_can(...) ==> (\result==ERR_PERM || \result==ERR_BAD_ID)`).
    ensures !role_can(caller_role, OP_GRANT_REPRESENTATION)
            ==> (\result == ERR_PERM || \result == ERR_BAD_ID);
    // El conteo solo puede crecer en 1 (inserción) o quedar igual (reactivación/error): atomicidad.
    ensures \result == EDU_OK ==> (rs->auth_count == \old(rs->auth_count)
                                || rs->auth_count == \old(rs->auth_count) + 1);
    ensures \result != EDU_OK ==> rs->auth_count == \old(rs->auth_count);

    // (1) INSERCIÓN NUEVA: no existe fila con ese par (ni activa ni inactiva) y hay hueco.
    behavior insert:
      assumes !(\exists integer k; 0 <= k < rs->auth_count
                && rs->auth[k].user_id == user_id && rs->auth[k].institution_id == institution_id);
      assumes rs->auth_count < MAX_AUTHZ && st->log_count < MAX_EVENTS;
      assigns rs->auth[rs->auth_count], rs->auth_count,
              st->log[st->log_count], st->log_count;
    // (2) REACTIVACIÓN: existe la fila del par pero inactiva (active==0) -> pasa a active==1.
    behavior reactivate:
      assumes (\exists integer k; 0 <= k < rs->auth_count
               && rs->auth[k].user_id == user_id && rs->auth[k].institution_id == institution_id
               && rs->auth[k].active == 0);
      assumes st->log_count < MAX_EVENTS;
      assigns { rs->auth[k].active | integer k;
                0 <= k < rs->auth_count && rs->auth[k].user_id == user_id
                && rs->auth[k].institution_id == institution_id },
              st->log[st->log_count], st->log_count;
    // (3) DUPLICADO: ya existe una fila ACTIVA del par -> ERR_DUPLICATE, no se duplica ni se muta.
    behavior duplicate:
      assumes (\exists integer k; 0 <= k < rs->auth_count
               && rs->auth[k].user_id == user_id && rs->auth[k].institution_id == institution_id
               && rs->auth[k].active == 1);
      assigns \nothing;
      ensures \result == ERR_DUPLICATE || \result == ERR_PERM || \result == ERR_BAD_ID;
    // (4) CAPACIDAD LLENA: inserción necesaria pero sin hueco (o log lleno) -> ERR_FULL, assigns \nothing.
    //     Aquí NO se nombra auth[auth_count] (evita el acceso fuera de rango que el usuario señaló).
    behavior full:
      assumes !(\exists integer k; 0 <= k < rs->auth_count
                && rs->auth[k].user_id == user_id && rs->auth[k].institution_id == institution_id);
      assumes rs->auth_count >= MAX_AUTHZ || st->log_count >= MAX_EVENTS;
      assigns \nothing;
      ensures \result == ERR_FULL || \result == ERR_PERM || \result == ERR_BAD_ID;
    complete behaviors;
    disjoint behaviors insert, reactivate, duplicate, full;
*/
int authz_grant(educhain_state_t *st, requests_state_t *rs,
                role_t caller_role, int caller_id, int user_id, int institution_id);
```

> **Nota de solapamiento rol/estado.** Los `behaviors` se separan por el **estado de `auth[]`**
> (existencia y `active` del par, más el hueco), no por el rol. Por eso cada comportamiento de
> error admite además `ERR_PERM`/`ERR_BAD_ID`: el chequeo de permiso e ids **precede** en el cuerpo al
> examen de `auth[]`, exactamente como `cred_transition` antepone `ERR_BAD_ID`/`ERR_PERM`. Así
> `complete`+`disjoint` se mantienen (las `assumes` particionan el estado de `auth[]`) sin volver
> insatisfacibles las ramas de autorización.

**Contrato ACSL propuesto de `authz_revoke` (Rev 4).** Dos comportamientos `complete`+`disjoint`:
**revocación efectiva** (existe un activo del par → pasa a `active==0`) y **sin activo**
(`ERR_NOT_FOUND`, `assigns \nothing`). Nunca toca `auth_count` (la revocación no borra) ni `reqs[]`.
`ERR_NULL` tampoco aparece como resultado (mismo motivo que `authz_grant`).

```c
/*@ requires \valid(st) && \valid(rs);
    requires 0 <= rs->auth_count <= MAX_AUTHZ;
    requires 0 <= st->log_count  <= MAX_EVENTS;
    ensures \result == EDU_OK || \result == ERR_PERM || \result == ERR_BAD_ID
         || \result == ERR_NOT_FOUND || \result == ERR_FULL;
    ensures !role_can(caller_role, OP_REVOKE_REPRESENTATION)
            ==> (\result == ERR_PERM || \result == ERR_BAD_ID);
    ensures rs->auth_count == \old(rs->auth_count);   // revocar NUNCA decrementa el contador
    // En éxito, el par queda inactivo (se espera WP sobre el slot; existencial global parcial por (G)).
    ensures \result == EDU_OK ==>
            (\exists integer k; 0 <= k < rs->auth_count
             && rs->auth[k].user_id == user_id && rs->auth[k].institution_id == institution_id
             && rs->auth[k].active == 0);
    behavior revoke_active:
      assumes (\exists integer k; 0 <= k < rs->auth_count
               && rs->auth[k].user_id == user_id && rs->auth[k].institution_id == institution_id
               && rs->auth[k].active == 1);
      assumes st->log_count < MAX_EVENTS;
      assigns { rs->auth[k].active | integer k;
                0 <= k < rs->auth_count && rs->auth[k].user_id == user_id
                && rs->auth[k].institution_id == institution_id },
              st->log[st->log_count], st->log_count;
    behavior no_active:
      assumes !(\exists integer k; 0 <= k < rs->auth_count
                && rs->auth[k].user_id == user_id && rs->auth[k].institution_id == institution_id
                && rs->auth[k].active == 1);
      assigns \nothing;
      ensures \result == ERR_NOT_FOUND || \result == ERR_PERM || \result == ERR_BAD_ID;
    complete behaviors;
    disjoint behaviors revoke_active, no_active;
*/
int authz_revoke(educhain_state_t *st, requests_state_t *rs,
                 role_t caller_role, int caller_id, int user_id, int institution_id);
```

**Estado esperado de verificación (aspiracional, no obtenido).** Se **espera** que WP cierre: la
mitad negativa de P-13 (`!role_can ==> ERR_PERM||ERR_BAD_ID`, implicación local, misma clase que P-2
de `experience_validate` y la mitad negativa de P-8 de `credential_revoke`); los `assigns` partidos
por `behavior` (patrón ya probado en `credential_issue`/`credential_revoke`); y la atomicidad del
contador (`auth_count` intacto en error, invariante en revoke). El **existencial positivo global** de
éxito (que exista `k` activo/inactivo para el par) cae en la limitación (G) (ausencia de invariante
global de unicidad), igual que el dual positivo de P-6; se deja como objetivo **parcial** con respaldo
dinámico en `tests/test_requests.c`. Nada de esto está demostrado todavía: la implementación y la
re-ejecución de WP·Eva son Etapa 3+. El `assigns` que acopla `auth[k].active` y `log[...]` reproduce
el acoplamiento estado+log de `cred_transition`, cuyo perfil en WP ya se conoce.

### 3.3 Matriz rol × operación (flujo nuevo)

Operaciones nuevas: `OP_SUBMIT_REQUEST`, `OP_ENDORSE_REQUEST` (respaldo institucional, §5),
`OP_VERIFY_REQUEST`, `OP_REJECT_REQUEST`, `OP_RECORD_REQUEST` (incorporar, con las dos variantes de
§5.5), y `OP_GRANT_REPRESENTATION` / `OP_REVOKE_REPRESENTATION` (gestión de `auth[]`, §3.2 bis).

| Operación | ADMIN | INSTITUTION | VERIFIER | PROFESSOR | HR | COORD |
|-----------|:-----:|:-----------:|:--------:|:---------:|:--:|:-----:|
| `OP_SUBMIT_REQUEST` | ✗ | ✓ (institución que representa) | ✗ | ✓ (solo propias) | ✗ | ✗ |
| `OP_ENDORSE_REQUEST` (respaldo) | ✗ | ✓ (institución que representa) | ✗ | ✗ | ✗ | ✗ |
| `OP_VERIFY_REQUEST` (aprobar) | ✗ | ✗ | ✓ (no las propias) | ✗ | ✗ | ✗ |
| `OP_REJECT_REQUEST` | ✗ | ✗ | ✓ (no las propias) | ✗ | ✗ | ✗ |
| `OP_RECORD_REQUEST` (incorporar / sustituir) | ✓ | ✗ | ✗ | ✗ | ✗ | ✗ |
| `OP_GRANT_REPRESENTATION` (autorizar) | ✓ | ✗ | ✗ | ✗ | ✗ | ✗ |
| `OP_REVOKE_REPRESENTATION` (revocar) | ✓ | ✗ | ✗ | ✗ | ✗ | ✗ |
| Consultar estado/trazabilidad (capa pública, detalle en **§3.3 bis**) | ✓ (todo) | ✓ (solo lo que la involucra) | ✓ (pendientes/en proceso) | ✓ (solo propio) | ✓ (RO, solo circuito heredado) | ✓ (RO, solo circuito heredado) |

**Nota (Rev 3):** `OP_RECORD_REQUEST` cubre tanto la incorporación simple como la
incorporación-con-sustitución (corrección con `prev_id`); no hay OP separada para sustituir, porque la
sustitución del registro anterior es un **efecto atómico** de incorporar la corrección (§5.5, §6).

**Clave de la separación estricta:** HR y COORD aparecen con **✗ en todas las operaciones de
escritura del flujo nuevo**. Conservan sus permisos únicamente sobre el circuito heredado
(`OP_REGISTER_PRO`, `OP_VALIDATE_EXP`, `OP_*_CRED`), que **no** es "historial oficial de solicitudes".
Así ningún rol tiene dos caminos al nuevo historial oficial: solo ADMIN incorpora, y solo tras VERIFIER.

### 3.3 bis — Consultas: primitivas internas vs capa pública por rol/identidad/ámbito (Rev 4)

La fila «Consultar estado/trazabilidad» de §3.3 era demasiado gruesa. La Rev 4 separa **dos niveles**
de consulta, porque hoy `request_find`/`request_status`/`request_is_recorded`/`request_recorded_count`
(§7.1) son, por forma, **primitivas de solo lectura sin chequeo de rol** (gemelas de
`experience_find`/`credential_status`, `assigns \nothing`): devuelven el dato a quien tenga el puntero
al estado, sin mirar quién pregunta.

**Decisión Rev 4:** esas funciones son **primitivas internas del módulo** (uso interno de `requests.c`
y de `credentials.c` para la elegibilidad, §8-ter). **No** se exponen directamente a un usuario final;
exponerlas tal cual violaría el principio de ámbito (un PROFESSOR podría leer solicitudes ajenas). Por
encima de ellas se diseña una **capa de consulta pública** que sí comprueba **rol + identidad +
ámbito** antes de devolver nada.

**Primitivas internas (sin chequeo de rol, `assigns \nothing`):** `request_find`, `request_status`,
`request_is_recorded`, `request_recorded_count`, `requests_is_representative`,
`request_requires_endorsement`. Contrato: el ya descrito en §7.1/§8-ter. Son el patrón
`experience_find`/`credential_status` y **se espera** que cierren en WP igual que ellas.

**Capa de consulta pública (con rol + identidad + ámbito).** Toma `caller_role` y `caller_id`, aplica
la matriz de abajo y delega en las primitivas solo para el ámbito permitido. Firmas propuestas:

```c
/* Estado de una solicitud, solo si el llamador puede verla según su rol/ámbito; si no, ERR_PERM. */
int request_status_for(const requests_state_t *rs, role_t caller_role, int caller_id, int req_id);
/* ¿req_id está en el historial vigente? Igual control de ámbito que request_status_for. */
int request_is_recorded_for(const requests_state_t *rs, role_t caller_role, int caller_id, int req_id);
/* Nº de registros vigentes de owner_id, solo si el llamador puede consultarlo (p.ej. el propio
 * PROFESSOR, o ADMIN); si no, ERR_PERM. */
int request_recorded_count_for(const requests_state_t *rs, role_t caller_role,
                               int caller_id, int owner_id);
```

**Matriz de permisos de CONSULTA (distingue los cuatro actores, Rev 4):**

| Consulta pública | ADMIN | INSTITUTION | VERIFIER | PROFESSOR | HR / COORD |
|------------------|:-----:|:-----------:|:--------:|:---------:|:----------:|
| Ver una solicitud concreta (`request_status_for`) | ✓ todas | ✓ solo las que la involucran (`institution_id` que representa, §3.2, o `endorser_id`) | ✓ las pendientes/en proceso (`PENDING`/`ENDORSED`/`VERIFIED`) | ✓ solo las propias (`owner_id==caller_id` o `submitter_id==caller_id`) | ✗ (flujo nuevo) |
| ¿Está en historial vigente? (`request_is_recorded_for`) | ✓ todas | ✓ ámbito propio | ✓ (lo ya tramitado) | ✓ propias | ✗ |
| Conteo vigente de un titular (`request_recorded_count_for`) | ✓ cualquiera | ✗ (no es su ámbito) | ✗ | ✓ solo el propio `owner_id` | ✗ |
| Auditoría / historial completo | ✓ | ✗ | ✗ | ✗ | ✓ RO (solo del circuito heredado, no de `reqs[]`) |

**Principio (P-SCOPE, Rev 4):** ningún usuario consulta información fuera de su ámbito autorizado.
Un PROFESSOR solo ve sus propias solicitudes; una INSTITUTION solo las que la involucran; el VERIFIER
las que están en tramitación; ADMIN el historial y la auditoría. La capa pública devuelve `ERR_PERM`
cuando la solicitud cae fuera del ámbito del llamador, **antes** de revelar cualquier campo.

**Contrato ACSL propuesto de la capa pública (forma).** Mitad negativa demostrable + `assigns
\nothing`:

```c
/*@ requires \valid_read(rs);
    requires 0 <= rs->reqs_count <= MAX_REQUESTS;
    assigns \nothing;
    // Fuera de ámbito => ERR_PERM, sin revelar estado (mitad negativa, se espera WP).
    ensures (caller_role == ROLE_PRO
             && !(\exists integer k; 0 <= k < rs->reqs_count && rs->reqs[k].id == req_id
                  && (rs->reqs[k].owner_id == caller_id || rs->reqs[k].submitter_id == caller_id)))
            ==> \result == ERR_PERM;
    ensures \result == ERR_PERM || \result == ERR_BAD_ID
         || \result == -1 || (0 <= \result);   // -1 = no existe dentro del ámbito
*/
int request_status_for(const requests_state_t *rs, role_t caller_role, int caller_id, int req_id);
```

**Estado esperado (aspiracional).** Se **espera** que WP cierre la mitad negativa (fuera de ámbito ⇒
`ERR_PERM`) y el `assigns \nothing` —implicación local sobre una consulta pura, misma clase que P-1 /
la mitad negativa de P-2—; el caso positivo «devuelve exactamente el estado del slot que existe y está
en ámbito» depende del existencial de unicidad y queda **parcial por (G)**, con respaldo dinámico en
`tests/test_requests.c` (un PROFESSOR intenta leer una solicitud ajena → `ERR_PERM`). Nada demostrado
aún (Etapa 3+).

### 3.4 Restricciones intra-rol (verificadas en el cuerpo, no solo por matriz)

- **PROFESSOR solo presenta lo suyo:** `caller_id == owner_id` (patrón `experience_register`).
- **INSTITUTION solo representa lo autorizado:** `requests_is_representative(rs, caller_id,
  institution_id)` (§3.2), **no** `caller_id == institution_id`.
- **Ningún solicitante aprueba lo propio:** en `request_verify`/`request_reject`, si el
  `submitter_id` de la solicitud `== caller_id` → `ERR_SELF_VALIDATE` (patrón `experience_validate`).

---

## 4. Estructuras de datos propuestas

### 4.1 Enums nuevos (en `include/common.h`, aditivos)

```c
typedef enum {
    REQ_PENDING    = 0,   /* recién creada, a la espera de verificación           */
    REQ_ENDORSED   = 1,   /* respaldada por su institución (solo categorías que lo exigen, §5) */
    REQ_VERIFIED   = 2,   /* aprobada por VERIFIER, lista para incorporar          */
    REQ_REJECTED   = 3,   /* rechazada por VERIFIER (terminal)                     */
    REQ_RECORDED   = 4,   /* incorporada al historial oficial vigente por ADMIN   */
    REQ_SUPERSEDED = 5    /* era RECORDED y fue sustituida por una corrección (terminal, Rev 3) */
} request_state_t;

typedef enum {          /* 14 categorías del requisito */
    CAT_INSTITUTION_WORKED = 0, CAT_LINK_PERIOD, CAT_LINK_TYPE_DEDICATION,
    CAT_COURSES_TAUGHT, CAT_ACADEMIC_PERIOD, CAT_TEACHING_HOURS,
    CAT_ACADEMIC_ACTIVITY, CAT_RESEARCH_PROJECT, CAT_PUBLICATION,
    CAT_THESIS_SUPERVISION, CAT_OUTREACH, CAT_COMMITTEES,
    CAT_TRAINING, CAT_CERTIFICATION
} request_category_t;   /* 14 valores */
```

> **Cambio respecto a la Revisión 1:** se añade el estado `REQ_ENDORSED` entre `PENDING` y
> `VERIFIED` para soportar el **respaldo institucional condicional** (ajuste 6, §5). Afecta solo a
> las categorías que lo exigen; las demás van directo `PENDING → VERIFIED`.
>
> **Cambio de la Revisión 3 — `REQ_SUPERSEDED` (requisito nuevo).** Se añade el estado terminal
> `REQ_SUPERSEDED`. Es el ÚNICO estado alcanzable **desde `REQ_RECORDED`** y **solo** por efecto
> lateral de `request_record` cuando incorpora una corrección (variante incorporación-con-sustitución,
> §5.5 y §6). Ninguna operación lleva una solicitud a `REQ_SUPERSEDED` de forma directa: no hay
> `OP_*` cuyo destino primario sea este estado. `REQ_SUPERSEDED` conserva TODO el contenido histórico
> del registro (owner, categoría, fechas, `evidence_ref`, metadatos); solo cambia `state`. Deja de
> contar como historial vigente: `request_recorded_count()` y el historial oficial consultan **solo**
> `REQ_RECORDED` (§8-ter). El valor `5` es append-only al final del enum (no renumera los anteriores).

### 4.2 Struct genérico de solicitud con metadatos por categoría (ajuste 1)

El usuario pide conservar `request_t` genérico **pero** poder representar metadatos específicos
(horas docentes **asignadas** y **realizadas**, cursos, períodos académicos) **sin** un campo distinto
por cada una de las 14 categorías. Solución: un **conjunto acotado de campos numéricos genéricos**
reinterpretados según `category`.

```c
typedef struct {
    int                id;            /* id único de la solicitud (ID_MIN..ID_MAX)     */
    int                owner_id;      /* profesor titular (a quién acredita)           */
    int                institution_id;/* institución representada (0 si no aplica)     */
    int                submitter_id;  /* presentador real (= caller_id que la creó)    */
    int                verifier_id;   /* 0 si no verificada; != submitter si verificada*/
    request_category_t category;      /* una de las 14 categorías                      */
    request_state_t    state;         /* PENDING/ENDORSED/VERIFIED/REJECTED/RECORDED   */
    int                start_date;    /* YYYYMMDD, reutiliza DATE_MIN..DATE_MAX        */
    int                end_date;      /* YYYYMMDD, >= start_date                        */
    int                evidence_ref;  /* referencia OPACA a evidencia externa (§4.4, §10.3) */
    int                prev_id;       /* id del registro que corrige; 0 = original. Lo fija a 0
                                        request_submit; lo fija !=0 request_submit_correction (§5.5) */
    int                endorser_id;   /* institución que respaldó (0 si sin respaldo)  */
    /* ---- Metadatos genéricos reinterpretados por `category` (ajuste 1) ---- */
    int                num_a;         /* entero genérico A                             */
    int                num_b;         /* entero genérico B                             */
    int                num_c;         /* entero genérico C                             */
    int                num_d;         /* entero genérico D                             */
} request_t;
```

**Cuatro campos numéricos genéricos** (`num_a..num_d`) cubren los metadatos pedidos sin inflar el
struct a 14 variantes. La interpretación la fija `category` (es documentación + validación en el
cuerpo de `request_submit`; no cambia el layout):

| Categoría | `num_a` | `num_b` | `num_c` | `num_d` | (`start/end_date` ya existen) |
|-----------|---------|---------|---------|---------|-------------------------------|
| `CAT_TEACHING_HOURS` | horas **asignadas** | horas **realizadas** | código de curso | período académico (p. ej. `AAAAP`) | duración del período |
| `CAT_COURSES_TAUGHT` | código de curso | nº de créditos | nº de grupos | período académico | fechas del período |
| `CAT_ACADEMIC_PERIOD` | período académico | modalidad (cód.) | — | — | inicio/fin del período |
| `CAT_LINK_PERIOD` | tipo de vínculo (cód.) | % dedicación | — | — | inicio/fin de contratación |
| `CAT_LINK_TYPE_DEDICATION` | tipo de vínculo (cód.) | % dedicación | — | — | inicio/fin |
| `CAT_INSTITUTION_WORKED` | — | — | — | — | inicio/fin del vínculo |
| `CAT_RESEARCH_PROJECT` | código de proyecto | rol (cód.) | — | — | inicio/fin |
| `CAT_PUBLICATION` | tipo (cód.) | nº de autores | — | — | fecha de publicación |
| `CAT_THESIS_SUPERVISION` | nivel (cód.) | nº de tesis | — | — | fechas |
| `CAT_OUTREACH` / `CAT_COMMITTEES` / `CAT_ACADEMIC_ACTIVITY` | tipo (cód.) | horas/sesiones | — | — | fechas |
| `CAT_TRAINING` / `CAT_CERTIFICATION` | tipo (cód.) | horas/créditos | entidad (cód.) | — | fechas |

Convención: `0` = campo no usado por esa categoría. Las validaciones por categoría (p. ej. "horas
realizadas ≤ asignadas" para `CAT_TEACHING_HOURS`, o rango de `%` de dedicación) se comprueban en
`request_submit` con **comparaciones enteras simples** (sin aritmética pesada, para no cargar WP) y,
donde el usuario lo decida, pueden quedar como validación de negocio documentada.

> **Por qué campos genéricos y no un `union` por categoría:** un `union` complicaría el modelo de
> memoria de WP (análisis de tipos efectivos) y el `assigns`; cuatro `int` planos mantienen el struct
> trivial para el probador. Mantiene la Decisión 4 original (genérico, no 14 structs) y la refuerza.

### 4.3 El arreglo de solicitudes va en un estado SEPARADO (no en `educhain_state_t`)

```c
request_t     reqs[MAX_REQUESTS];   int reqs_count;   /* MAX_REQUESTS = 128 (Rev 3) */
inst_authz_t  auth[MAX_AUTHZ];      int auth_count;   /* MAX_AUTHZ    = 64  (Rev 3) */
```

Estos arreglos NO se añaden a `educhain_state_t`; viven en un `requests_state_t` independiente
(§4.5). Esto es el ajuste 4, decidido en §10 (Decisión 5). **Rev 3:** los límites quedan fijados en
`MAX_REQUESTS = 128` y `MAX_AUTHZ = 64` **para la simulación**; **no** se afirma que sean definitivos
para producción (§10-D6).

### 4.4 `evidence_ref` es OPACO y NO criptográfico (ajuste 5)

`evidence_ref` es un **identificador opaco** (un `int`) que apunta a la evidencia que vive **fuera**
del contrato. Se deja **explícito** en el diseño:

- **NO es un hash criptográfico.** No se calcula SHA-256 ni ninguna función resumen. El proyecto
  excluye criptografía por alcance (`docs/explicacionv1.md` §1).
- **NO garantiza integridad documental.** Que dos evidencias tengan el mismo `evidence_ref` o que el
  documento externo cambie no es detectable por el contrato. Coherente con el principio del usuario:
  "un hash tampoco demuestra veracidad". La veracidad depende de la **autoridad del VERIFIER**, no
  del campo.
- **Lo que SÍ garantiza el contrato:** que la referencia aprobada por el VERIFIER se conserva
  **exacta** hasta la incorporación — el `assigns` de `request_record` **no** incluye `evidence_ref`,
  de modo que la incorporación no puede alterarlo (inmutabilidad del contenido aprobado, P-7).

### 4.5 Estado SEPARADO `requests_state_t` y su tamaño (ajustes 1 y 4)

```c
typedef struct {
    request_t     reqs[MAX_REQUESTS];   int reqs_count;
    inst_authz_t  auth[MAX_AUTHZ];      int auth_count;
} requests_state_t;
```

El historial de auditoría del flujo nuevo **sigue usando el `log[]` de `educhain_state_t`** (un solo
historial append-only para todo el sistema, reutilizando `audit_record_event`), por lo que las
funciones de `requests.c` reciben **ambos** estados: `educhain_state_t *st` (para auditar) y
`requests_state_t *rs` (para el flujo). Las consultas de solo lectura reciben solo lo que leen.

**Tamaño estimado y efecto en el framing de WP.** Con los campos de §4.2:

| Componente | Campos | Bytes aprox. |
|------------|-------:|-------------:|
| `request_t` | 17 `int`/enum × 4 B | **~68 B** |
| `reqs[128]` | 128 × 68 B | ~8.7 KB |
| `inst_authz_t` | 3 `int` | 12 B |
| `auth[64]` | 64 × 12 B | ~0.75 KB |
| **`requests_state_t` total** | | **~9.5 KB** |

Comparación con las dos alternativas de estado:

| | `educhain_state_t` (~7 KB) | `requests_state_t` separado (~9.5 KB) | Global ampliado (~16.5 KB) |
|---|---|---|---|
| Framing de WP sobre módulos ya verificados | intacto | **intacto** (no crece) | **degradado**: el struct global que ya es cuello de botella (`docs/explicacionv1.md` §9) crecería ~2.4× |

**Implicación directa (y razón del ajuste 4):** `docs/explicacionv1.md` §9 identifica el framing del
struct ~7 KB a través de `audit_record_event` como una de las dos causas de las obligaciones
Failure/Timeout. Si se ampliara `educhain_state_t` a ~16.5 KB, **las funciones ya verificadas que
llaman a `audit_record_event` verían un struct mayor en su frame-union** y obligaciones hoy cerradas
podrían degradarse a Timeout. Un `requests_state_t` **independiente** aísla el peso nuevo: los
contratos de `experience.c`/`credentials.c`/`professionals.c` siguen razonando sobre el mismo ~7 KB
de antes. Mitigaciones adicionales (igual que lo ya hecho): `MAX_REQUESTS` modesto (128/64) y
`assigns` por set-comprehension acotado a `reqs[k]` con `id == req_id`.

---

## 5. Máquina de estados, flujo y firmas de presentación (unificado, Rev 4)

> **Unificación Rev 4.** Antes, las reglas de nacimiento de la solicitud aparecían repartidas (y con
> riesgo de solaparse) entre la máquina de estados (§5), el flujo completo (§6) y las firmas (§7).
> La Rev 4 fija **una sola fuente de verdad** —las reglas canónicas de §5.0— y hace que §5.1–§5.6, §6
> y §7 **la referencien** en lugar de volver a enunciarla. Las tres secciones describen el mismo
> proceso desde tres ángulos (estados, narrativa paso a paso, firmas), sin reglas divergentes.

### 5.0 Reglas canónicas de nacimiento de la solicitud (ÚNICA fuente, Rev 4)

Estas tres reglas son **idénticas** en la máquina de estados (§5.1, §5.4), en el flujo (§6) y en las
firmas/contratos (§7, P-15 de §8). Cualquier otra sección que mencione el nacimiento remite aquí.

| Quién presenta | ¿La categoría exige respaldo? (`exige_respaldo`, §5.3 bis) | Estado de nacimiento | `endorser_id` | ¿Hace falta `request_endorse`? |
|----------------|:--:|:--:|:--:|:--:|
| **PROFESSOR** (propia, `caller_id==owner_id`) | **Sí** | `REQ_PENDING` | `0` | Sí: una INSTITUTION autorizada ejecuta `request_endorse` (`PENDING→ENDORSED`). |
| **INSTITUTION** (representa a `institution_id`, §3.2) | **Sí** | `REQ_ENDORSED` | `= institution_id` | **No**: su propia presentación constituye el respaldo (§5.3 ter). |
| **PROFESSOR o INSTITUTION** | **No** | `REQ_PENDING` | `0` | No: VERIFIER evalúa directo desde `PENDING`. |

**Dos principios que gobiernan toda la sección:**

1. **El respaldo NUNCA sustituye la verificación del VERIFIER.** `REQ_ENDORSED` es siempre paso
   previo a `REQ_VERIFIED`, jamás un atajo: toda solicitud pasa por `request_verify` (VERIFIER,
   `submitter_id != caller_id`). Un respaldo institucional —propio o vía `request_endorse`— no
   aprueba nada por sí mismo.
2. **Ninguna institución respalda de nuevo su propia presentación.** Cuando la INSTITUTION presenta
   una categoría que exige respaldo, nace `REQ_ENDORSED` en el mismo acto; **no existe** ni se exige
   una segunda operación `request_endorse` sobre esa solicitud. `request_endorse` solo aplica a
   solicitudes que un **PROFESSOR** presentó en `REQ_PENDING` y que esperan respaldo.

### 5.1 Diagrama

```
   submit PROFESSOR (propia)           submit INSTITUTION (representa)
   exige respaldo → PENDING            exige respaldo → ENDORSED (endorser_id=institution_id)
   no exige        → PENDING           no exige        → PENDING
                        │                        │
                        ▼                        ▼
                   ┌─────────┐              ┌──────────┐
                   │ PENDING │─────────────▶│ ENDORSED │   endorse (INSTITUTION que representa,
                   └─────────┘   endorse    └──────────┘   OP_ENDORSE_REQUEST; solo si PROFESSOR
                    │        │                   │         presentó y la categoría exige respaldo)
   no exige respaldo│        │ exige respaldo    │
   verify directo   │        │ (verify bloqueado │ verify (VERIFIER, ≠ submitter)
                    │        │  hasta ENDORSED)   │
         verify ────┤        └───────────────────┤
                    ▼                             ▼
   ┌──────────┐  reject (VERIFIER, ≠ submitter)  ┌──────────┐
   │ REJECTED │◀─────────────────────────────────│ VERIFIED │
   │[terminal]│                                  └──────────┘
   └──────────┘                                       │ record (ADMIN, OP_RECORD_REQUEST)
                                                       ▼
                                                  ┌──────────┐
   corrección de prev_id: en la MISMA operación   │ RECORDED │ [historial oficial VIGENTE]
   record que lleva la nueva a RECORDED, el        └──────────┘
   registro anterior (prev_id) pasa:                    │ record de una corrección cuyo
   RECORDED ──────────────────────────────────────▶    │ prev_id apunta a ESTE registro
                                                        ▼
                                                 ┌──────────────┐
                                                 │ REQ_SUPERSEDED│ [terminal — fuera del historial
                                                 └──────────────┘  vigente; contenido conservado]
```

**`REQ_SUPERSEDED` (Rev 3)** NO tiene arista de entrada propia desde ninguna `OP_*`: se alcanza
**solo** como efecto lateral atómico de un `request_record` que incorpora una corrección (§5.5, §6).
El único origen es `REQ_RECORDED`. Es terminal: toda transición de salida da `ERR_BAD_TRANSITION`.

### 5.2 Sobre `SUBMITTED` vs `PENDING` (decisión conservada)

`SUBMITTED` **NO es un estado propio**; es el acto de creación que deja la solicitud directamente en
`PENDING`. Fundamento: `experience_register` crea la experiencia ya en `EXP_PENDING` (no hay estado
"registrada-pero-no-pendiente"); un `SUBMITTED` idéntico a `PENDING` sería inalcanzable estable y WP
lo marcaría como código muerto (como las guardas que `-wp-smoke-tests` ya señala,
`docs/explicacionv1.md` §9). La presentación queda igualmente registrada como evento de auditoría
`OP_SUBMIT_REQUEST`.

### 5.3 Respaldo institucional previo: criterio PROPUESTO por categoría (ajuste 6)

**Regla de negocio — DECIDIDA en la Revisión 3.** La Revisión 2 presentaba esta tabla como propuesta
a confirmar. **La Rev 3 la cierra como decisión aprobada por el usuario.** El criterio distingue
cuándo una solicitud necesita respaldo institucional previo (pasar por `REQ_ENDORSED` antes de que
VERIFIER pueda aprobar) y cuándo VERIFIER puede evaluarla directamente desde `PENDING`. La única
categoría con tratamiento **condicional** es `CAT_RESEARCH_PROJECT` (§5.3 bis).

| Categoría | ¿Requiere respaldo institucional previo? | Motivo (propuesta) |
|-----------|:---------------------------------------:|--------------------|
| `CAT_INSTITUTION_WORKED` | **Sí** | Vinculación laboral: solo la institución puede confirmarla. |
| `CAT_LINK_PERIOD` | **Sí** | Períodos de contratación: dato institucional. |
| `CAT_LINK_TYPE_DEDICATION` | **Sí** | Tipo de vínculo / dedicación: dato institucional. |
| `CAT_TEACHING_HOURS` | **Sí** | Horas docentes institucionales (asignadas/realizadas): las certifica la institución. |
| `CAT_COURSES_TAUGHT` | **Sí** | Cursos dictados en la institución. |
| `CAT_ACADEMIC_PERIOD` | **Sí** | Período académico institucional. |
| `CAT_ACADEMIC_ACTIVITY` | **Sí** | Actividad académica interna. |
| `CAT_THESIS_SUPERVISION` | **Sí** | Dirección de tesis institucional. |
| `CAT_COMMITTEES` | **Sí** | Participación en comités internos. |
| `CAT_OUTREACH` | **Sí** | Extensión institucional. |
| `CAT_TRAINING` | **No** | Formación externa: VERIFIER evalúa con certificados de terceros. |
| `CAT_CERTIFICATION` | **No** | Certificaciones de terceros. |
| `CAT_PUBLICATION` | **No** | Publicaciones científicas: evidencia externa verificable (editorial/DOI opaco). |
| `CAT_RESEARCH_PROJECT` | **Condicional** | Depende de si el proyecto es institucional o externo (§5.3 bis). |

Alineación con el requisito del usuario (A): las categorías marcadas **Sí** son exactamente
*institución donde trabajó, período de vinculación, tipo de vinculación y dedicación, horas docentes
asignadas y realizadas, cursos dictados, períodos académicos, actividades académicas institucionales,
dirección de tesis, comités, y extensión y proyección social*. Las marcadas **No** son *formación
académica y actualización, certificaciones, y publicaciones científicas*.

Regla DECIDIDA, expresable como predicado puro (sin estado, cierra en WP como `security_can`):

```c
/* 1 si la categoría exige respaldo institucional previo; 0 si VERIFIER puede evaluar directo.
 * Para CAT_RESEARCH_PROJECT el resultado depende del dato de la solicitud (§5.3 bis),
 * por lo que la condicionalidad NO se resuelve en este switch puro, sino en el cuerpo de
 * request_submit mirando institution_id. Aquí CAT_RESEARCH_PROJECT devuelve 0 (base);
 * la exigencia condicional se añade por separado (ver abajo). */
int request_requires_endorsement(request_category_t cat);   /* switch puro, sin bucles */
```

### 5.3 bis — Proyectos de investigación: respaldo CONDICIONAL (ajuste A, Rev 3)

Un `CAT_RESEARCH_PROJECT` puede corresponder a una institución o ser externo/independiente. La
distinción se modela con un dato que ya existe en `request_t`: **`institution_id`**.

| Caso | Dato en la solicitud | Comportamiento |
|------|----------------------|----------------|
| Proyecto **institucional** | `institution_id != 0` (institución asociada presente) | Requiere respaldo de **esa** institución: la solicitud sigue la rama "exige respaldo" (nace `PENDING`, necesita `ENDORSED` antes de `verify`), salvo que la propia institución la presente directamente (§5.3 ter). |
| Proyecto **externo/independiente** | `institution_id == 0` (ausente) | NO requiere respaldo previo: VERIFIER lo evalúa directo desde `PENDING`. |

No se añade ningún flag nuevo: `institution_id == 0` vs `!= 0` **es** el discriminante (coherente con
que el struct ya reserva `institution_id = 0` para "no aplica", §4.2). La regla efectiva de exigencia
de respaldo que usa `request_submit`/`request_verify` es:

```
exige_respaldo(cat, institution_id) =
      request_requires_endorsement(cat)                         // categorías "Sí" de la tabla
   || (cat == CAT_RESEARCH_PROJECT && institution_id != 0);     // proyecto institucional
```

Esta composición mantiene `request_requires_endorsement` como switch puro (cierra en WP) y confina la
única parte dependiente del dato a una comparación entera simple (`institution_id != 0`) en el cuerpo.

### 5.3 ter — INSTITUTION que presenta directamente deja constancia del respaldo (ajuste A, Rev 3)

**Regla nueva y DECIDIDA.** Cuando el presentador es una **INSTITUTION** (representando a
`institution_id` según §3.2) y la categoría **exige respaldo** (`exige_respaldo` = 1), su propia
presentación **ya constituye el respaldo**: no se exige una segunda operación `request_endorse`.

- **INSTITUTION presenta y exige respaldo** ⇒ `request_submit` crea la solicitud **directamente en
  `REQ_ENDORSED`**, fijando `endorser_id = institution_id` en el mismo acto. Salta el paso de
  endorsement.
- **PROFESSOR presenta y exige respaldo** ⇒ `request_submit` crea la solicitud en `REQ_PENDING` con
  `endorser_id = 0`; queda pendiente hasta que una INSTITUTION autorizada ejecute `request_endorse`
  (`PENDING → ENDORSED`).
- **Cualquiera presenta y NO exige respaldo** ⇒ nace `REQ_PENDING`; VERIFIER evalúa directo; `ENDORSED`
  no se usa.

El respaldo (propio de la INSTITUTION o vía `request_endorse`) **NO reemplaza** la verificación del
VERIFIER: `ENDORSED` sigue siendo paso previo obligatorio a `REQ_VERIFIED`, nunca sustituto.
Esta regla se eleva a **propiedad de seguridad P-15** (§8).

**Efecto en la máquina de estados (con la regla efectiva `exige_respaldo`, §5.3 bis/ter):**

- Si `exige_respaldo(category, institution_id)` es 1 y presenta el **PROFESSOR**: la solicitud nace
  `PENDING` y **VERIFIER no puede aprobarla** hasta que la institución la respalde (`request_endorse`:
  `PENDING → ENDORSED`, `OP_ENDORSE_REQUEST`, solo INSTITUTION que representa a `institution_id`).
  `request_verify` sobre una solicitud que exige respaldo pero está aún en `PENDING` devuelve
  `ERR_BAD_TRANSITION`.
- Si `exige_respaldo` es 1 y presenta la **INSTITUTION** (que representa): nace directamente
  `REQ_ENDORSED` con `endorser_id = institution_id` (§5.3 ter), sin segunda operación.
- Si `exige_respaldo` es 0: VERIFIER aprueba/rechaza directamente desde `PENDING` (no se usa
  `ENDORSED`), con independencia de quién presente.

> **Marcado explícito (Rev 3):** esta tabla ya NO es una propuesta: es la decisión aprobada del
> usuario. Si en el futuro se reclasifica una categoría, es editar un `case` del `switch` puro, sin
> impacto en el resto del diseño ni en la verificación.

### 5.4 Tabla de transiciones

| Transición | Función | Origen → destino | Rol | Regla extra |
|------------|---------|------------------|-----|-------------|
| Presentar (PROFESSOR) | `request_submit` | (nueva) → `REQ_PENDING` | PROFESSOR (propia) | id único; fechas válidas; metadatos por categoría; `caller_id==owner_id` |
| Presentar (INSTITUTION, exige respaldo) | `request_submit` | (nueva) → `REQ_ENDORSED` | INSTITUTION (representa) | §5.3 ter; fija `endorser_id=institution_id`; autorización §3.2 |
| Presentar (no exige respaldo) | `request_submit` | (nueva) → `REQ_PENDING` | PROFESSOR / INSTITUTION | nace sin respaldo; VERIFIER directo |
| Respaldar | `request_endorse` | `REQ_PENDING` → `REQ_ENDORSED` | INSTITUTION (representa) | solo tras presentación del PROFESSOR que exige respaldo (§5.3); fija `endorser_id` |
| Aprobar | `request_verify` | `REQ_ENDORSED`* o `REQ_PENDING`** → `REQ_VERIFIED` | VERIFIER | `submitter_id != caller_id`; *si exige respaldo; **si no |
| Rechazar | `request_reject` | `REQ_PENDING`/`REQ_ENDORSED` → `REQ_REJECTED` | VERIFIER | `submitter_id != caller_id`; terminal |
| Incorporar (simple) | `request_record` | `REQ_VERIFIED` → `REQ_RECORDED` | ADMIN | `prev_id==0`; no re-incorporar; contenido inmutable |
| Incorporar con sustitución | `request_record` | nueva `REQ_VERIFIED` → `REQ_RECORDED` **y** anterior `REQ_RECORDED` → `REQ_SUPERSEDED` | ADMIN | `prev_id!=0`; validaciones §5.5; **atómica** |

`REQ_REJECTED`, `REQ_RECORDED` y `REQ_SUPERSEDED` son **terminales** (igual que `CRED_REVOKED`): toda
transición de salida devuelve `ERR_BAD_TRANSITION`, por el chequeo de estado origen exacto (patrón
`cred_transition`: `if (st->reqs[idx].state != from) return ERR_BAD_TRANSITION;`). Matiz de la Rev 3:
`REQ_RECORDED` deja de ser puramente terminal en un único sentido — puede pasar a `REQ_SUPERSEDED`,
pero **nunca por una operación dirigida a ese registro**, sino como efecto lateral de incorporar la
corrección que lo apunta por `prev_id` (§5.5).

### 5.5 — Corrección del historial: `request_record` con dos variantes de efecto (requisito nuevo, Rev 3; `prev_id` resuelto en Rev 4)

**Decisión Rev 4 — función específica de corrección, NO `prev_id` en `request_submit`.** Hay dos
formas de permitir que una solicitud nazca con `prev_id != 0`:

| Opción | Descripción | Trade-off |
|--------|-------------|-----------|
| (a) Añadir `prev_id` a la firma de `request_submit` | Un único punto de entrada; `prev_id==0` = original, `prev_id!=0` = corrección. | La firma de `request_submit` ya tiene 15 parámetros; `prev_id` solo aplica a correcciones; obliga a validar «si `prev_id==0` no mires el historial / si `!=0` sí» dentro del mismo cuerpo, mezclando dos contratos (original vs corrección) en un `ensures` con disyunciones por caso. |
| (b) **Función específica `request_submit_correction`** | `request_submit` crea siempre originales (`prev_id` se fija a 0 internamente); una función hermana crea correcciones y recibe `prev_id`. | **Elegida.** Separa los dos contratos: `request_submit` conserva su `ensures` de nacimiento sin la rama de corrección; `request_submit_correction` concentra la validación de `prev_id` al presentar. Es el mismo criterio por el que `credentials.c` expone wrappers delgados (`credential_activate`, `credential_block`, …) sobre `cred_transition` en vez de un único punto con un parámetro de modo. |

**Se elige (b).** `request_submit` no cambia su firma ni su semántica de nacimiento (§5.3 ter) y fija
`prev_id = 0` internamente; `request_submit_correction` es su hermana para correcciones. Ambas comparten
el helper interno de inserción; la diferencia es que la corrección valida `prev_id` **al presentar**.

```c
/* Crea una corrección: nueva solicitud con prev_id apuntando al registro a corregir.
 * Misma forma que request_submit (§7.1) + prev_id; valida prev_id AL PRESENTAR (§5.5, validación-S). */
int request_submit_correction(educhain_state_t *st, requests_state_t *rs,
                              role_t caller_role, int caller_id, int req_id, int owner_id,
                              int institution_id, int category, int start_date, int end_date,
                              int evidence_ref, int prev_id,
                              int num_a, int num_b, int num_c, int num_d);
```

**`prev_id` se valida DOS VECES (Rev 4): al PRESENTAR y al INCORPORAR.** Son dos momentos distintos
porque el estado del registro anterior **puede cambiar entre medias** (otra corrección podría
incorporarse mientras ésta se tramita). La validación al presentar rechaza correcciones mal formadas
desde el inicio; la revalidación al incorporar es la que garantiza la coherencia final.

- **Validación-S (al PRESENTAR, en `request_submit_correction`):** `prev_id` existe
  (`request_find(rs, prev_id) >= 0`, si no `ERR_NOT_FOUND`); el anterior está vigente
  (`reqs[prev].state == REQ_RECORDED`, si no `ERR_BAD_TRANSITION`); mismo owner; coherencia
  categoría/institución (§5.5 validación 4). Si pasa, la corrección nace y recorre el flujo normal
  (respaldo si aplica, verificación) con `prev_id` almacenado en su slot.
- **Validación-R (al INCORPORAR, en `request_record`, Variante B):** se **revalida** `prev_id`
  (validaciones 1–6 de abajo). Es imprescindible porque entre la presentación y la incorporación el
  registro anterior pudo pasar a `REQ_SUPERSEDED` (lo sustituyó otra corrección que se incorporó
  antes). La revalidación detecta ese cambio y rechaza sin mutar nada (§5.5 «anterior ya no vigente»).

Una corrección NO edita el registro vigente: se presenta como **nueva** solicitud con `prev_id`
apuntando al registro `REQ_RECORDED` a corregir, y recorre **todo** el flujo (respaldo si aplica,
verificación, aprobación) como cualquier solicitud. El registro original permanece **inalterado**
mientras la corrección se tramita. La diferencia aparece **solo** en `request_record`:

- **Variante A — incorporación simple** (`prev_id == 0`): `nueva: REQ_VERIFIED → REQ_RECORDED`.
  Idéntica al diseño previo.
- **Variante B — incorporación con sustitución** (`prev_id != 0`): en **una sola operación**,
  `nueva: REQ_VERIFIED → REQ_RECORDED` **y** `anterior(prev_id): REQ_RECORDED → REQ_SUPERSEDED`.

`request_record` detecta la variante mirando el `prev_id` de la solicitud `req_id` que incorpora.

**Validaciones de la Variante B (todas ANTES de mutar, patrón "validar todo antes de mutar nada"):**

1. **`prev_id` existe:** `request_find(rs, prev_id) >= 0`, si no `ERR_NOT_FOUND`.
2. **El anterior está vigente:** `reqs[prev].state == REQ_RECORDED`, si no `ERR_BAD_TRANSITION`.
   Esto rechaza por construcción **re-sustituir un ya sustituido** (un `REQ_SUPERSEDED` no está en
   `REQ_RECORDED`), cubriendo "no SUPERSEDED → otra vez".
3. **Mismo propietario:** `reqs[nueva].owner_id == reqs[prev].owner_id`, si no `ERR_PERM`
   (una corrección pertenece al mismo profesor que el registro corregido).
4. **Coherencia categoría/institución (T-1 CERRADA, Rev 5):** la categoría de la corrección
   **siempre** debe coincidir con la del original (`reqs[nueva].category == reqs[prev].category`), y la
   institución queda fijada por la categoría, **sin** grado de libertad:
   - Si la categoría **asocia** institución, ambos registros deben tener **exactamente** el mismo
     `institution_id`: `reqs[nueva].institution_id == reqs[prev].institution_id`.
   - Si la categoría **no** asocia institución, **ambos** deben tener `institution_id == 0`:
     `reqs[nueva].institution_id == 0 && reqs[prev].institution_id == 0`.

   Cualquier violación ⇒ `ERR_BAD_TRANSITION`. Así una corrección **nunca** cambia la institución de
   un registro (cambiar de categoría o de institución produciría un registro distinto, no una
   corrección). Esto cierra la tensión T-1 de la Rev 4: ya **no** queda el grado de libertad «categoría
   sin institución» sin resolver; la regla es única y definitiva.
5. **La nueva está verificada:** `reqs[nueva].state == REQ_VERIFIED` (igual que la Variante A).
6. **Hueco de log:** `log_count < MAX_EVENTS`, si no `ERR_FULL`.

**Aciclicidad garantizada por las TRANSICIONES DE ESTADO, no por el orden numérico de ids (Rev 4).**
El grafo de correcciones es un bosque dirigido donde cada arista va de la **corrección** (nueva) al
**corregido** (`prev_id`). La propiedad clave —que nunca se forma un ciclo entre correcciones que
**llegan al historial oficial**— se obtiene de la máquina de estados, **no** de que los ids sean
crecientes:

- **Un `REQ_RECORDED` solo puede ser sustituido una vez.** Al incorporar una corrección (Variante B),
  el anterior pasa `REQ_RECORDED → REQ_SUPERSEDED`. La validación-R 2 exige que `prev_id` esté en
  `REQ_RECORDED`; como `REQ_SUPERSEDED` es terminal y no retorna a `REQ_RECORDED`, **ningún registro
  ya sustituido puede volver a ser `prev_id`**. Cada registro vigente tiene a lo sumo un sucesor.
- **Un registro solo es `prev_id` válido si ya está incorporado.** Una corrección solo entra a
  `REQ_RECORDED` tras ser incorporada; mientras se tramita no es `REQ_RECORDED`, de modo que no puede
  ser `prev_id` de nada aún. Una corrección no puede, por tanto, apuntar a sí misma ni a una
  corrección todavía en trámite. Esto **no** depende de comparar `req_id` con `prev_id`: aunque una
  corrección tuviera un id numérico menor que su anterior, la regla de estado sigue impidiendo el
  ciclo. La aciclicidad es una propiedad de las **transiciones alcanzables**, no del espacio de ids.
- Resultado: las cadenas que llegan al historial oficial son lineales y finitas
  (`A(SUPERSEDED) ← B(SUPERSEDED) ← C(RECORDED)`); es imposible construir `A ← B ← A`.
- **Decisión de diseño explícita (T-2 CERRADA, Rev 5):** NO se recorre explícitamente la cadena
  `prev_id` en tiempo de ejecución (sería un bucle con su propio coste de WP). **Decisión definitiva
  del usuario: no se añade validación de cadena.** La garantía estructural se apoya exclusivamente en
  las transiciones de estado: «`prev_id` debe apuntar a un registro existente y vigente
  (`REQ_RECORDED`)» + «al incorporar una corrección el anterior pasa a `REQ_SUPERSEDED`» + «un
  `REQ_SUPERSEDED` no puede volver a usarse como `prev_id`» + «la revalidación al incorporar
  (validación-R) impide que dos correcciones sustituyan el mismo registro». La aciclicidad se refiere
  a las correcciones que **llegan al historial oficial** y depende de esas transiciones de estado,
  **no** del orden numérico de los ids ni de recorrer la cadena. Esto cierra T-2: ya **no** es un punto
  a confirmar (§14).

**Carrera lógica: dos correcciones verificadas sustituyen el MISMO registro (Rev 4).** Dos
correcciones `B` y `B'`, ambas con `prev_id == A` (donde `A` está `REQ_RECORDED`), pueden quedar las
dos en `REQ_VERIFIED` a la vez (la verificación de una no mira a la otra). No hay carrera de hilos
—el modelo es secuencial—, sino una **carrera lógica**: ambas pretenden sustituir `A`. La regla es
**la primera incorporación que llega gana**:

1. ADMIN incorpora `B`: la validación-R pasa (`A` está `REQ_RECORDED`); en la operación atómica,
   `B → REQ_RECORDED` y `A → REQ_SUPERSEDED`.
2. ADMIN incorpora `B'` después: la **revalidación-R de `prev_id`** encuentra que `A` **ya NO está en
   `REQ_RECORDED`** (ahora es `REQ_SUPERSEDED`). La validación 2 falla y `request_record` devuelve
   `ERR_BAD_TRANSITION` **sin modificar nada** (ni `B'`, ni `A`, ni `B`). `B'` queda en `REQ_VERIFIED`.

Esta es la razón directa de revalidar `prev_id` al incorporar (validación-R): sin ella, `B'` habría
sustituido a `A` por segunda vez o sobrescrito el efecto de `B`. Por construcción, un `REQ_RECORDED`
se sustituye exactamente una vez.

**Corrección YA VERIFICADA cuyo registro anterior DEJÓ DE ESTAR VIGENTE (Rev 4).** Es el caso de `B'`
arriba, y también el de cualquier corrección que fue verificada apuntando a `A` y, antes de que ADMIN
la incorpore, `A` pasó a `REQ_SUPERSEDED` por otra corrección. Resolución:

- **Qué ocurre al incorporar:** la revalidación-R de `prev_id` (validación 2) falla con
  `ERR_BAD_TRANSITION` (`A` ya no está `REQ_RECORDED`). `request_record` no muta nada.
- **Estado en que queda la corrección:** permanece en `REQ_VERIFIED` (su propio estado no cambia; no
  pasa a `REJECTED` ni a `SUPERSEDED`). No se incorpora: no aparece en el historial vigente ni cuenta
  para elegibilidad (§8-ter). Es un callejón sin salida respecto de `A`, pero su contenido y su traza
  de auditoría se conservan.
- **Vía de resolución:** presentar una **nueva** corrección (`request_submit_correction`) cuyo
  `prev_id` apunte al **registro vigente actual** (el `REQ_RECORDED` que sustituyó a `A`, es decir
  `B`), no al ya sustituido `A`. Esa nueva corrección recorre el flujo y, si se incorpora,
  sustituye a `B`. La corrección obsoleta puede dejarse inerte en `REQ_VERIFIED`; el diseño **no**
  añade una operación para «caducarla» (no hay regla que lo exija), de modo que no se introduce
  comportamiento nuevo sobre ese registro.

**Atomicidad crítica (BR-10 reforzada).** Incorporar la nueva y sustituir la anterior son **una sola
operación lógica**: todas las validaciones (1–6) y la comprobación de permiso ADMIN se hacen **antes**
de tocar ningún `state`. Si algo falla —validación, permiso, capacidad, o el hueco de log—, **ninguno**
de los dos registros se modifica (`reqs[nueva].state` y `reqs[prev].state` quedan como estaban). Solo
cuando todo pasa se escriben **ambos** `state` y luego se audita. Si la auditoría misma fallara, la
solución honesta es la misma que ya usan los módulos actuales: el hueco de log se comprueba **antes**
de mutar (`if (log_count >= MAX_EVENTS) return ERR_FULL;`), igual que `cred_transition`, de modo que
cuando se llega a escribir los dos `state` el `audit_record_event` tiene garantizado su hueco. El
análisis WP de esta atomicidad está en §8-bis (P-16).

### 5.6 — Resumen de la decisión `prev_id` (Rev 4)

Ancla de referencia para el resto del documento. La decisión de corrección de la Rev 4, detallada en
§5.5, es: (1) **función específica `request_submit_correction`** en lugar de añadir `prev_id` a
`request_submit` (separa contratos original/corrección, patrón wrappers de `credentials.c`);
(2) **doble validación de `prev_id`**: al presentar (validación-S) y al incorporar (validación-R en
`request_record`), porque el estado del anterior puede cambiar entre medias; (3) **carrera de dos
correcciones** sobre el mismo registro: gana la primera incorporación, la segunda falla con
`ERR_BAD_TRANSITION` al revalidar; (4) **aciclicidad por transiciones de estado** (un `RECORDED` se
sustituye una vez, un `SUPERSEDED` no vuelve a ser `prev_id`), no por orden de ids; (5) **corrección
con anterior ya no vigente**: queda inerte en `REQ_VERIFIED`, se resuelve presentando una nueva
corrección contra el registro vigente actual.

---

## 6. Flujo completo de presentación, verificación e incorporación

> **Narrativa, no reglas nuevas (Rev 4).** Esta sección recorre el proceso paso a paso; las reglas de
> nacimiento son las de §5.0 y las transiciones son las de §5.4. No introduce ninguna regla que no
> esté ya en §5. La creación de correcciones usa `request_submit_correction` (§5.5/§5.6).

1. **PROFESSOR o INSTITUTION presenta** → `request_submit(st, rs, role, caller_id, req_id, owner_id,
   institution_id, category, start, end, evidence_ref, num_a..num_d)`. INSTITUTION debe pasar
   `requests_is_representative(rs, caller_id, institution_id)` (§3.2).
2. **Registro con id único** → `request_find(rs, req_id) >= 0 ==> ERR_DUPLICATE` (patrón
   `experience_find`), inserción atómica `reqs[n]` con `n = reqs_count`.
3. **Nacimiento según §5.0 (regla canónica única)** → el estado inicial lo fijan las tres reglas de
   §5.0: PROFESSOR con respaldo obligatorio nace `REQ_PENDING` (`endorser_id=0`); INSTITUTION que
   presenta una categoría que exige respaldo nace `REQ_ENDORSED` (`endorser_id=institution_id`, sin
   `request_endorse`); sin respaldo obligatorio nace `REQ_PENDING`. En todos los casos `verifier_id=0`.
4. **Respaldo institucional (solo si PROFESSOR presentó y la categoría lo exige)** → una INSTITUTION
   que representa a `institution_id` ejecuta `request_endorse` (`PENDING → ENDORSED`, fija
   `endorser_id`). Este paso **se omite** cuando la INSTITUTION fue la presentadora (ya nació
   `ENDORSED`, §5.0 / §5.3 ter) o cuando la categoría no exige respaldo. El respaldo **no** sustituye
   al VERIFIER (§5.0, principio 1).
5. **VERIFIER revisa** (consulta) → `request_status` / `request_find` (solo lectura).
6. **Aprueba o rechaza** → `request_verify` (→ VERIFIED) o `request_reject` (→ REJECTED), ambos con
   `submitter_id != caller_id`. `request_verify` exige `ENDORSED` si la categoría requería respaldo.
7. **Si rechaza, no pasa a oficial** → `REQ_REJECTED` terminal; `request_record` da `ERR_BAD_TRANSITION`.
8. **ADMIN incorpora** → `request_record(st, rs, ROLE_ADMIN, caller_id, req_id)`, exige
   `state == REQ_VERIFIED`. Si la solicitud lleva `prev_id != 0`, se activa la **Variante B**
   (incorporación con sustitución, §5.5), con sus validaciones (existencia de `prev_id`, anterior
   en `REQ_RECORDED`, mismo owner, coherencia categoría/institución, no re-sustituir).
9. **Historial oficial** → `state = REQ_RECORDED` para la nueva; el contenido (owner, categoría,
   fechas, `evidence_ref`, metadatos) **no se toca** (el `assigns` solo nombra `reqs[k].state`). En la
   **Variante B**, en la **misma** operación, `reqs[prev].state = REQ_SUPERSEDED` (y **solo** su
   `state`: todo su contenido histórico se conserva para auditoría, §4.1, §8-bis P-17).
10. **Auditoría** → cada paso llama `audit_record_event(st, caller_id, OP_*, req_id, EDU_OK)` con
    `actor_id = caller_id` (ejecutor real, nunca `owner_id`).

**Correcciones (Rev 3):** el profesor no edita; presenta una **nueva** solicitud con
`prev_id = <id a corregir>`. La original mantiene su estado `REQ_RECORDED` **mientras se tramita** la
corrección; la nueva recorre el flujo desde `REQ_PENDING`/`REQ_ENDORSED`. Solo cuando ADMIN incorpora
la corrección (paso 8, Variante B) el registro anterior pasa a `REQ_SUPERSEDED` — en la misma
operación atómica, nunca antes. A partir de ahí, el anterior sale del historial vigente (ya no cuenta
en `request_recorded_count`, §8-ter) pero su contenido queda disponible para auditoría.

---

## 7. Funciones nuevas, modificadas y eliminadas

> **Firmas coherentes con §5.0/§5.4 (Rev 4).** `request_submit` crea siempre originales y fija
> `prev_id=0` internamente; `request_submit_correction` (§5.5/§5.6) crea correcciones y recibe
> `prev_id`. El estado de nacimiento que devuelven ambas es exactamente el de §5.0. Las consultas se
> dividen en primitivas internas y capa pública por rol/ámbito (§7.3, §3.3 bis).

### 7.1 Nuevas (`src/requests.c`, `include/requests.h`)

| Función | Firma propuesta | Propósito |
|---------|-----------------|-----------|
| `request_submit` | `int request_submit(educhain_state_t*, requests_state_t*, role_t, int caller_id, int req_id, int owner_id, int institution_id, int category, int start_date, int end_date, int evidence_ref, int num_a, int num_b, int num_c, int num_d)` | Crea solicitud **original** (`prev_id` fijado a 0 internamente). Estado de nacimiento según §5.0 (PROFESSOR→`PENDING`; INSTITUTION que exige respaldo→`ENDORSED`; sin respaldo→`PENDING`). |
| `request_submit_correction` | `int request_submit_correction(educhain_state_t*, requests_state_t*, role_t, int caller_id, int req_id, int owner_id, int institution_id, int category, int start_date, int end_date, int evidence_ref, int prev_id, int num_a, int num_b, int num_c, int num_d)` | Crea una **corrección** (`prev_id!=0`). Valida `prev_id` **al presentar** (validación-S, §5.5), incluida la **coherencia categoría/institución T-1 (Rev 5)**: misma categoría que el original y mismo `institution_id` (o ambos `0` si la categoría no asocia institución); nunca cambia la institución. Nace con el mismo estado §5.0 que una original; su efecto diferencial aparece en `request_record` Variante B. **Decisión Rev 4** (vs añadir `prev_id` a `request_submit`): §5.5. |
| `request_endorse` | `int request_endorse(educhain_state_t*, requests_state_t*, role_t, int caller_id, int req_id)` | `PENDING → ENDORSED` (INSTITUTION que representa; solo categorías que lo exigen). |
| `request_verify` | `int request_verify(educhain_state_t*, requests_state_t*, role_t, int caller_id, int req_id)` | `ENDORSED`/`PENDING → VERIFIED` (VERIFIER, no auto). |
| `request_reject` | `int request_reject(educhain_state_t*, requests_state_t*, role_t, int caller_id, int req_id)` | `PENDING`/`ENDORSED → REJECTED` (VERIFIER, no auto). |
| `request_record` | `int request_record(educhain_state_t*, requests_state_t*, role_t, int caller_id, int req_id)` | `VERIFIED → RECORDED` (solo ADMIN). **Variante B** (si `prev_id!=0`): además `anterior: RECORDED → SUPERSEDED` en la misma operación atómica (§5.5). |
| `request_find` | `int request_find(const requests_state_t*, int req_id)` | Búsqueda (patrón `experience_find`, `assigns \nothing`). |
| `request_status` | `int request_status(const requests_state_t*, int req_id)` | Estado actual o -1 (`assigns \nothing`). |
| `request_is_recorded` | `int request_is_recorded(const requests_state_t*, int req_id)` | 1 si el req_id está en `REQ_RECORDED` vigente (excluye `REQ_SUPERSEDED`; base de §8-ter; `assigns \nothing`). |
| `request_recorded_count` | `int request_recorded_count(const requests_state_t*, int owner_id)` | **Primitiva interna** (sin rol). Nº de registros `REQ_RECORDED` del titular, **excluyendo `REQ_SUPERSEDED`** (trayectoria oficial vigente; `assigns \nothing`). |
| `request_status_for` / `request_is_recorded_for` / `request_recorded_count_for` | `int request_*_for(const requests_state_t*, role_t caller_role, int caller_id, int req_id_or_owner)` | **Capa pública** de consulta con control de rol + identidad + ámbito (§3.3 bis, P-SCOPE); delega en las primitivas solo para el ámbito permitido; `ERR_PERM` fuera de ámbito; `assigns \nothing`. |
| `requests_is_representative` | `int requests_is_representative(const requests_state_t*, int user_id, int institution_id)` | Vínculo ACTIVO de representación (§3.2 bis; `assigns \nothing`). |
| `authz_grant` | `int authz_grant(educhain_state_t*, requests_state_t*, role_t, int caller_id, int user_id, int institution_id)` | Autoriza representación (solo ADMIN; §3.2 bis). |
| `authz_revoke` | `int authz_revoke(educhain_state_t*, requests_state_t*, role_t, int caller_id, int user_id, int institution_id)` | Revoca: `active=0` sin borrar (solo ADMIN; §3.2 bis). |
| `request_requires_endorsement` | `int request_requires_endorsement(request_category_t)` | Regla de negocio §5.3 (switch puro). La condicionalidad de `CAT_RESEARCH_PROJECT` se compone con `institution_id` en el cuerpo (§5.3 bis). |
| `request_transition` (static) | helper genérico `from→to` con op_code | Reutiliza el diseño de `cred_transition`. |

### 7.2 Modificadas (aditivo)

| Archivo | Cambio | Justificación |
|---------|--------|---------------|
| `include/common.h` | +rol `ROLE_INSTITUTION`/alias; +OPs `OP_SUBMIT_REQUEST`, `OP_ENDORSE_REQUEST`, `OP_VERIFY_REQUEST`, `OP_REJECT_REQUEST`, `OP_RECORD_REQUEST`, `OP_GRANT_REPRESENTATION`, `OP_REVOKE_REPRESENTATION` (tras `OP_REVOKE_CRED=8`); +enums `request_state_t` (incl. `REQ_SUPERSEDED=5`) y `request_category_t`; +`request_t` (con `prev_id`), +`inst_authz_t`, +`requests_state_t` (`reqs[MAX_REQUESTS]`, `auth[MAX_AUTHZ]` + contadores), `MAX_REQUESTS=128`, `MAX_AUTHZ=64`; +`ERR_*` (todos ya existen). | Soporte de datos. **Append-only.** El estado nuevo NO entra en `educhain_state_t`. |
| `src/security.c` + `include/security.h` | +casos para las OPs nuevas + cláusulas bicondicionales | Mantener `security_can` como decisor único. Ramas existentes sin cambio. |
| `verification/specifications.h` | +cláusulas `role_can` para las OPs nuevas | Permite derivar P-1 en `requests.c` (igual que hoy). |

### 7.3 Capa de consulta: primitivas internas vs funciones públicas (Rev 4)

Las consultas de §7.1 (`request_find`, `request_status`, `request_is_recorded`,
`request_recorded_count`, `requests_is_representative`, `request_requires_endorsement`) son
**primitivas internas de solo lectura, sin chequeo de rol** (forma `experience_find`/
`credential_status`, `assigns \nothing`): las usan `requests.c` y, para elegibilidad, `credentials.c`
(§8-ter). **No** se exponen a un usuario final. Sobre ellas se diseña la **capa pública** con
comprobación de **rol + identidad + ámbito** (`request_status_for`, `request_is_recorded_for`,
`request_recorded_count_for`), cuyo contrato y matriz están en **§3.3 bis** (principio P-SCOPE: nadie
consulta fuera de su ámbito). Estado esperado: las primitivas cierran como sus gemelas; la capa
pública espera WP en su mitad negativa (fuera de ámbito ⇒ `ERR_PERM`) + respaldo dinámico.

### 7.4 Eliminadas

**Ninguna.** `experience_validate` y `credential_*` se **conservan** (los usan tests A1–A3 y la CLI);
permisos de HR/COORD se **restringen, no eliminan** (se les niegan las OPs nuevas en `security_can`).

**Cómo se impide saltarse el flujo (respuesta directa):** todo paso de escritura pasa por
`security_can`, que **no concede ninguna OP nueva a HR, COORD, PRO (para verify/record/endorse) ni a
quien presentó**. Además, `experience_validate`/`credential_*` escriben en `exps[]`/`creds[]` dentro
de `educhain_state_t`; `reqs[]` vive en `requests_state_t`, un **struct distinto que esas funciones ni
siquiera reciben por parámetro**. No existe firma que permita a una función antigua tocar `reqs[]`.
Ver el invariante transversal (§8-bis).

---

## 8. Las propiedades de seguridad (≥20) y sus contratos ACSL

Para cada propiedad: contrato ACSL propuesto (forma) y **estado esperado** (objetivo), deducido del
comportamiento real de los artefactos base (`verification/wp_results_baseline.txt`,
`docs/explicacionv1.md` §7, §9). **Lenguaje:** todo lo de esta sección es un *objetivo de prueba*, no
un resultado obtenido — se usa "se espera demostrar en WP", "necesita prueba dinámica" o "podría
quedar parcialmente automatizada". NINGUNA cifra nueva está demostrada todavía; la implementación y la
re-ejecución de WP/Eva son Etapa 2+. Cada propiedad lleva una de tres etiquetas:

- **se espera WP** — objetivo: que WP lo cierre (implicación local, patrón ya probado en el repo).
- **dinámica** — se comprobará con `tests/test_requests.c` (patrón `test_attacks.c`); no se espera
  cierre automático en WP.
- **WP parcial + dinámica** — se espera que WP cierre una parte (p. ej. la mitad negativa o el
  `assigns`) y la otra quede como objetivo dinámico.

Las dos limitaciones conocidas del probador que gobiernan estas predicciones: **(F)** framing del
struct a través de `audit_record_event`, y **(G)** ausencia de invariante global `state_inv`
(unicidad/existenciales globales).

| # | Propiedad | Contrato ACSL propuesto (forma) | Estado esperado | Por qué |
|---|-----------|----------------------------------|-----------------|---------|
| 1 | PROFESSOR solo presenta lo propio | `ensures (role==ROLE_PRO && caller_id!=owner_id) ==> \result==ERR_PERM;` | **se espera WP** | Patrón `experience_register` P-1: implicación local sin framing global. |
| 2 | INSTITUTION solo en su ámbito | `ensures (role==ROLE_INSTITUTION && !requests_is_representative(rs,caller_id,institution_id)) ==> \result==ERR_PERM;` | **se espera WP** | Implicación local; `requests_is_representative` es consulta `assigns \nothing` que cierra. |
| 3 | Ningún solicitante aprueba lo suyo | `ensures (role_can(role,OP_VERIFY_REQUEST) && ids en rango && \exists k: reqs[k].id==req_id && \old(reqs[k].submitter_id)==caller_id) ==> \result==ERR_SELF_VALIDATE;` | **se espera WP (mitad negativa) + dinámica** | Forma exacta de P-2 de `experience_validate` (la mitad negativa cierra). El dual positivo queda por (G). |
| 4 | Solo VERIFIER aprueba/rechaza | `ensures (role!=ROLE_VERIFIER) ==> \result==ERR_PERM;` + bicondicional en `security_can` | **se espera WP** | `security.c` cierra 114/114; añadir cláusulas al `switch` puro hereda ese estado. |
| 5 | Solo ADMIN incorpora | `ensures (role!=ROLE_ADMIN) ==> \result==ERR_PERM;` + bicondicional | **se espera WP** | Igual que #4: decisor puro. |
| 6 | ADMIN no incorpora salvo VERIFIED | `ensures \result==EDU_OK ==> (\exists k: reqs[k].id==req_id && \old(reqs[k].state)==REQ_VERIFIED);` | **se espera WP parcial + dinámica** | Estado origen exacto cierra local (`cred_transition`); el existencial global queda como P-7 de credenciales. |
| 7 | Contenido incorporado = aprobado (inmutabilidad) | `assigns { reqs[k].state | k; reqs[k].id==req_id }, log[...];` (contenido y `evidence_ref` NO en el `assigns`) | **se espera WP (vía `assigns`)** | Igual que P-5 de `experience_validate`: la inmutabilidad la captura el `assigns` acotado. |
| 8 | Sin solicitudes duplicadas | `ensures \result==EDU_OK ==> reqs_count==\old(reqs_count)+1;` + `request_find` previo | **dinámica + WP parcial** | La unicidad global de id es lo que (G) bloquea; mismo destino que `experience_register`. |
| 9 | Una rechazada no se incorpora | `ensures (\exists k: reqs[k].id==req_id && \old(reqs[k].state)==REQ_REJECTED) ==> \result==ERR_BAD_TRANSITION;` | **se espera WP (mitad negativa) + dinámica** | Clona la mitad negativa de P-8 (`credential_revoke` REVOKED terminal). |
| 10 | Una incorporada no se reincorpora | `ensures (\exists k: reqs[k].id==req_id && \old(reqs[k].state)==REQ_RECORDED) ==> \result==ERR_BAD_TRANSITION;` | **se espera WP (mitad negativa) + dinámica** | Idéntico a #9 (RECORDED terminal). |
| 11 | Operación fallida no corrompe estado | `ensures \result!=EDU_OK ==> reqs_count==\old(reqs_count);` (atomicidad BR-10) | **se espera WP parcial (contadores) + dinámica** | Contador cierra en casos simples; el post-estado completo queda Failure/Timeout por (F). |
| 12 | Auditoría conserva el actor real | `audit_record_event(st, caller_id, OP_*, req_id, EDU_OK)` + `ensures` de append/`seq` en `audit.h` | **se espera WP (a nivel de operación) + dinámica** | `audit.c` demuestra append-only y `seq` creciente por operación; `actor_id==caller_id` es decisión de código, respaldada dinámicamente. |
| **13** | **Autorización usuario→institución (ajuste 2)** | `ensures (role==ROLE_INSTITUTION && \result==EDU_OK) ==> (\exists k: 0<=k<rs->auth_count && rs->auth[k].user_id==caller_id && rs->auth[k].institution_id==institution_id && rs->auth[k].active==1);` + mitad negativa `ensures (role==ROLE_INSTITUTION && !(\exists k: ...activo...)) ==> \result==ERR_PERM;` | **se espera WP (mitad negativa) + dinámica** | Análoga a la no-autovalidación (#3): la mitad negativa es implicación local sobre una consulta `assigns \nothing`; el existencial positivo global queda por (G), con respaldo dinámico. |
| **14** | **Respaldo institucional condicional (ajuste 6)** | `ensures (exige_respaldo(cat,institution_id) && \old(reqs[k].state)==REQ_PENDING) ==> \result==ERR_BAD_TRANSITION;` en `request_verify` | **se espera WP (mitad negativa) + dinámica** | `request_requires_endorsement` es switch puro (cierra como `security_can`); el chequeo de estado origen exacto es el patrón `cred_transition`. |
| **15** | **INSTITUTION que presenta deja constancia del respaldo sin segunda operación (ajuste A, Rev 3)** | en `request_submit`: `ensures (role==ROLE_INSTITUTION && exige_respaldo(cat,institution_id) && \result==EDU_OK) ==> (\exists k: reqs[k].id==req_id && reqs[k].state==REQ_ENDORSED && reqs[k].endorser_id==institution_id);` y para PROFESSOR el dual con `REQ_PENDING && endorser_id==0` | **se espera WP (local) + dinámica** | El destino (`PENDING`/`ENDORSED`) lo decide una comparación de rol + `exige_respaldo` en el mismo cuerpo que inserta; es relación local sobre el slot recién escrito, sin existencial global, análoga a la inicialización de `experience_register`. |
| **16** | **Doble transición atómica de la corrección (requisito nuevo, Rev 3)** | en `request_record` Variante B: `ensures \result==EDU_OK ==> (\exists j,p: reqs[j].id==req_id && reqs[j].state==REQ_RECORDED && reqs[p].id==\old(prev_id) && \old(reqs[p].state)==REQ_RECORDED && reqs[p].state==REQ_SUPERSEDED);` + `ensures \result!=EDU_OK ==> (\forall k: reqs[k].state==\old(reqs[k].state));` | **se espera WP parcial (atomicidad de no-mutación en error) + dinámica (doble existencial)** | La mitad "en error nada cambia" es acotable por `assigns` condicionado (patrón `cred_transition` behaviors). El doble existencial positivo sobre dos slots cae en la limitación (G), igual que P-6; **podría quedar parcialmente automatizada**. Análisis honesto en §8-bis. |
| **17** | **La sustitución solo cambia `state` del anterior (inmutabilidad-salvo-state, requisito nuevo, Rev 3)** | `assigns { reqs[k].state | k; reqs[k].id==req_id || reqs[k].id==\old(prev_id) }, log[...];` + `ensures \old(prev_id)!=0 && \result==EDU_OK ==> (reqs[p].owner_id==\old(reqs[p].owner_id) && reqs[p].category==\old(reqs[p].category) && reqs[p].evidence_ref==\old(reqs[p].evidence_ref) && ... todos los campos salvo state)` | **se espera WP (vía `assigns` acotado) + ensures de inmutabilidad; honestamente: el `assigns` sobre DOS slots es más exigente que el de un slot de `cred_transition`** | El `assigns` set-comprehension que nombra `reqs[k].state` para los dos ids captura la inmutabilidad del resto por framing; es la generalización a dos slots del `assigns` de `cred_transition`. §8-bis evalúa si cierra o queda parcial. |
| **18** | **Los `REQ_SUPERSEDED` no cuentan para elegibilidad de credenciales (ajuste C + requisito nuevo)** | en `request_recorded_count`: `ensures 0 <= \result <= rs->reqs_count;` + loop invariant que solo incrementa cuando `reqs[i].state==REQ_RECORDED` (nunca `REQ_SUPERSEDED`) | **se espera WP (demostrable)** | Gemelo exacto de `experience_validated_count` (`src/experience.c`): búsqueda lineal con `loop invariant 0<=count<=i`, `assigns \nothing`; ese patrón cierra en WP hoy. El filtro por `state==REQ_RECORDED` excluye `SUPERSEDED` por construcción. |
| **19** | **Las rutas heredadas no pueden modificar el nuevo historial oficial (P-SEP, §8-bis)** | `assigns` de toda función de `experience.c`/`credentials.c` NO nombra `reqs[]`/`auth[]` (y ni reciben `requests_state_t`) | **se espera WP (trivial/demostrable)** | Aislamiento de tipos: las firmas viejas no referencian `requests_state_t`; su `assigns` ya existente no menciona esos campos. Detalle en §8-bis (P-SEP). |
| **20** | **Elegibilidad de credenciales = ≥1 `REQ_RECORDED` vigente en `CAT_INSTITUTION_WORKED` o `CAT_LINK_PERIOD` (regla DEFINITIVA, Rev 5)** | `credential_eligible`: `assigns \nothing;` + `ensures \result==0 || \result==1;` + `ensures \result==1 <==> (\exists k: reqs[k].owner_id==owner_id && reqs[k].state==REQ_RECORDED && (reqs[k].category==CAT_INSTITUTION_WORKED || reqs[k].category==CAT_LINK_PERIOD));` y en `credential_issue`: `ensures \result==EDU_OK ==> credential_eligible(rs, owner_id);` | **se espera WP (`assigns`/acotación) + parcial por (G) en el existencial** | Gemelo exacto de `experience_validated_count` (`src/experience.c`): búsqueda lineal con `loop invariant 0<=count<=i`, `assigns \nothing`, sin aritmética de fechas; ese patrón cierra hoy. El bicondicional positivo global cae en (G), con respaldo dinámico. Regla del modelo académico EduChain, no legal. |

**(§8-bis) Invariante transversal — separación historial oficial nuevo vs `experience.c` heredado
(requisito central).**

> **P-SEP (invariante de separación, = P-19):** ninguna función antigua (`experience_validate`,
> `experience_register`, `credential_*`, y las rutas de HR/COORD) puede crear ni mutar un registro del
> historial oficial nuevo (`reqs[]`) ni la tabla de autorizaciones (`auth[]`), y en particular no
> puede producir un `REQ_RECORDED` ni un `REQ_SUPERSEDED`. (Rev 3: la separación cubre también
> `auth[]` y el nuevo estado `REQ_SUPERSEDED`.)

Cómo se **garantiza por construcción** (no solo por contrato):

1. **Aislamiento de estado.** `reqs[]` vive en `requests_state_t`; `exps[]`/`creds[]` viven en
   `educhain_state_t`. Las funciones antiguas tienen firmas que reciben `educhain_state_t *st` y
   **no** reciben `requests_state_t`. A nivel de tipos de C es **imposible** que escriban `reqs[]`:
   no tienen referencia a ese estado. Esto se refleja en ACSL con el `assigns` de cada función
   antigua, que **no nombra** ningún campo de `requests_state_t` (ya es así hoy; no cambia).
2. **Ausencia de ruta de escritura.** La única transición `VERIFIED → RECORDED` la expone
   exclusivamente `request_record`, y `security_can` solo la concede a ADMIN para `OP_RECORD_REQUEST`.
   HR/COORD/PRO/INSTITUTION/VERIFIER obtienen `ERR_PERM` (P-5).
3. **Contrato explícito.** El `assigns` de `request_record` nombra solo `reqs[k].state` (con
   `id==req_id`) y el `log`; nunca `exps[]`/`creds[]`. Y, recíprocamente, los `assigns` de las
   funciones de `experience.c`/`credentials.c` (ya verificados) no nombran `reqs[]`.

Estado esperado en WP: la parte "las funciones antiguas no asignan `reqs[]`" es **trivial y
demostrable** (sus `assigns` existentes no lo mencionan, y ni siquiera reciben el estado). La parte
"solo ADMIN produce `REQ_RECORDED`" se apoya en P-5 (demostrable) + el estado origen exacto (patrón
`cred_transition`). Respaldo dinámico en `tests/test_requests.c`: intentar incorporar vía rutas
viejas (que ni compilan contra `reqs[]`) y verificar que ninguna operación heredada cambia
`reqs_count`.

**(§8-bis.2) Análisis honesto de la doble transición atómica (P-16/P-17, requisito nuevo).**

La pregunta clave del requisito: *¿cerrará en WP el `request_record` de la Variante B, que modifica
el `state` de DOS registros (`req_id` → `RECORDED` y `prev_id` → `SUPERSEDED`) de forma atómica?*
Respuesta honesta, comparando con lo que hoy cierra y lo que no:

- **El `assigns` sobre dos slots (P-17) es una generalización de `cred_transition`.** Hoy
  `cred_transition` cierra con `assigns { st->creds[k].state | k; 0<=k<creds_count && creds[k].id==cred_id }`
  (un solo id, un solo slot). La Variante B necesita
  `assigns { reqs[k].state | k; reqs[k].id==req_id || reqs[k].id==prev_id }` (dos ids). Es la misma
  forma de set-comprehension, con un disyuntor más en el filtro. **Se espera** que WP demuestre la
  inmutabilidad del resto de campos (owner, categoría, `evidence_ref`, metadatos) por framing de ese
  `assigns`, igual que hoy la inmutabilidad de fechas/owner en `experience_validate` (P-5). El riesgo
  es que el filtro con `||` sobre dos ids distintos exija a WP razonar que ambos slots existen y son
  distintos; esa parte **podría quedar parcial** y respaldarse dinámicamente.
- **El doble existencial positivo (P-16, "ambos quedaron en su nuevo estado") cae en (G).** Es el
  mismo límite que ya afecta a P-6 (el existencial global "existe k con `reqs[k].state==from`"): el
  probador no dispone de un invariante global de unicidad, de modo que el consecuente positivo con dos
  existenciales acoplados **probablemente quede Failure/Timeout** y se respalde con pruebas dinámicas.
- **La atomicidad "en error nada se modifica" (la parte crítica del requisito) es la más tratable.**
  Partiendo el contrato en `behavior`s como ya hace `cred_transition` (`success` vs `log_full`), la
  cláusula `assigns \nothing` del comportamiento de error y la comprobación de TODAS las validaciones
  antes de la primera escritura hacen que "si `\result != EDU_OK` entonces
  `\forall k: reqs[k].state == \old(reqs[k].state)`" sea **acotable**. Es exactamente el perfil de la
  atomicidad de contadores que ya se razona en los módulos actuales (P-11): **se espera WP parcial**,
  con el post-estado completo respaldado dinámicamente si el framing (F) lo degrada.
- **Comparación honesta con el estado actual:** la atomicidad de contadores a través de `audit` ya
  queda hoy parcialmente en Failure/Timeout por (F) (`docs/explicacionv1.md` §9). No hay razón para
  esperar que la doble transición cierre *mejor* que eso; lo esperable es el **mismo** perfil:
  negativo/`assigns` acotado demostrable, existenciales positivos parciales. El documento **no**
  promete que P-16 cierre entero en WP.

**Resumen de estados esperados (objetivos, no resultados):** se espera cierre limpio en WP: #1, #2,
#4, #5, #7, #15, #18, #19/P-SEP (aislamiento), el `assigns \nothing`/acotación de #20, y las mitades
negativas de #3, #9, #10, #13, #14. El bicondicional positivo global de #20 (elegibilidad) queda
**parcial por (G)**, con respaldo dinámico, igual que los demás existenciales positivos.
Se espera WP parcial + respaldo dinámico: #6, #11, #12, #16 (atomicidad-en-error y `assigns`), #17
(`assigns` de dos slots). Se espera pendiente por (G), con respaldo dinámico: dual positivo de #3 y
#13, unicidad global de #8, doble existencial positivo de #16. **Ninguna predicción supone un
contraejemplo**: las Failure/Timeout serían límites de automatización, respaldados con
`tests/test_requests.c` (patrón `test_attacks.c`).

---

## 8-ter. Integración `requests.c ↔ credentials.c` (ajuste 3)

**Requisito (ajuste C, cerrado en Rev 3; criterio de elegibilidad CERRADO en Rev 5).** La emisión y
renovación de credenciales deben consultar **exclusivamente la trayectoria oficialmente incorporada y
VIGENTE**: los registros en estado `REQ_RECORDED` **que no han sido sustituidos**. Se **excluyen**:
`REQ_PENDING`, `REQ_ENDORSED`, `REQ_VERIFIED` no incorporadas, `REQ_REJECTED`, los registros heredados
de `experience.c`, y —desde Rev 3— los registros corregidos y sustituidos `REQ_SUPERSEDED`. Como el
conteo cuenta solo `state == REQ_RECORDED`, los `REQ_SUPERSEDED` quedan excluidos **por construcción**
(no hace falta un filtro aparte): tras una corrección, el registro anterior deja de sumar para
elegibilidad.

**REGLA DEFINITIVA DE ELEGIBILIDAD (Rev 5 — CERRADA por el usuario).** Un profesor `owner_id` es
elegible para recibir o renovar una credencial cuando tiene **al menos un** registro vigente
`REQ_RECORDED` perteneciente a **alguna** de estas dos categorías:

- `CAT_INSTITUTION_WORKED`
- `CAT_LINK_PERIOD`

El registro debe estar en `REQ_RECORDED` **vigente**; se excluyen `REQ_SUPERSEDED`, las solicitudes
pendientes (`REQ_PENDING`), respaldadas-no-verificadas (`REQ_ENDORSED`), verificadas-no-incorporadas
(`REQ_VERIFIED`), rechazadas (`REQ_REJECTED`) y los registros heredados de `experience.c`. Ambas
categorías exigen respaldo institucional y verificación del VERIFIER **antes** de incorporarse
(§5.3, §5.0), de modo que un `REQ_RECORDED` de cualquiera de ellas ya pasó por el flujo completo.

> **Es una regla del MODELO ACADÉMICO EduChain, NO una exigencia legal profesional.** El criterio
> modela qué habilita a un docente dentro de la simulación; **no** pretende reflejar un requisito legal
> de la profesión. **No** se añaden requisitos arbitrarios de horas, años de experiencia ni
> publicaciones: la elegibilidad es exclusivamente «≥1 `REQ_RECORDED` vigente en
> `CAT_INSTITUTION_WORKED` o `CAT_LINK_PERIOD`».

> **`request_recorded_count(rs, owner) > 0` genérico YA NO APLICA (Rev 5).** En la Rev 4 el conteo
> genérico `> 0` se marcaba como ejemplo insuficiente y la regla quedaba abierta. En la Rev 5 la regla
> está **cerrada** y es por **esas dos categorías**: tener ≥1 `REQ_RECORDED` de **cualquier** categoría
> **no** basta; debe ser de `CAT_INSTITUTION_WORKED` o `CAT_LINK_PERIOD`. Por eso la elegibilidad se
> consulta con `credential_eligible(rs, owner_id)` (abajo), no con `request_recorded_count > 0`.

**Función conceptual `credential_eligible(rs, owner_id)` (SOLO LECTURA, Rev 5).** Consulta pura que
recorre `rs->reqs[]` y responde 1 si existe al menos un registro del titular en `REQ_RECORDED` vigente
cuya categoría sea `CAT_INSTITUTION_WORKED` o `CAT_LINK_PERIOD`; 0 si no. Es el **patrón gemelo** de
`experience_validated_count` (`src/experience.c`): búsqueda lineal con `loop invariant 0 <= count <= i`
(o un flag booleano acotado), `loop variant`, sin aritmética de fechas y **`assigns \nothing`**. Ese
patrón **cierra hoy en WP**, por lo que su estado esperado es **«se espera demostrable en WP»**
(objetivo, no resultado obtenido).

```c
/* 1 si owner_id es elegible: >=1 registro REQ_RECORDED VIGENTE cuya categoria sea
 * CAT_INSTITUTION_WORKED o CAT_LINK_PERIOD. 0 si no. Consulta de SOLO LECTURA.
 * Excluye REQ_SUPERSEDED, PENDING/ENDORSED/VERIFIED/REJECTED y los registros heredados
 * de experience.c (que viven en educhain_state_t, no en rs). Regla del modelo academico
 * EduChain, NO exigencia legal. Patron gemelo de experience_validated_count. */
/*@ requires \valid_read(rs);
    requires 0 <= rs->reqs_count <= MAX_REQUESTS;
    assigns \nothing;
    ensures \result == 0 || \result == 1;
    // (se espera WP) elegible <=> existe un slot RECORDED vigente del owner en una de las dos cat.
    ensures \result == 1 <==>
            (\exists integer k; 0 <= k < rs->reqs_count
             && rs->reqs[k].owner_id == owner_id
             && rs->reqs[k].state == REQ_RECORDED
             && (rs->reqs[k].category == CAT_INSTITUTION_WORKED
                 || rs->reqs[k].category == CAT_LINK_PERIOD));
*/
int credential_eligible(const requests_state_t *rs, int owner_id);
```

> **Estado esperado (aspiracional, Rev 5).** Se **espera** que WP cierre el `assigns \nothing` y la
> acotación del resultado (`0`/`1`), por ser una consulta pura sin framing (igual que
> `experience_validated_count` y `cred_find_by_owner`, que cierran hoy). El bicondicional positivo con
> existencial global puede caer en la limitación (G) (ausencia de invariante global de unicidad) y
> quedar **parcial**, con respaldo dinámico en `tests/test_requests.c`. **Nada está demostrado
> todavía**: la implementación y la re-ejecución de WP·Eva son Etapa 3+.

**Diseño de la dependencia (lectura, no escritura).** `credentials.c` ya consulta el estado antes de
emitir (`professional_is_active`, `cred_find_by_owner`, todas `assigns \nothing`). Se añade una
**dependencia de lectura** de `credentials.c` hacia `requests.c` mediante una función de consulta
pura:

```c
/* Nº de registros del titular owner_id en el historial VIGENTE: state == REQ_RECORDED.
 * Los REQ_SUPERSEDED NO se cuentan (quedan fuera por el filtro de estado). */
/*@ requires \valid_read(rs);
    requires 0 <= rs->reqs_count <= MAX_REQUESTS;
    assigns \nothing;
    ensures \result == ERR_BAD_ID || (0 <= \result <= rs->reqs_count);
    // el conteo solo suma slots con state == REQ_RECORDED (nunca REQ_SUPERSEDED)
*/
int request_recorded_count(const requests_state_t *rs, int owner_id);
```

Es el **gemelo exacto** de `experience_validated_count` (`src/experience.c`): búsqueda lineal con
`loop invariant 0 <= count <= i`, `assigns \nothing`, sin aritmética de fechas. Ese patrón **cierra
en WP** hoy.

**Cómo se integra sin reintroducir el framing.** Clave: la dependencia es de **solo lectura**.

- `credential_issue`/renovación reciben además `const requests_state_t *rs` y, antes de emitir,
  aplican la **regla de elegibilidad definitiva (Rev 5)** consultando `credential_eligible(rs,
  owner_id)` sobre `rs`. Al ser `rs` de solo lectura y la consulta `assigns \nothing`, **no amplía el
  `assigns` de `credential_issue`** ni mete `reqs[]` en el frame de escritura. El framing problemático
  (F) viene de `audit_record_event` sobre el struct que se **escribe**; una consulta `assigns \nothing`
  sobre un struct **distinto** que solo se **lee** no agrava ese frame.
- Como `requests_state_t` es **separado** (§4.5), el struct que `credential_issue` escribe sigue
  siendo `educhain_state_t` (~7 KB), sin crecer. La lectura de `rs` añade solo precondiciones de
  `\valid_read`, que WP trata como las de cualquier consulta.

**Qué función usa `credentials` y con qué contrato (Rev 5).** `credential_eligible(rs, owner_id)`
(definida arriba), de solo lectura. El contrato de `credential_issue` se amplía con la lectura de `rs`
y una postcondición que exige elegibilidad para emitir:

```c
/*@ requires \valid(st) && \valid_read(rs);
    requires 0 <= rs->reqs_count <= MAX_REQUESTS;
    assigns { st->creds[k].state | integer k; 0 <= k < st->creds_count && st->creds[k].id == cred_id },
            st->log[st->log_count], st->log_count;   // SIN cambios: rs se LEE, no entra en el assigns
    // (Rev 5, se espera WP) emitir exige elegibilidad academica EduChain:
    ensures \result == EDU_OK ==> credential_eligible(rs, owner_id);
    ensures !credential_eligible(rs, owner_id) ==> \result != EDU_OK;
    // ... el resto del contrato actual de credential_issue (space/no_space, ERR_PERM/ERR_BAD_ID) se
    //     conserva; el cambio es ADITIVO y de SOLO LECTURA sobre rs (no amplia el frame de escritura).
*/
int credential_issue(educhain_state_t *st, const requests_state_t *rs,
                     role_t caller_role, int caller_id, int cred_id, int owner_id);
```

La forma es análoga al uso actual de `professional_is_active` dentro de `credential_issue`: una guarda
de solo lectura que, si no se cumple, impide `EDU_OK`. El código de error concreto para «no elegible»
(p. ej. `ERR_PERM` o un `ERR_*` existente de `include/common.h`) se fija en la Etapa 4; el diseño no
inventa un error nuevo.

**Regla de elegibilidad = DECISIÓN CERRADA (Rev 5), ya NO un parámetro abierto.** En la Rev 4 la
elegibilidad era «el único parámetro que debe definir el usuario». En la Rev 5 **el usuario la ha
definido**: `credential_eligible` = «≥1 `REQ_RECORDED` vigente en `CAT_INSTITUTION_WORKED` o
`CAT_LINK_PERIOD`». El diseño ya no deja ninguna forma alternativa a elegir; la **fuente** sigue siendo
**exclusivamente** el historial `REQ_RECORDED` vigente (excluye `REQ_SUPERSEDED`,
`REQ_PENDING`/`ENDORSED`/`VERIFIED`/`REJECTED` y los registros heredados de `experience.c`), y el
**criterio** queda fijado por las dos categorías.

> **Nota de alcance — REAPERTURA de WP de `credentials.c` (hecho asumido, Rev 4; ya NO es pregunta).**
> Hoy `credential_issue` tiene la firma `credential_issue(educhain_state_t *st, role_t, int caller_id,
> int cred_id, int owner_id)` (`src/credentials.c`) y **no recibe** `requests_state_t`. La decisión
> **tomada** (integración DIRECTA de solo lectura) es añadir el parámetro `const requests_state_t *rs`
> y la consulta `credential_eligible(rs, owner_id)` (Rev 5) en `credential_issue`. Esto **modifica
> `credential_issue` y, por tanto, REABRE la verificación WP de `credentials.c`** (298/315 goals
> probados hoy, según
> `verification/wp_results_fase4_credentials.txt` y `docs/explicacionv1.md` §7). En consecuencia hay
> que **re-ejecutar WP y comparar contra la línea base anterior**: NO se garantiza que los resultados
> viejos sigan válidos automáticamente. El riesgo se acota porque el cambio es **aditivo y de solo
> lectura** sobre un struct **distinto** (`rs` no entra en el frame de escritura, que sigue siendo
> `educhain_state_t`), pero el objetivo —0 regresiones sobre los 298 goals— debe **comprobarse**, no
> presuponerse. **La alternativa de "no tocar `credentials.c` y aplicar la regla en una capa nueva"
> queda DESCARTADA**: ya no es un punto abierto (se elimina de §14). Se documenta aquí como hecho
> asumido del diseño, no como pregunta.

---

## 9. Impacto sobre la verificación formal ya conseguida

### 9.1 Qué se conserva **intacto** (no se reabre)

Con la recomendación de **no editar** `experience.c`, `professionals.c`, `audit.c` ni sus cabeceras,
y de **aislar** el estado nuevo en `requests_state_t`, sus resultados WP/Eva actuales siguen siendo
válidos. **El aislamiento del estado es lo que protege estas cifras**: si se hubiera ampliado
`educhain_state_t`, el framing (F) habría crecido para todas ellas (§4.5).

| Módulo | Línea base (`wp_results_baseline.txt`) | Fase 4 (`wp_results_fase4_*.txt`, `docs/explicacionv1.md` §7) |
|--------|----------------------------------------|---------------------------------------------------------------|
| `security.c` | 69/69 | **114/114** (0 Failure, 0 Timeout) |
| `audit.c` | 68/74 | 95/95 |
| `professionals.c` | 62/74 | 78/81 |
| `experience.c` | 75/106 | 146/179 |
| `credentials.c` | 52/86 | 298/315 |
| Eva | — | 0 alarmas; 91/91 precondiciones; 17 assertions (0 inválidas) |

**La línea base (`verification/wp_results_baseline.txt`) y la sección LÍNEA BASE permanecen
inalteradas**, como exige la restricción dura.

**Qué se conserva POR AISLAMIENTO vs qué DEBE RE-EJECUTARSE (requisito explícito, Rev 3).**

| Módulo | ¿Se edita? | Estado de su verificación | Base |
|--------|-----------|---------------------------|------|
| `experience.c` / `professionals.c` / `audit.c` (+ cabeceras) | **No** | **Conservado por aislamiento arquitectónico**: sus archivos no se tocan y el estado nuevo vive en `requests_state_t` separado, así que el framing (F) de `audit_record_event` no crece para ellos. Sus cifras WP/Eva siguen válidas **sin** re-ejecutar. | §4.5, §10-D5 |
| `security.c` (+ `security.h`, `specifications.h`) | **Sí** (aditivo: nuevas OPs en el `switch` y en `role_can`) | **DEBE re-ejecutarse WP.** No se garantiza que 114/114 se conserve automáticamente; se compara contra la baseline. Riesgo bajo (prueba pura), pero comprobado, no presupuesto. | §9.2 |
| `credentials.c` | **Sí** (aditivo: lee `requests_state_t` en `credential_issue`) | **DEBE re-ejecutarse WP.** Objetivo: 0 regresiones sobre 298/315; se compara contra la baseline. | §8-ter |
| `requests.c` | **Nuevo** | Se verifica **desde cero** (WP + Eva + pruebas dinámicas), incluidas las OPs de autorización y la sustitución atómica. | §8, §8-bis |

Regla honesta: lo conservado por aislamiento **no** se re-verifica porque no cambió; lo que sí cambia
(`security.c`, `credentials.c`) **no** hereda su resultado anterior automáticamente y se re-ejecuta.

### 9.2 Qué habría que **construir de cero** (no rehacer)

- Contratos ACSL completos de `src/requests.c` (submit/endorse/verify/reject/record con sus dos
  variantes, `authz_grant`/`authz_revoke`, y las consultas) — trabajo nuevo. Incluye la doble
  transición atómica de la corrección (P-16/P-17), que entra en WP, Eva y pruebas dinámicas.
- Extensión aditiva de `security_can` + contrato bicondicional: WP reprueba **security.c** (el
  `switch` crece con `OP_SUBMIT/ENDORSE/VERIFY/REJECT/RECORD_REQUEST` y
  `OP_GRANT/REVOKE_REPRESENTATION`). Riesgo bajo: misma clase de prueba pura que ya cierra 114/114,
  pero **debe re-ejecutarse** y compararse con la baseline (no se presupone).
- Extensión aditiva de `role_can` en `verification/specifications.h` (añadir las OPs nuevas al término
  exhaustivo; hoy ese término cierra con `\false` para toda op fuera de la lista y para VERIFIER —
  habrá que **reformular** la cláusula `r == ROLE_VERIFIER ==> \false`, §10-D2).
- **Cambio deliberado:** `credential_issue` pasa a leer `requests_state_t` (§8-ter) — **reabre WP de
  `credentials.c`**, cambio aditivo de solo lectura que **debe re-verificarse** contra la baseline.

### 9.3 Riesgo cuantificado (actualizado tras los ajustes)

- **`security.c`:** debe reabrirse (toda OP nueva necesita rama). Mejor historial del repo
  (114/114), riesgo de regresión el más bajo.
- **`credentials.c`:** se reabre por el ajuste 3 (lectura de `rs`). Cambio aditivo/solo lectura;
  objetivo: 0 regresiones sobre los 298 goals probados.
- **`requests.c`:** hereda el perfil de `experience.c`/`credentials.c`: parte demostrable, parte
  Failure/Timeout por (F)/(G), con respaldo dinámico.
- **Estado separado (ajuste 4):** es lo que **evita** degradar `experience.c`/`professionals.c`/
  `audit.c`. El struct nuevo (~9.5 KB) solo pesa en el framing de `requests.c`, no en los módulos
  viejos. **Si en cambio se hubiera ampliado `educhain_state_t` a ~16.5 KB, el riesgo sería alto**
  para esos módulos ya cerrados — razón directa de la Decisión 5.
- **Metadatos por categoría (ajuste 1):** cuatro `int` planos; no añaden bucles ni aritmética pesada;
  impacto de framing proporcional al tamaño del struct (ya contabilizado en §4.5).
- **Memoria:** `requests_state_t` ~9.5 KB con `MAX_REQUESTS=128` (ajustable a 64 → ~5 KB). Total del
  programa `educhain_state_t` (~7 KB) + `requests_state_t` (~9.5 KB) ≈ 16.5 KB, pero **nunca en un
  solo struct**: cada uno acota su propio framing.
- **Capacidad y atomicidad (ajuste D):** `tests/test_requests.c` debe probar `reqs[]` llena
  (`MAX_REQUESTS=128`) y `auth[]` llena (`MAX_AUTHZ=64`) → inserción rechazada con **`ERR_FULL`** sin
  mutar contadores; y la **atomicidad ante errores** (toda operación fallida deja `reqs_count`,
  `auth_count` y los `state` como estaban), incluida la doble transición de la corrección (si una
  validación de la Variante B falla, ni la nueva ni el anterior se modifican).
- **Pruebas:** `tests/test_requests.c` debe cubrir las ≥20 propiedades + P-SEP como pruebas
  adversariales (patrón `test_attacks.c`): auto-aprobación, verificar las propias (VERIFIER
  `submitter_id==caller_id`), representar sin autorización, operar tras revocación, grant duplicado,
  verificar sin respaldo cuando la categoría lo exige, INSTITUTION que presenta y nace `ENDORSED`,
  proyecto de investigación institucional vs externo, incorporar desde PENDING/REJECTED/ENDORSED,
  reincorporar, duplicados, corrección (`prev_id` inexistente, anterior no `RECORDED`, distinto owner,
  re-sustituir un `SUPERSEDED`, incoherencia categoría/institución, **corrección que intenta cambiar de
  institución o de categoría → `ERR_BAD_TRANSITION`, T-1**), que el anterior conserve su contenido
  salvo `state`, que `request_recorded_count` excluya los `SUPERSEDED`, **elegibilidad P-20
  (`credential_eligible`): elegible con ≥1 `REQ_RECORDED` en `CAT_INSTITUTION_WORKED`/`CAT_LINK_PERIOD`,
  NO elegible con registros de otras categorías, con `REQ_SUPERSEDED`/no-incorporados o con registros
  heredados de `experience.c`**, actor real en auditoría, y que ninguna ruta vieja toque
  `reqs[]`/`auth[]`.

---

## 10. Comparación de las decisiones (ventajas / desventajas / riesgos / recomendación)

### Decisión 1 — Arquitectura: `requests.c` nuevo vs absorber en `experience.c`

| | Módulo nuevo `requests.c` | Absorber en `experience.c` |
|---|---------------------------|----------------------------|
| **Ventajas** | Aísla el flujo; no toca módulos verificados; prueba de cero sin regresión; separación clara. | Menos archivos; reutiliza `exps[]`. |
| **Desventajas** | Un estado `reqs[]` más (peso de struct, acotado por §4.5). | Reabre WP de `experience.c` (146/179); mezcla dos máquinas de estado; riesgo de ruta paralela. |
| **Riesgos** | Duplicación de patrón (mitigable copiando el probado). | **Alto**: romper especificaciones demostradas; crear la "ruta vieja que salta el flujo" prohibida. |

**Recomendación (conservada): módulo nuevo `requests.c`.** El aislamiento satisface "no romper la
verificación" e "impedir que funciones antiguas salten el flujo": `reqs[]` es estado distinto,
`experience_validate` ni lo recibe, y no se reabre el WP de `experience.c`.

### Decisión 2 — Roles: mantener HR/COORD vs migrar a 4 actores

**Recomendación (conservada): híbrido disciplinado** — migrar conceptualmente a 4 actores, conservar
los valores enum y **negar a HR/COORD toda operación del flujo nuevo** (§3.3). Se añade
`ROLE_INSTITUTION`, se reactiva `ROLE_VERIFIER`, `ROLE_PRO` actúa como PROFESSOR. Cumple "separación
estricta, ningún rol salta el flujo" sin eliminar funcionalidades (99/99 tests + CLI intactos).

**Encaje del `ROLE_VERIFIER` actual (contrato que cambia de forma):** hoy está demostrado **sin**
escritura (`security.h`: `(caller_role==ROLE_VERIFIER) ==> (\result==0)`; `specifications.h`:
`r==ROLE_VERIFIER ==> \false`). El VERIFIER nuevo **sí** aprueba/rechaza. La migración añade a
`security_can` las cláusulas para `OP_VERIFY_REQUEST`/`OP_REJECT_REQUEST` (y solo esas), y
**reformula** ese `ensures`/predicado a "VERIFIER solo puede verificar/rechazar solicitudes". Es el
único punto donde un contrato existente cambia de forma; se documenta como cambio deliberado en
Etapa 4.

### Decisión 3 — Evidencias: SHA-256 real vs identificador/compromiso opaco

**Recomendación (conservada y reforzada): identificador opaco (`int evidence_ref`), sin SHA-256 ni
criptografía.** Se deja **explícito** (§4.4) que NO es un hash, NO garantiza integridad documental y
que la veracidad la aporta el VERIFIER, no el campo. El contrato conserva la referencia **exacta** en
la incorporación (el `assigns` de `request_record` no incluye `evidence_ref`). Fundamento: el proyecto
excluye cripto por alcance (`docs/explicacionv1.md` §1); un hash real agravaría el cuello de botella
de WP (§9); "un hash tampoco demuestra veracidad".

### Decisión 4 — Modelo de datos: struct genérico + enum vs structs por categoría

**Recomendación (conservada y ampliada): un único struct genérico con enum de categoría y un conjunto
acotado de campos numéricos reinterpretados** (§4.2). Los metadatos pedidos (horas asignadas/
realizadas, cursos, períodos) caben en `num_a..num_d` + `start/end_date`, sin inflar a 14 variantes ni
usar `union`. 14 structs multiplicarían el framing (ya cuello de botella, §4.5) y el número de
contratos; el enum de categoría es un `int` que no añade framing.

### Decisión 5 (NUEVA, ajuste 4) — Estado: `requests_state_t` separado vs ampliar `educhain_state_t`

| | `requests_state_t` separado | Ampliar `educhain_state_t` global |
|---|-----------------------------|-----------------------------------|
| **Ventajas** | **Aísla el peso nuevo**: los módulos verificados siguen razonando sobre el mismo ~7 KB; protege 114/114, 146/179, 298/315, etc. Separación física que refuerza P-SEP (las funciones viejas ni reciben el estado nuevo). | Un solo struct que pasar; menos parámetros. |
| **Desventajas** | Las funciones de `requests.c` reciben dos punteros (`st` para auditar, `rs` para el flujo); la integración con credenciales pasa `const requests_state_t*` extra. | El struct global crece a ~16.5 KB (§4.5). |
| **Riesgos** | Bajo: patrón de "dos estados" es mecánico; `audit_record_event` sigue operando sobre `educhain_state_t`. | **Alto**: el framing (F) a través de `audit_record_event` empeora para TODOS los módulos que lo llaman; obligaciones hoy cerradas (atomicidad de contadores, relaciones de post-estado) podrían degradar a Timeout. Regresión sobre lo ya conseguido. |

**Recomendación: `requests_state_t` separado.** El punto decisivo es el **framing de WP**: el struct
~7 KB a través de `audit_record_event` ya es, según `docs/explicacionv1.md` §9, una de las dos causas
de Failure/Timeout. Ampliarlo arrastraría a los módulos verificados; aislarlo los deja intactos y
confina el peso nuevo a `requests.c`. **Convivencia de ambos estados:** el log de auditoría sigue en
`educhain_state_t` (un solo historial append-only); `requests.c` recibe `educhain_state_t *st` (para
`audit_record_event`) y `requests_state_t *rs` (para `reqs[]`/`auth[]`). **Cómo lee `credentials.c`
(que vive en `educhain_state_t`) el estado de requests:** recibe un `const requests_state_t *rs`
adicional y llama a la consulta `assigns \nothing` `request_recorded_count` (§8-ter); al ser lectura
sobre un struct distinto, no amplía su frame de escritura.

### Decisión 6 (Rev 3, ajuste D) — Capacidades `MAX_REQUESTS=128`, `MAX_AUTHZ=64`

**Recomendación/decisión: `MAX_REQUESTS = 128` y `MAX_AUTHZ = 64` para la simulación.** Implicaciones:

- **Memoria.** `requests_state_t` ≈ `reqs[128]` (~8.7 KB) + `auth[64]` (~0.75 KB) ≈ **~9.5 KB**
  (§4.5), separado de `educhain_state_t` (~7 KB). El total del programa (~16.5 KB) **nunca** vive en
  un solo struct: cada uno acota su propio framing.
- **Verificación formal.** El tamaño del estado separado solo pesa en el framing de WP de
  `requests.c`; no toca a los módulos aislados. Un `MAX_REQUESTS` menor (p. ej. 64) reduciría ese
  framing; 128 es el valor decidido para la simulación por dar holgura a escenarios con correcciones
  (cada corrección es un `reqs[]` nuevo que coexiste con el `SUPERSEDED`).
- **Pruebas.** Capacidad llena, rechazo con `ERR_FULL` y atomicidad ante errores entran en
  `tests/test_requests.c` (§9.3).

> **No definitivo para producción.** Estos límites son adecuados para la **simulación** del contrato;
> el documento **no** afirma que sean los valores definitivos de una implementación de producción.

---

## 11. Plan de migración por etapas (preservando lo necesario)

Cada etapa deja el repo compilando y con 99/99 tests + WP sin regresiones. **Etapa 1 (esta) es
READ-ONLY y queda completa con este documento (Revisión 5).**

| Etapa | Nombre | Alcance | Verificación de cierre |
|-------|--------|---------|------------------------|
| **1** | Análisis y diseño | Este documento (Rev 5). READ-ONLY. **La Etapa 2 solo empieza tras aprobación expresa del usuario sobre este documento actualizado.** | `git status` muestra solo `docs/diseno_flujo_solicitudes.md`; secciones + ajustes A–D + `REQ_SUPERSEDED` + correcciones Rev 4 (1–6) + cierres Rev 5 (elegibilidad `credential_eligible`, T-1, T-2) presentes y coherentes. |
| **2** | Arquitectura de solicitudes | `requests.h` (prototipos, sin cuerpo), enums/struct/`requests_state_t` en `common.h` (incl. `REQ_SUPERSEDED`, `prev_id`), `MAX_REQUESTS=128`/`MAX_AUTHZ=64`, OPs (incl. `OP_GRANT/REVOKE_REPRESENTATION`) y roles (append-only). | Compila `gcc -std=c99 -Wall -Wextra -Iinclude`; `bash run_tests.sh` sigue 99/99. |
| **3** | Estructuras y funciones C | Implementar `src/requests.c` (`request_submit` + `request_submit_correction` con la coherencia categoría/institución T-1; endorse/verify/reject/record con Variantes A/B; `authz_grant`/`authz_revoke`; primitivas de consulta + capa pública `request_*_for`; `credential_eligible`) estilo `cred_transition`/`credential_issue`/`experience_validated_count`; casos en `security_can`; `requests_is_representative`, `request_requires_endorsement` y la regla `exige_respaldo` con `institution_id`. | `bash run_tests.sh` 99/99 + nuevos casos de `tests/test_requests.c`. |
| **4** | ACSL | Contratos nuevos (incl. `authz_*` partidos por `behavior` sin `ERR_NULL` ni `auth[auth_count]` fuera de rango; doble transición atómica P-16/P-17, `assigns` de dos slots; capa pública P-SCOPE; `credential_eligible` P-20 con `assigns \nothing`); extender `role_can` y `security.h` (reformular `ensures` de VERIFIER, §10-D2); ampliar `credential_issue` con la lectura de `rs` y la postcondición `credential_eligible` (§8-ter). | Revisión de contratos; `experience/professionals/audit` **sin cambios** → sus WP intactos. |
| **5** | WP + Eva | `bash verification/verify.sh`; **re-ejecutar y comparar contra baseline** `security.c` y `credentials.c`. Objetivo: ningún goal previamente probado pasa a Failure; documentar Failure/Timeout nuevos como (F)/(G). | `security.c` y `credentials.c` reprobados sin regresión; `requests.c` con perfil §8; Eva 0 alarmas. |
| **6** | Tests | Completar `tests/test_requests.c`: ≥20 propiedades + P-SEP + capacidad/atomicidad (§9.3) + validaciones de corrección (incl. T-1) + elegibilidad P-20 (`credential_eligible`), como pruebas adversariales (patrón `test_attacks.c`). | `bash run_tests.sh` normal + ASan/UBSan, 0 fallos. |
| **7** | Documentación | Actualizar `docs/explicacionv1.md`/`formal_verification.md` con el flujo nuevo y las cifras WP/Eva releídas de los artefactos regenerados. | Documento coherente con artefactos. |

**Funcionalidades heredadas que se preservan (no se tocan):** `professional_*`, `experience_*`,
`audit_*` y sus contratos; la CLI `ui/educhain_cli.c`; la línea base
`verification/wp_results_baseline.txt`. `credential_*` se preserva salvo la ampliación aditiva de
lectura de §8-ter (Etapa 4): `credential_issue` recibe `const requests_state_t *rs` y consulta
`credential_eligible` (regla DEFINITIVA Rev 5, ya no un punto a confirmar).

---

## 12. Resumen de las decisiones CERRADAS en la Revisión 5 y herencia (entregable 1)

### 12.-1 Decisiones CERRADAS en la Revisión 5 (sobre la Rev 4) — las tres que estaban abiertas

Estas tres eran las únicas decisiones de negocio que la Rev 4 dejaba abiertas (un punto abierto y dos
tensiones residuales). La Rev 5 las **cierra definitivamente**:

1. **Elegibilidad de credenciales — CERRADA.** Regla definitiva: un profesor es elegible para recibir
   o renovar una credencial cuando tiene **al menos un** registro vigente `REQ_RECORDED` de categoría
   `CAT_INSTITUTION_WORKED` **o** `CAT_LINK_PERIOD`. Se formaliza con la función conceptual de solo
   lectura `credential_eligible(rs, owner_id)` (`assigns \nothing`), gemela de
   `experience_validated_count`, consultada por `credential_issue`. Se excluyen `REQ_SUPERSEDED`,
   pendientes, respaldadas-no-verificadas, verificadas-no-incorporadas, rechazadas y los registros
   heredados de `experience.c`. Es una regla del **modelo académico EduChain**, no legal; sin
   requisitos de horas/años/publicaciones. `request_recorded_count > 0` genérico **ya no aplica**.
   (§8-ter, §8 P-20, §11, §14.)
2. **Tensión T-1 (categoría/institución en correcciones) — CERRADA.** La categoría de la corrección
   **siempre** coincide con la del original; si la categoría asocia institución, ambos registros tienen
   el **mismo** `institution_id`; si no la asocia, ambos tienen `institution_id = 0`. **No** se cambia
   la institución por una corrección. Validación actualizada en `request_submit_correction`
   (validación-S) y `request_record` (validación-R). **T-1 retirada de pendientes.** (§5.5 validación 4,
   §7.1.)
3. **Tensión T-2 (cadena de `prev_id`) — CERRADA.** **No** se recorre explícitamente la cadena. La
   aciclicidad de las correcciones que llegan al historial se apoya en las transiciones de estado
   (`prev_id` apunta a un registro existente y vigente; el anterior pasa a `REQ_SUPERSEDED` al
   incorporar; un `REQ_SUPERSEDED` no vuelve a ser `prev_id`; la revalidación al incorporar impide dos
   sustituciones del mismo registro), no del orden de ids ni de recorrer la cadena. Se mantienen las
   validaciones al presentar y al incorporar. **T-2 retirada de pendientes.** (§5.5.)

**Estado final de las decisiones de negocio:** NO queda pendiente **ninguna** de las tres. Siguen
aprobadas sin cambios: integración directa con `credentials.c` (solo lectura sobre `REQ_RECORDED`
vigente), gestión de `auth[]` exclusiva de ADMIN, reglas de respaldo institucional condicional,
sustitución atómica vía `REQ_SUPERSEDED` obligatoria, y `requests_state_t` separado de
`educhain_state_t`. Lenguaje aspiracional para todo resultado WP·Eva nuevo (incluida P-20).

### 12.0 Correcciones aplicadas en la Revisión 4 (sobre la Rev 3)

1. **`prev_id` resuelto (§4.2, §5.5, §6, §7).** Se decide una **función específica
   `request_submit_correction`** en vez de añadir `prev_id` a `request_submit` (justificación: separa
   los dos contratos, patrón de wrappers de `credentials.c`). `prev_id` se valida **al presentar**
   (validación-S) y **de nuevo al incorporar** (validación-R en `request_record`, porque el estado del
   anterior pudo cambiar). Carrera de dos correcciones resuelta: la primera incorporación gana; la
   segunda falla con `ERR_BAD_TRANSITION` al revalidar. Aciclicidad por **transiciones de estado**
   (un `RECORDED` se sustituye una vez; un `SUPERSEDED` no vuelve a ser `prev_id`), no por orden de
   ids. Corrección cuyo anterior dejó de estar vigente: queda inerte en `REQ_VERIFIED`; vía de salida =
   presentar una nueva corrección apuntando al registro vigente actual.
2. **Flujo unificado (§5.0, §5, §6, §7).** Reglas de nacimiento en una **única fuente** (§5.0),
   referenciada por máquina de estados, flujo y firmas: PROFESSOR+respaldo→`PENDING`;
   INSTITUTION+respaldo→`ENDORSED` (sin segunda operación); sin respaldo→`PENDING`. El respaldo nunca
   sustituye al VERIFIER; se elimina todo paso que obligue a una institución a re-respaldar su propia
   presentación.
3. **Credenciales (§8-ter, §14).** Integración DIRECTA de solo lectura **como hecho asumido** (se
   modifica `credential_issue` sobre `rs`, reabre y re-verifica WP de `credentials.c`). Se **elimina**
   de §14 la pregunta «`credential_issue` vs capa nueva». Único pendiente: el **criterio de
   elegibilidad** del usuario; `request_recorded_count > 0` marcado como ejemplo insuficiente. Fuente
   exclusiva: historial vigente `REQ_RECORDED`.
4. **Consultas y permisos (§3.3 bis, §7.3).** Se separan **primitivas internas** (sin rol,
   `assigns \nothing`) de la **capa pública** (`request_*_for`) con rol + identidad + ámbito; matriz
   de consulta por PROFESSOR/INSTITUTION/VERIFIER/ADMIN; principio P-SCOPE.
5. **ACSL de `authz_grant`/`authz_revoke` (§3.2 bis).** `ERR_NULL` retirado de los resultados
   alcanzables (inalcanzable por `\valid`, patrón `credential_issue`); `assigns` partido por
   `behavior` sin nombrar `auth[auth_count]` fuera de rango; comportamientos
   insert/reactivate/duplicate/full `complete`+`disjoint`; atomicidad y ADMIN-exclusivo conservados.
6. **Consistencia general.** Referencias a «Revisión 2» obsoletas actualizadas; afirmación de «sin
   contradicciones» corregida (§14 reconoce tensiones residuales); firmas/estados/flujo/ACSL/etapas
   alineados; lenguaje aspiracional para todo resultado WP·Eva nuevo.

### 12.1 Decisiones heredadas y cerradas desde la Revisión 3

1. **A — Respaldo institucional por categoría (cerrado).** Tabla §5.3 fijada (no ya propuesta):
   10 categorías exigen respaldo; 3 (formación/actualización, certificaciones, publicaciones) no.
   `CAT_RESEARCH_PROJECT` es **condicional** por `institution_id` presente/ausente (§5.3 bis). Y regla
   nueva: una **INSTITUTION que presenta** una categoría que exige respaldo nace `REQ_ENDORSED` con
   `endorser_id=institution_id`, sin segunda operación; una del **PROFESSOR** queda `PENDING` hasta
   `request_endorse` (§5.3 ter). El respaldo no reemplaza la verificación del VERIFIER.
2. **B — Gestión de `auth[]` por ADMIN en runtime (cerrado).** `authz_grant`, `authz_revoke`
   (ADMIN-exclusivas, auditadas, sin duplicados activos, revocación no borra y no es retroactiva) y
   `requests_is_representative` (consulta). Estados `active ∈ {1,0}`; errores y contratos en §3.2 bis.
3. **C — Integración con `credentials.c` (cerrada).** Lectura exclusiva del historial vigente
   `REQ_RECORDED` (excluye `REQ_SUPERSEDED` y las rutas heredadas); elegibilidad como **parámetro de
   diseño** que define el usuario; `assigns \nothing` sobre `rs`; **reabre y re-verifica** WP de
   `credentials.c` (§8-ter).
4. **D — Capacidades (cerradas).** `MAX_REQUESTS=128`, `MAX_AUTHZ=64` para la simulación, no
   definitivas para producción; implicaciones de memoria/WP y pruebas de capacidad/atomicidad
   (§4.5, §9.3, §10-D6).
5. **Requisito nuevo — `REQ_SUPERSEDED`.** Corrección como nueva solicitud con `prev_id`; `request_record`
   con dos variantes (simple / con sustitución); doble transición **atómica**
   (`nueva→RECORDED` y `anterior→SUPERSEDED`); validaciones de `prev_id` (existencia, anterior
   `RECORDED`, mismo owner, coherencia categoría/institución, no re-sustituir, no ciclos);
   inmutabilidad-salvo-`state` del anterior; `request_recorded_count` excluye `SUPERSEDED`
   (§4.1, §5.5, §6, §8 P-16/P-17/P-18, §8-bis.2).

## 13. Secciones modificadas (entregable 3)

### 13.-1 Modificadas en la Revisión 5 (sobre la Rev 4)

- **Encabezado:** banner Revisión 5 y bloque de cambios (cierres 1–3 + estado final + procedencia).
- **§5.5 (validación 4 de la Variante B):** coherencia categoría/institución reescrita como **regla
  única y definitiva** (T-1 cerrada): misma categoría; mismo `institution_id` si la categoría asocia
  institución, o ambos `0` si no; nunca se cambia la institución. Y el bloque de aciclicidad: decisión
  definitiva de **no recorrer la cadena `prev_id`** (T-2 cerrada), aciclicidad por transiciones de
  estado.
- **§7.1:** la fila de `request_submit_correction` incorpora la validación-S de coherencia
  categoría/institución T-1.
- **§8 (tabla de propiedades):** nueva **P-20** (elegibilidad = ≥1 `REQ_RECORDED` vigente en
  `CAT_INSTITUTION_WORKED` o `CAT_LINK_PERIOD`; `credential_eligible`, `assigns \nothing`, estado «se
  espera WP»); título a «≥20»; resumen de estados esperados incluye P-20.
- **§8-ter:** regla DEFINITIVA de elegibilidad; función `credential_eligible(rs, owner_id)` (patrón
  `experience_validated_count`); contrato de `credential_issue` ampliado con `rs` y la postcondición
  de elegibilidad; aclaración de que `request_recorded_count > 0` genérico ya no aplica y de que es
  regla académica, no legal; eliminadas las «formas posibles del parámetro».
- **§11 (plan de migración):** Etapa 1 = Revisión 5; Etapas 3–4 incluyen `credential_eligible` y la
  coherencia T-1; nota final sin «punto a confirmar» sobre `credential_*`.
- **§12 / §14 / §15 / apéndices:** entregables actualizados a Revisión 5; §14 declara que **no queda
  ninguna decisión de negocio abierta** (T-1, T-2 y elegibilidad retiradas de pendientes).

### 13.0 Modificadas en la Revisión 4 (sobre la Rev 3)

- **Encabezado:** banner y bloque de cambios de Revisión 4 (correcciones 1–6).
- **§3.2 bis:** contrato ACSL de `authz_grant`/`authz_revoke` reescrito (sin `ERR_NULL` alcanzable,
  `behaviors` insert/reactivate/duplicate/full, `assigns` sin `auth[auth_count]` fuera de rango).
- **§3.3 / §3.3 bis (nueva):** matriz de consulta por rol; primitivas internas vs capa pública P-SCOPE.
- **§5.0 (nueva):** reglas canónicas de nacimiento (fuente única); §5, §6, §7 la referencian.
- **§5.5 / §5.6:** decisión `request_submit_correction`; doble validación de `prev_id`; carrera de dos
  correcciones; aciclicidad por transiciones; corrección con anterior ya no vigente.
- **§6 (pasos 3–4):** nacimiento según §5.0; endorse solo cuando aplica.
- **§7.1 / §7.3 / §7.4:** `request_submit_correction` y capa pública `request_*_for` añadidas;
  primitivas internas distinguidas; «Eliminadas» renumerada a §7.4.
- **§8-ter / §14:** integración directa como hecho asumido; eliminada la pregunta de capa nueva;
  `request_recorded_count > 0` marcado insuficiente.
- **§11:** Etapa 1 = Revisión 4; etapas 3–4 con las funciones/contratos nuevos.
- **§12 / §14 / §15:** entregables actualizados a Revisión 4.

### 13.1 Modificadas en la Revisión 3 (histórico)

- **Encabezado:** nota de Revisión 3.
- **§3.2 / §3.2 bis:** `auth[]` gestionado por ADMIN en runtime; tres operaciones, estados, errores,
  contratos.
- **§3.3:** matriz rol×operación con `OP_GRANT/REVOKE_REPRESENTATION` y nota de variantes de `record`.
- **§4.1:** enum con `REQ_SUPERSEDED=5` y su semántica.
- **§4.3 / §4.5 / §10-D6:** `MAX_REQUESTS=128`, `MAX_AUTHZ=64` fijados.
- **§5.1:** diagrama de estados con `REQ_SUPERSEDED` y ramas de presentación por rol.
- **§5.3 / §5.3 bis / §5.3 ter:** tabla de respaldo decidida; proyecto de investigación condicional;
  INSTITUTION-presenta-ya-respaldado.
- **§5.4 / §5.5:** transiciones con variantes de presentación y la corrección con doble transición.
- **§6:** flujo con la Variante B y la sustitución atómica.
- **§7.1 / §7.2:** funciones nuevas (`authz_*`), `request_record` con variante, OPs en `common.h`.
- **§8:** propiedades ampliadas a ≥19 con etiquetas se-espera/dinámica/parcial; lenguaje aspiracional.
- **§8-bis / §8-bis.2:** P-SEP extendido a `auth[]`/`SUPERSEDED`; análisis honesto de P-16/P-17.
- **§8-ter:** integración solo-lectura sobre `REQ_RECORDED` vigente; elegibilidad como parámetro;
  reapertura de WP de `credentials.c`.
- **§9:** impacto formal con tabla conservado-por-aislamiento vs debe-reejecutarse; pruebas de
  capacidad/atomicidad.
- **§11:** plan de etapas actualizado; Etapa 2 solo tras aprobación.

## 14. Estado de las decisiones de negocio: NINGUNA pendiente (entregable 4)

**Confirmación EXPLÍCITA (Rev 5): NO queda abierta ninguna decisión de negocio.** Las tres que la
Revisión 4 dejaba pendientes (un punto abierto y dos tensiones residuales) están **cerradas
definitivamente** por el usuario en esta Revisión 5:

1. **Elegibilidad de credenciales — CERRADA.** Regla definitiva: ≥1 registro vigente `REQ_RECORDED` en
   `CAT_INSTITUTION_WORKED` o `CAT_LINK_PERIOD`, consultada por `credential_eligible(rs, owner_id)`
   (`assigns \nothing`, §8-ter, §8 P-20). `request_recorded_count > 0` genérico **ya no aplica**. Regla
   del modelo académico EduChain, no legal. **Retirada de pendientes.**
2. **Tensión T-1 (categoría/institución en correcciones) — CERRADA.** Misma categoría; mismo
   `institution_id` si la categoría asocia institución, o ambos `0` si no; nunca se cambia la
   institución (§5.5 validación 4, §7.1). **Retirada de pendientes.**
3. **Tensión T-2 (cadena de `prev_id`) — CERRADA.** No se recorre la cadena; aciclicidad por
   transiciones de estado para las correcciones que llegan al historial oficial (§5.5). **Retirada de
   pendientes.**

**Coherencia del documento (Rev 5).** Tras los tres cierres, máquina de estados (§4.1, §5.1, §5.4),
flujo (§6), firmas (§7), propiedades ACSL (§8, §8-bis, §8-ter), contrato de `credential_issue` (§8-ter)
y plan de etapas (§11) quedan alineados entre sí, y **no quedan tensiones residuales**: las dos que la
Rev 4 listaba (T-1, T-2) se han resuelto con reglas únicas. Se afirma coherencia con honestidad: no
hay contradicción interna conocida ni grado de libertad de negocio sin fijar.

**Lo que queda (NO son decisiones de negocio, son trabajo de ingeniería de etapas posteriores).** La
implementación, los contratos ACSL definitivos, la re-ejecución de WP/Eva y las pruebas dinámicas son
Etapa 2+ y requieren aprobación expresa del usuario para empezar (§11). En particular: re-verificar WP
de `security.c` y `credentials.c` contra la línea base tras los cambios aditivos; y confirmar
dinámicamente los existenciales positivos que caen en la limitación (G). Nada de esto es una decisión
de negocio pendiente: son objetivos de verificación aspiracionales, no resultados obtenidos.

> **Eliminado desde Rev 4:** la antigua pregunta «¿ampliar `credential_issue` o aplicar la regla en una
> capa nueva?» (decidida en Rev 4: integración directa, §8-ter). **Eliminado en Rev 5:** el punto
> abierto «criterio de elegibilidad» y las tensiones T-1 y T-2. Ninguno reaparece como punto abierto.

## 15. Confirmación de cumplimiento READ-ONLY (entregable 5)

Esta Revisión 5 **no modificó ningún archivo de código, ACSL, CLI, tests, scripts ni línea base /
resultados de verificación**. El único archivo escrito es `docs/diseno_flujo_solicitudes.md`. En
concreto, NO se tocaron: ningún `.c` (`src/*.c`, `main.c`, `tests/*.c`, `ui/educhain_cli.c`,
`verification/eva_harness.c`), ningún `.h` (`include/*.h`, `verification/specifications.h`,
`verification/invariants.h`, `tests/test_framework.h`), los contratos ACSL embebidos, los scripts de
verificación (`verification/*.sh`), ni `verification/wp_results_baseline.txt` / los
`verification/wp_results_fase4_*.txt` / `eva_results*.txt` / la sección LÍNEA BASE. La verificación de
cierre de la Etapa 1 es `git status`: solo `docs/diseno_flujo_solicitudes.md` aparece como cambio del
trabajo (los cambios preexistentes bajo `.frama-c/wp/cache/` son caché de ejecuciones previas de WP,
ajenos a esta edición del documento).

---

## Apéndice — Estado de las decisiones del usuario (consolidado en §14)

Ver **§14**. Resumen tras la Revisión 5: **NO queda ninguna decisión de negocio pendiente.** Las tres
que estaban abiertas en la Rev 4 están **cerradas**: (1) **elegibilidad de credenciales** = ≥1
`REQ_RECORDED` vigente en `CAT_INSTITUTION_WORKED` o `CAT_LINK_PERIOD` (`credential_eligible`, §8-ter,
regla académica no legal; `request_recorded_count > 0` genérico ya no aplica); (2) **tensión T-1**
(categoría/institución en correcciones) cerrada (misma categoría, mismo `institution_id` o ambos `0`,
nunca se cambia institución; §5.5 validación 4); (3) **tensión T-2** (cadena de `prev_id`) cerrada (sin
recorrido de cadena; aciclicidad por transiciones de estado; §5.5). **La pregunta «ampliar
`credential_issue` vs capa nueva» ya no figura** (decidida en Rev 4: integración directa, §8-ter). Las
decisiones A–D del encargo original siguen **cerradas** desde la Revisión 3. Lo que resta es trabajo de
ingeniería de Etapas 2+ (implementación, ACSL, re-ejecución WP/Eva, tests), no decisiones de negocio.

---

## Apéndice — Respuestas directas a las preguntas clave

- **¿Módulo nuevo o absorber experience?** → **Módulo nuevo `requests.c`** (Decisión 1).
- **¿Mantener o migrar roles?** → **Migración disciplinada** (Decisión 2): 4 actores, enums
  conservados, HR/COORD sin acceso a las OPs nuevas.
- **¿Hash real o referencia opaca?** → **Referencia opaca** (Decisión 3), declarada NO-hash y SIN
  garantía de integridad (§4.4).
- **¿Struct genérico o por categoría?** → **Genérico + enum + 4 campos numéricos reinterpretados**
  (Decisión 4, §4.2): soporta horas asignadas/realizadas, cursos y períodos sin inflar el struct.
- **¿`caller_id` = `institution_id`?** → **No** (ajuste 2, §3.2): `caller_id` es el ejecutor;
  `institution_id` la representada; la autorización se modela en `auth[]` y se eleva a P-13.
- **¿De dónde lee `credentials` la trayectoria y con qué criterio?** → **Solo de `REQ_RECORDED`
  vigente** (excluye `REQ_SUPERSEDED` y todo lo demás; ajuste C, §8-ter) vía `credential_eligible(rs,
  owner_id)`, `assigns \nothing`. **Criterio DEFINITIVO (Rev 5):** elegible si tiene ≥1 `REQ_RECORDED`
  vigente en `CAT_INSTITUTION_WORKED` o `CAT_LINK_PERIOD`; `request_recorded_count > 0` genérico ya no
  aplica; regla académica EduChain, no legal. **Integración directa decidida:** `credential_issue`
  recibe `const requests_state_t *rs` y consulta `credential_eligible` (reabre y re-verifica WP de
  `credentials.c`); ya no hay pregunta de capa nueva ni criterio pendiente.
- **¿Cómo se consulta sin filtrar información ajena?** → **Primitivas internas** sin rol
  (`request_find`/`request_status`/…) + **capa pública** `request_*_for` con rol + identidad + ámbito
  (P-SCOPE, §3.3 bis): PROFESSOR solo lo propio, INSTITUTION lo que la involucra, VERIFIER lo en
  trámite, ADMIN todo. Fuera de ámbito ⇒ `ERR_PERM`.
- **¿`ERR_NULL` en `authz_grant`/`authz_revoke`?** → **No alcanzable**: `requires \valid(st)&&\valid(rs)`
  lo descartan (patrón `credential_issue`); el `assigns` se parte por `behavior` y nunca nombra
  `auth[auth_count]` fuera de rango (§3.2 bis).
- **¿Cómo se corrige el historial?** → Función específica `request_submit_correction` con `prev_id`
  (no se añade `prev_id` a `request_submit`, Rev 4 §5.5); `prev_id` se valida al presentar y **se
  revalida al incorporar**; al incorporar, el anterior pasa a `REQ_SUPERSEDED` en la **misma operación
  atómica** (§5.5). Solo cambia su `state`. Dos correcciones sobre el mismo registro: gana la primera;
  la segunda falla con `ERR_BAD_TRANSITION`. **T-1 (Rev 5):** la corrección conserva la categoría del
  original y su `institution_id` (o ambos `0`); nunca cambia la institución. **T-2 (Rev 5):**
  aciclicidad por transiciones de estado, no por ids ni recorriendo la cadena.
- **¿ADMIN gestiona `auth[]`?** → **Sí, en runtime** (ajuste B, §3.2 bis): `authz_grant`/`authz_revoke`
  ADMIN-exclusivas y auditadas; revocar no borra ni es retroactivo.
- **¿Estado separado o global?** → **`requests_state_t` separado** (Decisión 5, §4.5/§10): protege el
  framing de WP de los módulos ya verificados; el global ~16.5 KB los degradaría.
- **¿Cómo se separa el historial nuevo del heredado?** → **P-SEP** (§8-bis): aislamiento de estado
  (las funciones viejas ni reciben `requests_state_t`) + ausencia de ruta de escritura (solo ADMIN vía
  `request_record`). Demostrable por `assigns`.
- **¿Qué de la verificación se conserva y qué se rehace?** → Se conserva todo
  `experience/professionals/audit`; se **reprueba `security.c`** (crece el `switch`) y `credentials.c`
  (ajuste 3, aditivo/solo lectura); se construye de cero el WP de `requests.c`. Línea base intacta.

---

### Nota de procedencia y cumplimiento READ-ONLY

Cifras de verificación tomadas de `verification/wp_results_baseline.txt` y de `docs/explicacionv1.md`
§7 (que cita `verification/wp_results_fase4_*.txt` y `eva_results_fase4.txt`). Patrones de código
citados desde `src/security.c` (switch de `security_can`), `src/experience.c`
(`experience_validate`/`experience_register`/`experience_validated_count`), `src/credentials.c`
(`cred_transition` con su `assigns` por `behavior`, `credential_issue` con su firma actual sin
`requests_state_t`, `cred_find`/`cred_find_by_owner`), `src/audit.c` (`audit_record_event`),
`include/common.h` (enums `role_t`/`op_code_t`, `educhain_state_t`, `ERR_*`),
`verification/specifications.h` (predicado `role_can` con `r == ROLE_VERIFIER ==> \false`) y
`ui/educhain_cli.c`. Para la Revisión 4 se releyeron además `credential_issue` con sus `behavior`s
`space`/`no_space` (modelo del `assigns` partido de `authz_grant`), `cred_transition` con
`!role_can(...) ==> (ERR_PERM || ERR_BAD_ID)`, `experience_validate`/`experience_validated_count`, y
`include/common.h` (`op_code_t` termina en `OP_REVOKE_CRED=8`; `ERR_*` disponibles). Para la Revisión
5 se releyeron `src/experience.c` (`experience_validated_count`: búsqueda lineal con `loop invariant
0<=count<=i`, `loop assigns i, count`, `loop variant`, `assigns \nothing`, sin aritmética de fechas)
como patrón gemelo de `credential_eligible`, `src/credentials.c` (`credential_issue` con su firma
actual `credential_issue(educhain_state_t*, role_t, int, int, int)` sin `requests_state_t`, y su
`assigns { st->creds[k].state | ... }, st->log[...], st->log_count`) para fundamentar la ampliación
aditiva de solo lectura, y la máquina de estados de §4.1/§5.4/§5.5 para los cierres T-1 y T-2. Este
documento **no implementa nada** y es el único archivo escrito en esta etapa (Revisión 5). No se
modificó ningún `.c`/`.h`/contrato ACSL/CLI/tests/scripts/línea base ni resultados de WP·Eva.
