# EduChain — Fase 1: Modelo, Actores, Reglas, Estructuras y Propiedades

**Proyecto académico ficticio.** EduChain simula la *lógica* de un contrato inteligente en lenguaje C
para registrar profesionales, validar experiencia y administrar credenciales digitales, con el fin de
demostrar propiedades de seguridad y consistencia mediante verificación formal (Frama-C / ACSL / WP / Eva).

> **Qué NO es este proyecto.** No es una blockchain real, no despliega contratos en una red, no usa
> criptografía avanzada ni datos reales de personas o instituciones. "Contrato inteligente" aquí es una
> metáfora de diseño: un conjunto de funciones deterministas con reglas de transición explícitas y un
> registro de eventos *append-only*. Todos los actores, identificadores y reglas son ficticios y se
> definen a continuación como parte del modelo académico.

Organización ficticia: **EduCorp**.

---

## 1. Alcance de la Fase 1

Esta fase **no escribe la implementación**. Define, para que las fases posteriores las verifiquen:

1. Los **actores** y la **matriz de permisos**.
2. Las **reglas del contrato** (invariantes de negocio).
3. Las **estructuras de datos** y **enumeraciones** (a nivel de modelo, aún sin `.c`).
4. Las **máquinas de estado** (credencial y experiencia).
5. El catálogo de **códigos de retorno / errores**.
6. Las **propiedades formales** candidatas a demostrarse con Frama-C.
7. El **modelo de amenazas** básico mapeado a contramedidas.
8. Los **supuestos y límites** del modelo.

Nada aquí depende de reglamentos reales. Las constantes (límites, longitudes) son decisiones de diseño
elegidas para favorecer la demostrabilidad.

---

## 2. Actores (roles)

El sistema reconoce cinco roles. Son etiquetas lógicas dentro del modelo; **no** hay autenticación real
(la autenticación la aportaría un sistema externo en una integración futura). Cada operación recibe el
rol del invocador como parámetro y lo valida.

| Rol (enum)       | Descripción ficticia                                           |
|------------------|----------------------------------------------------------------|
| `ROLE_ADMIN`     | Administrador del sistema. Gestiona cuentas y permisos.        |
| `ROLE_HR`        | Recursos Humanos de EduCorp. Valida vinculación/experiencia.  |
| `ROLE_COORD`     | Coordinador. Aprueba/rechaza actividades y emite credenciales.|
| `ROLE_PRO`       | Profesional registrado. Consulta y solicita; no se autovalida.|
| `ROLE_VERIFIER`  | Verificador externo. Solo lectura de lo autorizado.           |

**Principios de diseño aplicados:** mínimo privilegio y separación de responsabilidades. Quien registra
un dato no es quien lo valida; quien es titular de un dato no puede validarlo.

---

## 3. Matriz de permisos

Leyenda: ✓ permitido · ✗ prohibido · (titular) solo sobre registros propios y solo lectura.

| Operación                         | ADMIN | HR | COORD | PRO        | VERIFIER |
|-----------------------------------|:-----:|:--:|:-----:|:----------:|:--------:|
| register_professional             |   ✓   | ✓  |   ✗   |     ✗      |    ✗     |
| register_experience               |   ✗   | ✓  |   ✗   | ✓ (propia) |    ✗     |
| validate_experience               |   ✗   | ✓  |   ✓   |     ✗      |    ✗     |
| issue_credential                  |   ✗   | ✗  |   ✓   |     ✗      |    ✗     |
| activate_credential               |   ✗   | ✗  |   ✓   |     ✗      |    ✗     |
| block_credential                  |   ✓   | ✗  |   ✓   |     ✗      |    ✗     |
| revoke_credential                 |   ✓   | ✗  |   ✓   |     ✗      |    ✗     |
| get_history / verify_event        |   ✓   | ✓  |   ✓   | (titular)  |    ✓     |
| verify_credential_status (lectura)|   ✓   | ✓  |   ✓   | (titular)  |    ✓     |

**Regla transversal R-SELF:** en `validate_experience`, el validador (`caller_id`) **debe ser distinto**
del profesional titular de la experiencia (`owner_id`), con independencia del rol. Un `ROLE_HR` que a su
vez sea titular de esa experiencia no puede validarla.

---

## 4. Modelo de datos (nivel diseño)

Arreglos de tamaño fijo, sin memoria dinámica, para facilitar la verificación con WP/Eva.

### 4.1 Constantes (decisiones de diseño)

| Constante               | Valor | Razón                                             |
|-------------------------|------:|---------------------------------------------------|
| `MAX_PROFESSIONALS`     |   64  | Límite de registros; cota superior para WP.       |
| `MAX_EXPERIENCES`       |  256  | Experiencias totales en el sistema.               |
| `MAX_CREDENTIALS`       |   64  | Una credencial por profesional como máximo.       |
| `MAX_EVENTS`            | 1024  | Capacidad del log append-only.                    |
| `ID_MIN` / `ID_MAX`     | 1 / 2_000_000_000 | IDs válidos (>0, dentro de int32).    |
| `DATE_MIN` / `DATE_MAX` | 19000101 / 21001231 | Fecha como entero `YYYYMMDD`.       |

Fechas representadas como enteros `YYYYMMDD` (p. ej. `20240115`). Simplifica comparaciones y evita
tipos de tiempo del sistema; la validez de calendario (días por mes) se trata como decisión: la Fase 4
decidirá entre validación completa de calendario o solo monotonía `start <= end` dentro de rango.
**Decisión preliminar:** validar rango + `start <= end`; la validación de calendario completo se marca
como mejora opcional (Fase 7).

### 4.2 Estructuras (pseudo-C del modelo)

```c
typedef enum { ROLE_ADMIN, ROLE_HR, ROLE_COORD, ROLE_PRO, ROLE_VERIFIER } role_t;

typedef struct {
    int      id;          /* identificador único, ID_MIN..ID_MAX, 0 = slot libre */
    int      active;      /* 0/1 */
} professional_t;

typedef enum { EXP_PENDING, EXP_VALIDATED } exp_state_t;

typedef struct {
    int         id;           /* id de la experiencia, único */
    int         owner_id;     /* profesional titular */
    int         start_date;   /* YYYYMMDD */
    int         end_date;     /* YYYYMMDD, >= start_date */
    exp_state_t state;        /* PENDING -> VALIDATED (no revierte) */
    int         validator_id; /* 0 si no validada; != owner_id si validada */
} experience_t;

typedef enum { CRED_PENDING, CRED_ACTIVE, CRED_BLOCKED, CRED_REVOKED } cred_state_t;

typedef struct {
    int          id;          /* id de credencial, único */
    int          owner_id;    /* profesional titular */
    cred_state_t state;
} credential_t;

typedef struct {
    int  seq;         /* identificador secuencial del evento, estrictamente creciente */
    int  actor_id;    /* quién ejecutó */
    int  op_code;     /* operación */
    int  target_id;   /* registro afectado */
    int  result;      /* código de retorno resultante */
} event_t;

typedef struct {
    professional_t pros[MAX_PROFESSIONALS];  int pros_count;
    experience_t   exps[MAX_EXPERIENCES];    int exps_count;
    credential_t   creds[MAX_CREDENTIALS];   int creds_count;
    event_t        log[MAX_EVENTS];          int log_count;
} educhain_state_t;
```

> Las estructuras **no almacenan datos personales** (nombres, documentos, fotos, salarios). Solo IDs
> numéricos y estados. En una integración real los datos sensibles vivirían fuera del núcleo; aquí ni
> siquiera existen, por ser un modelo ficticio.

---

## 5. Máquinas de estado

### 5.1 Credencial

```
         issue            activate
   (∅) ----------> PENDING ----------> ACTIVE
                      |                  |  ^
                      |                  |  | reactivate (BLOCKED -> ACTIVE)
                      | revoke     block |  |
                      |                  v  |
                      |               BLOCKED
                      |                  |
                      |  revoke          | revoke
                      v                  v
                   REVOKED <-------------+
                   (terminal)

   revoke permitido desde PENDING, ACTIVE o BLOCKED  ->  REVOKED (terminal)
   reactivate permitido solo desde BLOCKED -> ACTIVE (ADMIN/COORD)
```

Transiciones **permitidas** (todo lo demás se rechaza con `ERR_BAD_TRANSITION`):

| Desde \ Hacia | PENDING | ACTIVE | BLOCKED | REVOKED |
|---------------|:-------:|:------:|:-------:|:-------:|
| **PENDING**   |    —    |   ✓    |    ✗    |    ✓    |
| **ACTIVE**    |    ✗    |   —    |    ✓    |    ✓    |
| **BLOCKED**   |    ✗    |   ✓*   |    —    |    ✓    |
| **REVOKED**   |    ✗    |   ✗    |    ✗    |    —    |

`ACTIVE*` desde `BLOCKED` = reactivación (solo ADMIN/COORD). `REVOKED` es **terminal**: ninguna
operación ordinaria lo saca de ese estado. Una credencial **válida para uso** es únicamente la que está
en `ACTIVE`.

### 5.2 Experiencia

```
   register ---> PENDING ---validate---> VALIDATED (terminal vía operaciones ordinarias)
```

`VALIDATED` no se revierte ni se edita con funciones ordinarias (regla de inmutabilidad de registros
aprobados). Una corrección exigiría una operación distinta y auditada (fuera del alcance mínimo; se deja
como mejora en Fase 7).

---

## 6. Códigos de retorno (contrato de errores)

Todas las funciones devuelven `int`. `OK = 0`; errores negativos. Un error **nunca** corrompe el estado:
o la operación se aplica por completo o no modifica nada (atomicidad a nivel de función).

| Código                | Valor | Significado                                            |
|-----------------------|------:|--------------------------------------------------------|
| `EDU_OK`              |    0  | Operación exitosa.                                     |
| `ERR_NULL`            |   -1  | Puntero nulo donde no se permite.                      |
| `ERR_PERM`            |   -2  | Rol sin permiso para la operación.                     |
| `ERR_SELF_VALIDATE`   |   -3  | Intento de validar la propia experiencia.              |
| `ERR_DUPLICATE`       |   -4  | Identificador duplicado.                               |
| `ERR_NOT_FOUND`       |   -5  | Registro inexistente.                                  |
| `ERR_BAD_DATE`        |   -6  | Fecha fuera de rango o `start > end`.                  |
| `ERR_BAD_ID`          |   -7  | ID fuera de `ID_MIN..ID_MAX`.                          |
| `ERR_FULL`            |   -8  | Capacidad máxima alcanzada.                            |
| `ERR_BAD_TRANSITION`  |   -9  | Transición de estado no permitida.                     |
| `ERR_IMMUTABLE`       |  -10  | Intento de modificar un registro validado.             |
| `ERR_ALREADY_VALID`   |  -11  | La experiencia ya estaba validada.                     |

---

## 7. Reglas del contrato (invariantes de negocio)

Numeradas para trazarlas hasta las propiedades formales (§8) y las pruebas (Fase 6).

- **BR-1 (autorización).** Ninguna operación de escritura se ejecuta si el rol del invocador no está
  autorizado en la matriz (§3). → `ERR_PERM`.
- **BR-2 (no autovalidación).** `validate_experience` exige `caller_id != owner_id`. → `ERR_SELF_VALIDATE`.
- **BR-3 (unicidad).** No existen dos profesionales, experiencias o credenciales con el mismo `id`.
- **BR-4 (fechas válidas).** Toda experiencia cumple `DATE_MIN <= start <= end <= DATE_MAX`.
- **BR-5 (inmutabilidad).** Una experiencia en `VALIDATED` no cambia por operaciones ordinarias.
- **BR-6 (uso de credencial).** Solo `ACTIVE` es válida; `BLOCKED` y `REVOKED` no se consideran válidas.
- **BR-7 (transiciones).** Solo se permiten las transiciones de la tabla §5.1; `REVOKED` es terminal.
- **BR-8 (capacidad).** Ningún contador supera su `MAX_*`; no se escribe fuera de los arreglos.
- **BR-9 (validación de parámetros).** IDs y fechas se validan antes de usarse; punteros no nulos.
- **BR-10 (atomicidad en error).** Si una operación retorna error, el estado queda idéntico al previo.
- **BR-11 (auditoría append-only).** El log solo crece; `seq` es estrictamente creciente; no se reescribe
  ni se borra con funciones ordinarias; al llenarse retorna `ERR_FULL` sin sobrescribir.

---

## 8. Propiedades formales candidatas (a demostrar con Frama-C)

Se expresan en lenguaje natural ahora; en Fase 4-5 se traducen a ACSL (`requires`/`ensures`/`assigns`
+ invariantes de lazo). Clasificación objetivo: **WP** para contratos funcionales, **Eva** para
seguridad de memoria/aritmética.

| ID    | Propiedad                                                                 | Regla | Herramienta |
|-------|---------------------------------------------------------------------------|-------|-------------|
| P-1   | Si `role` no autorizado ⇒ retorno `ERR_PERM` y estado sin cambios.         | BR-1,10 | WP        |
| P-2   | `validate_experience` con `caller==owner` ⇒ `ERR_SELF_VALIDATE`, sin cambio.| BR-2  | WP        |
| P-3   | Tras registrar, no hay dos IDs iguales (invariante de unicidad preservado).| BR-3  | WP        |
| P-4   | Una experiencia registrada satisface `start<=end` dentro de rango.         | BR-4  | WP        |
| P-5   | Una experiencia `VALIDATED` no cambia `start/end/owner` tras cualquier op.  | BR-5  | WP        |
| P-6   | `verify_credential_status` retorna "válida" ⇔ estado `ACTIVE`.             | BR-6  | WP        |
| P-7   | Toda transición aplicada pertenece a la tabla permitida.                   | BR-7  | WP        |
| P-8   | `REVOKED` es terminal: no existe op ordinaria que lo abandone.             | BR-7  | WP        |
| P-9   | Todos los accesos a arreglos están dentro de `0..count-1 < MAX`.           | BR-8  | WP + Eva  |
| P-10  | No hay desbordamiento en incrementos de contador ni en `seq`.              | BR-8,11| WP + Eva |
| P-11  | `assigns` vacío en funciones de solo lectura (no mutan estado).            | BR-9  | WP        |
| P-12  | El log es monótono: `log_count` solo crece; `seq` estrictamente creciente. | BR-11 | WP        |
| P-13  | Ausencia de UB: punteros válidos, sin lectura de no inicializados.         | BR-9  | Eva       |

> **Distinción honesta (se mantendrá en todo el proyecto):** WP *demuestra* obligaciones de prueba
> deductivas; Eva es análisis estático de valores (ausencia de ciertos errores en las trazas
> analizadas); las pruebas unitarias y los sanitizadores (ASan/UBSan) solo *observan* comportamiento en
> las entradas ejecutadas. No se afirmará que algo está "verificado" sin la salida de Frama-C que lo
> respalde.

---

## 9. Modelo de amenazas (resumen Fase 1 → se amplía en Fase 3)

| Amenaza                         | Escenario                                   | Contramedida (regla) | Prueba          |
|---------------------------------|---------------------------------------------|----------------------|-----------------|
| Escalada de privilegios         | ROLE_PRO llama `validate_experience`        | BR-1 (P-1)           | test negativo   |
| Autovalidación                  | HR valida su propia experiencia             | BR-2 (P-2)           | test negativo   |
| Duplicación de registros        | Reusar un `id` ya existente                 | BR-3 (P-3)           | test negativo   |
| Fechas inválidas                | `start > end` / fuera de rango              | BR-4 (P-4)           | test negativo   |
| Alteración de historial         | Editar experiencia validada / reescribir log| BR-5, BR-11 (P-5,P-12)| test negativo  |
| Uso de credencial revocada      | Operar con credencial `REVOKED`             | BR-6, BR-7 (P-6,P-8) | test negativo   |
| Transición ilegal               | `REVOKED -> ACTIVE`                         | BR-7 (P-7,P-8)       | test negativo   |
| Desbordamiento de arreglo       | Insertar con arreglo lleno                  | BR-8 (P-9)           | test + Eva/WP   |
| Desbordamiento aritmético       | `seq`/contador cerca de INT_MAX             | BR-8,11 (P-10)       | test + Eva/WP   |
| Puntero nulo / entradas malas   | Pasar `NULL` o IDs inválidos                | BR-9 (P-11,P-13)     | test + Eva      |

---

## 10. Supuestos, límites y garantías

**Supuestos del modelo:**
- La autenticación es externa; el `caller_role`/`caller_id` llega como dato confiable al núcleo. El
  núcleo verifica *autorización*, no *autenticación*.
- Ejecución mononúcleo, sin concurrencia dentro del núcleo (las "solicitudes concurrentes" del enunciado
  original quedan como responsabilidad de la capa externa; el núcleo es determinista y reentrante solo si
  se serializa el acceso al `educhain_state_t`).
- Fechas como enteros `YYYYMMDD`; no se usa el reloj del sistema.

**Lo que el núcleo garantiza (cuando WP/Eva lo demuestren):** autorización, no autovalidación, unicidad,
validez de fechas en rango, inmutabilidad de lo validado, transiciones legales, ausencia de accesos fuera
de límites y de ciertos desbordamientos, atomicidad en error.

**Lo que el núcleo NO garantiza:** veracidad del mundo real de los datos (un hash o un ID no prueban que
algo ocurrió), persistencia, cifrado, control de concurrencia, ni seguridad de la capa de aplicación.

**Límites explícitos de trazabilidad y enforcement (identificados en la auditoría de seguridad, Fase 4):**
- **Auditoría solo de operaciones exitosas (S-12).** `audit_record_event` se invoca únicamente en la
  rama de éxito. Los intentos **rechazados** (`ERR_PERM`, `ERR_SELF_VALIDATE`, `ERR_BAD_TRANSITION`,
  etc.) **no dejan rastro** en el log. El campo `event_t.result` existe pero hoy siempre vale `EDU_OK`.
  Decisión actual: documentarlo como límite; auditar intentos fallidos queda como mejora (requiere
  cambio funcional, respetando capacidad del log). Un auditor que quiera ver intentos de violación
  debe asumir que el núcleo no los registra todavía.
- **R-6 sin punto de enforcement interno (S-6).** El núcleo expone `credential_is_valid`/
  `credential_status` (solo lectura) pero **no tiene ninguna operación que "consuma" una credencial**.
  La regla "no usar credenciales BLOCKED/REVOKED" se cumple como *consulta correcta*; su *aplicación*
  (no actuar si no está ACTIVE) depende de que la capa externa llame a `credential_is_valid` antes de
  operar. Es un control delegado, no interno.
- **Orden de códigos de error (S-9).** La verificación de permiso (`ERR_PERM`) ocurre **antes** que la
  de existencia/estado (`ERR_NOT_FOUND`/`ERR_BAD_TRANSITION`), por lo que un rol **no autorizado** no
  obtiene un oráculo de existencia (siempre recibe `ERR_PERM`). Para roles **autorizados**, los códigos
  distintos de `ERR_PERM` revelan estado interno; es aceptable (ya pueden leer estado) pero la capa
  externa no debe reenviar esos códigos a `ROLE_VERIFIER`/terceros.
- **Append-only como convención de API (S-11).** `audit_record_event` es la única vía de escritura del
  log y es append-only con `seq` creciente, pero al ser `educhain_state_t` una struct pública, nada
  impide a código externo escribir `st->log[]` directamente. La garantía append-only es de API, no
  estructural.

---

## 11. Entorno de verificación (para fases posteriores)

- Target indicado: **WSL2 Debian**. En esta máquina hay **WSL2 Ubuntu** disponible (no Debian). Ubuntu
  usa el mismo `apt` y el mismo paquete `frama-c`, por lo que sirve; se confirmará la instalación real al
  llegar a la **Fase 5**, no antes.
- Comandos de instalación previstos (se ejecutarán en Fase 5, no ahora):
  ```bash
  sudo apt update
  sudo apt install -y frama-c why3 z3        # WP + Eva + SMT
  frama-c -wp -wp-prover z3 <archivos.c>     # verificación deductiva
  frama-c -eva <archivos.c>                  # análisis de valores
  ```
- **No se ha ejecutado Frama-C todavía.** No hay resultados de verificación que reportar en esta fase.

---

## 12. Cierre de Fase 1 (checklist del enunciado)

| Punto pedido en Fase 1        | Estado |
|-------------------------------|--------|
| Reglas del sistema            | ✓ §7   |
| Actores                       | ✓ §2–3 |
| Estructuras                   | ✓ §4   |
| Estados (máquinas)            | ✓ §5   |
| Propiedades a demostrar       | ✓ §8   |
| Sin escribir toda la implementación | ✓ (solo modelo/pseudo-C) |

### Reporte de fase
- **Qué se desarrolló:** modelo completo (actores, permisos, datos, estados, errores, reglas,
  propiedades, amenazas, supuestos).
- **Decisiones tomadas:** arreglos fijos sin memoria dinámica; fechas `YYYYMMDD`; `REVOKED` terminal;
  `VALIDATED` inmutable; validación de rango de fechas en vez de calendario completo (calendario = mejora);
  núcleo sin concurrencia.
- **Problemas / abiertos → RESUELTOS (confirmados al pasar a Fase 2):**
  1. Validación de fechas = **solo rango + `start <= end`** (calendario completo = mejora en Fase 7).
  2. Reactivación `BLOCKED -> ACTIVE` = **permitida, solo ADMIN/COORD**.
  3. Credenciales por profesional = **una sola** (unicidad por `owner_id`).
- **Qué falta:** nada para Fase 2; estas decisiones quedan fijadas.
- **Resultados obtenidos:** ninguno de verificación aún (correcto: no se ha corrido Frama-C).
- **Para continuar:** validar este documento y resolver los 3 puntos abiertos.
