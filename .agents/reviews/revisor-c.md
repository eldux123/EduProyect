# Revisión de C (re-revisión, 2.ª ronda) — EduChain · agente `revisor-c`

**Rol:** revisor senior de C orientado a verificación formal con Frama-C. Solo reviso y reporto; no modifico archivos del proyecto.

**Alcance:** todo el código C del repo `EduProyect`:
- Headers: `include/common.h`, `include/security.h`, `include/professionals.h`, `include/experience.h`, `include/credentials.h`, `include/audit.h`.
- Implementación: `src/security.c`, `src/professionals.c`, `src/experience.c`, `src/credentials.c`, `src/audit.c`.
- Demo: `main.c`.

**Nota metodológica:** no afirmo que nada esté "verificado" ni "probado formalmente"; eso lo decide Frama-C (WP/Eva), no esta revisión. No se ejecutó WP ni Eva. La única escritura es este informe.

---

## Aviso importante sobre el estado del código (discrepancia con el encargo)

El encargo dice que *"el código C (`include/`, `src/`, `main.c`) NO ha cambiado desde el informe anterior"*. **Al leer el código con ojos frescos, esto no se sostiene: el código SÍ cambió respecto al informe anterior.** Tres de los cinco puntos a re-verificar describen un código que ya no existe en `src/`:

- **I-1 (`experience_total_days` resta `YYYYMMDD`)**: esa función **ya no está** en `src/experience.c`. En su lugar hay `experience_validated_count`, que **no hace aritmética de fechas**: solo cuenta experiencias en estado `EXP_VALIDATED` del titular (`src/experience.c:117-146`).
- **I-2 (`professional_register` audita con `actor_id = new_id`)**: ya **no ocurre**. La firma ahora recibe `int caller_id` y audita con él (`src/professionals.c:35`, `src/professionals.c:72`).
- **I-3 (transiciones/revocación auditan con `actor_id = 0`)**: ya **no ocurre**. Las funciones de credencial reciben `caller_id` y auditan con él (`src/credentials.c:117`, `src/credentials.c:166`).

Reporto lo que el código dice hoy, no lo que decía el informe previo. Más abajo confirmo qué hallazgos siguen vivos (I-4, I-5) y doy por **resueltos** en el código los otros tres, con la salvedad de que **la documentación quedó desincronizada** (ver nota al final).

---

## Resumen

La implementación sigue siendo limpia y favorable a la verificación: patrón consistente **validar todo antes de mutar nada** (NULL → rango de ID → permiso → duplicados → capacidad de arreglo → capacidad de log → recién entonces escribir). No hay memoria dinámica ni dependencias de tiempo/aleatoriedad en los módulos del dominio, ni accesos fuera de límites visibles. La trazabilidad de auditoría mejoró: ahora se registra `caller_id` como `actor_id` en las seis operaciones mutadoras.

| Severidad | Nº de hallazgos vivos |
|-----------|:---------------------:|
| Crítico    | 0 |
| Importante | 2 (I-4, I-5) |
| Menor      | 4 (M-1 … M-4) |

Hallazgos previos **resueltos en código**: antiguos I-1, I-2, I-3 (detalle en §"Hallazgos previos ya resueltos").

---

## Crítico

Ninguno. No hallé corrupción de estado, comportamiento indefinido, acceso fuera de límites ni uso de variables no inicializadas en la implementación. Confirmado de nuevo en esta ronda.

---

## Importante

### I-4 · `credential_status` (y `audit_verify_event`) multiplexan "estado/índice" y "no encontrado" en el mismo `int` — **sigue presente (sin cambios)**
`include/credentials.h:35` + `src/credentials.c:9-21`; análogo en `audit_verify_event` (`src/audit.c:38-56`) y `experience_find`/`professional_find`/`cred_find`.

- **Qué está mal:** `credential_status` devuelve `(int)st->creds[i].state` (0..3) o `-1` si no existe. Como `CRED_PENDING == 0`, el valor legítimo `0` convive con `-1` como único centinela. El comentario del header dice "retorna el estado (`cred_state_t`)", cuando en realidad devuelve un `int` que **solo a veces** es un `cred_state_t`.
- **Por qué importa:** `credential_is_valid` (`src/credentials.c:23-26`) compara contra `CRED_ACTIVE`, así que para ese uso el patrón es seguro; pero cualquier otro llamador debe recordar discriminar `-1` antes de tratar el valor como estado. Para WP obliga a un `ensures` disyuntivo (`\result == -1 || 0 <= \result <= 3`) y es una fuente habitual de errores del llamador. El mismo patrón `-1` multiplexado aparece en los `*_find`, donde es idiomático (índice o `-1`) y menos problemático porque no se confunde con un dato de dominio.
- **Corrección sugerida:** aceptable si se documenta inequívocamente que `-1` nunca coincide con un estado (todos los estados son `>= 0`); alternativa más verificable: `int credential_status(const educhain_state_t *st, int cred_id, cred_state_t *out)` que retorne `EDU_OK`/`ERR_NOT_FOUND`/`ERR_NULL` y escriba el estado por puntero. Es decisión de diseño; la señalo para que se tome conscientemente, no la impongo.

### I-5 · `security_can` recibe `int op_code` y decide solo por rol (eje de identidad fuera) — **sigue presente (sin cambios)**
`include/security.h:28` + `src/security.c:6-27`.

- **Qué está mal:** el parámetro sigue siendo `int op_code` en vez de `op_code_t`, de modo que el dominio válido (1..8) no está en el tipo y el `default: return 0;` del `switch` (`src/security.c:25`) es la única barrera. Además `security_can` decide **solo por rol**: las reglas por identidad (PRO solo su propia experiencia en `src/experience.c:41`; no autovalidación en `src/experience.c:93`) viven en cada módulo. El comentario `security.h` ("punto unico de decision de autorizacion") sobre-promete respecto a lo que la firma hace.
- **Por qué importa:** con `int`, WP debe razonar sobre un entero arbitrario en vez de un `enum` acotado, y la "unicidad" del punto de decisión no se cumple literalmente en código (la autorización por identidad está repartida). No es un defecto de corrección —el `.c` aplica las reglas de identidad correctamente, verificado de nuevo— sino de contrato/modularidad para la verificación. El contrato ACSL del header ya documenta de forma parcial el comportamiento por `op_code` (P-1, issue/activate, VERIFIER), lo que mitiga pero no elimina la observación de tipado.
- **Corrección sugerida:** tipar el parámetro como `op_code_t`; y documentar explícitamente el reparto: `security_can` cubre autorización por rol, y la autorización por identidad (R-SELF / "propia") es responsabilidad declarada de cada módulo. Así el postcondicionado de `security_can` y de cada módulo queda limpio para WP.

---

## Menor

### M-1 · `op_code` como `int` en firmas y en `event_t` — **sigue presente**
`include/audit.h` (`int op_code` en `audit_record_event`), `src/audit.c:6-7`, `include/common.h` (`int op_code;` en `event_t`), y el parámetro `int op_code` de `cred_transition` (`src/credentials.c:92`).

- **Qué está mal / por qué:** mismo motivo que I-5 pero en auditoría, en el `struct` del log y en el helper de transición. Guardar el código como `int` pierde la relación con `op_code_t`; puede ser intencional (registrar códigos no reconocidos en auditoría forense), pero no está documentado.
- **Corrección sugerida:** tipar como `op_code_t` donde aplique, o añadir un comentario indicando que `event_t.op_code` admite cualquier `int` por diseño de auditoría.

### M-2 · Número mágico `2000000000` en el guardia de desbordamiento de `seq` — **sigue presente**
`src/audit.c:24` — `if (last >= 2000000000) { return ERR_FULL; }`.

- **Qué está mal / por qué:** el literal coincide con `ID_MAX` de `common.h` pero sin nombrarlo ni relacionarlo con `INT_MAX`. Es código esencialmente inalcanzable: `seq` crece de uno en uno y el log está acotado a `MAX_EVENTS == 1024`, así que `seq` nunca supera 1024. El guardia es defensivo pero el literal suelto reduce legibilidad.
- **Corrección sugerida:** definir una constante con nombre (p. ej. `SEQ_MAX`) en `common.h` y usarla, o comentar que es un margen bajo `INT_MAX` y que el guardia es formalmente inalcanzable dado `MAX_EVENTS`.

### M-3 · Retorno de `audit_record_event` descartado con `(void)` en todos los mutadores — **sigue presente**
`src/professionals.c:72`, `src/experience.c:72`, `src/experience.c:108`, `src/credentials.c:86`, `src/credentials.c:117`, `src/credentials.c:166`.

- **Qué está mal / por qué:** se ignora el código de retorno de la auditoría. Es **seguro hoy** porque cada mutador comprueba `st->log_count >= MAX_EVENTS` antes de mutar y el único otro fallo (desbordamiento de `seq`) es inalcanzable (ver M-2). Pero la seguridad depende de que esa pre-comprobación y la lógica interna de `audit_record_event` nunca se desincronicen; si alguien cambia una de las dos, el `(void)` oculta que un evento no se registró tras haber mutado el estado (quedaría estado sin su evento, rompiendo P-12).
- **Corrección sugerida:** comprobar el retorno (revertir/propagar) o dejar en cada sitio un comentario que documente por qué el `(void)` es seguro (capacidad de log ya reservada arriba). Varios sitios ya traen un comentario sobre el `actor_id`, pero no sobre por qué se descarta el retorno.

### M-4 · Asimetría de contratos ACSL en los headers — **sigue presente (matiz respecto al informe previo)**
`include/experience.h`, `include/credentials.h`, `include/professionals.h`.

- **Qué está mal / por qué:** siguen con contrato ACSL completo `experience_validate`, `experience_validated_count`, `professional_register`/`find`/`is_active`, `credential_is_valid` y las tres funciones de `audit.h`. Siguen **sin** contrato ACSL (solo prosa): `experience_register`, `credential_issue` y las transiciones `credential_activate/block/reactivate/revoke`/`credential_status`. No es un error de código, pero deja sin especificar funciones mutadoras que WP tendrá que verificar.
- **Corrección sugerida:** añadir en Fase 4 los contratos `requires/assigns/ensures` a las mutadoras que aún no los tienen, con el mismo estilo (atomicidad en error, cota de `count`). (Coincide con lo señalado por `revisor-acsl`.)

> Los antiguos M-5 (`cred_find`/`cred_find_by_owner` sin `NULL`) y M-6 (comentarios de permisos duplican la matriz) siguen **igual** en el código y mantienen el mismo veredicto del informe previo: ambos son **correctos hoy**; M-5 solo pide `requires \valid_read(st);` para WP, y M-6 solo pide indicar que la fuente de verdad es `security_can`. No los re-numero para no inflar; quedan confirmados sin cambios.

---

## Hallazgos previos ya resueltos en el código (confirmación explícita)

Estos tres hallazgos del informe anterior **ya no aplican al código actual**. Lo dejo escrito para que la trazabilidad entre rondas sea clara.

### (ex-I-1) `experience_total_days` — **ELIMINADA**
Ya no existe en `src/experience.c` ni en `include/experience.h`. La función presente es `experience_validated_count` (`src/experience.c:117-146`), que **no resta `YYYYMMDD`**: solo cuenta experiencias `EXP_VALIDATED` del `owner_id` y retorna `0..exps_count` (o `ERR_NULL`/`ERR_BAD_ID`/`ERR_NOT_FOUND`). Lleva contrato ACSL y bucle con invariantes (`0 <= count <= i`). El defecto semántico de "días" desapareció por construcción.

### (ex-I-2) `professional_register` audita con el actor real — **CORREGIDO**
La firma ahora es `professional_register(st, caller_role, int caller_id, int new_id)` (`src/professionals.c:35`), valida `caller_id` en rango (`src/professionals.c:43-46`) y audita `audit_record_event(st, caller_id, OP_REGISTER_PRO, new_id, EDU_OK)` (`src/professionals.c:72`). `actor_id` ya no es igual a `new_id`.

### (ex-I-3) Transiciones/revocación de credencial auditan con el actor real — **CORREGIDO**
`cred_transition` recibe `caller_id` y audita con él (`src/credentials.c:91-92`, `src/credentials.c:117`); `credential_revoke` igual (`src/credentials.c:166`); `credential_issue` audita con el `caller_id` del COORD, **no** con `owner_id` (`src/credentials.c:85-86`). Desapareció el `actor_id = 0` constante y la inconsistencia issue↔owner que señalaba el informe previo.

---

## Áreas revisadas sin problemas (declaración explícita, re-verificadas)

- **Seguridad de memoria:** toda escritura en `pros/exps/creds/log` usa el índice `count` solo tras comprobar `count < MAX_*` (`professionals.c:52-67`, `experience.c:59-70`, `credentials.c:71-83`, `audit.c:16-18`). Índices de `*_find`/`cred_find` en `0..count-1`. Sin accesos fuera de límites ni lectura de campos no inicializados.
- **Punteros / NULL:** todas las funciones públicas comprueban `st == 0` antes de desreferenciar; los helpers estáticos (`cred_find`, `cred_find_by_owner`) se llaman siempre tras esa comprobación.
- **Atomicidad en error (BR-10):** patrón "validar antes de mutar" aplicado de forma consistente; ningún camino de error escribe arreglos ni incrementa contadores. En éxito la mutación precede a la auditoría y la capacidad del log ya está reservada.
- **Códigos de retorno:** cada función comunica éxito/error con los códigos de `common.h`; los llamadores internos comprueban `security_can`, `professional_is_active`, `experience_find`, `cred_find`. Única excepción deliberada: el retorno de `audit_record_event` (ver M-3).
- **Límites de almacenamiento:** `professional_register`, `experience_register`, `credential_issue` rechazan con `ERR_FULL` a capacidad del arreglo o del log, antes de mutar; `audit_record_event` rechaza con `ERR_FULL` a capacidad.
- **Aritmética / desbordamiento:** `seq` crece de uno en uno con guardia inalcanzable; contadores `int` con tope `MAX_* <= 1024`. `experience_validated_count` incrementa un `int count` acotado por `i <= exps_count <= 256`, sin riesgo de desbordamiento y **sin** aritmética de fechas. No hay divisiones.
- **Fechas YYYYMMDD:** `date_in_range` (`experience.c:21-24`) acota a `[DATE_MIN, DATE_MAX]` y `experience_register` exige `start_date <= end_date`. La validación es de **rango sintáctico**, no de calendario (p. ej. `20210231` pasaría); es decisión consciente del modelo (coincide con lo señalado por `revisor-seguridad`). Ya no hay aritmética de "días" que magnifique ese matiz.
- **Determinismo:** los módulos del dominio no usan tiempo, aleatoriedad, E/S ni memoria dinámica. `main.c` usa `printf`/`memset` pero está declarado "demo NO verificada", fuera del alcance de WP/Eva.
- **Inmutabilidad de experiencia (BR-5):** `experience_validate` solo escribe `state` y `validator_id`; no toca `start_date`, `end_date` ni `owner_id`, y rechaza revalidar con `ERR_ALREADY_VALID` (`experience.c:99-102`). Coherente con el `assigns` del header.
- **No autovalidación (BR-2):** `experience_validate` rechaza `ERR_SELF_VALIDATE` cuando `owner_id == caller_id`, con independencia del rol (`experience.c:92-94`). Correcto.
- **Máquina de estados de credencial (BR-7):** `cred_transition` exige el estado `from` exacto antes de pasar a `to`; `credential_revoke` admite desde cualquier estado salvo `CRED_REVOKED` (terminal). Coincide con §5.1 de la tabla. Correcto.
- **Unicidad:** `professional_register`/`experience_register`/`credential_issue` rechazan `id` duplicado; `credential_issue` además rechaza una segunda credencial por titular (`cred_find_by_owner`). Correcto.
- **Orden de comprobaciones:** en los mutadores el orden es `NULL → rango ID → permiso → existencia/duplicado/estado → capacidad`. Es consistente entre módulos; `ERR_BAD_ID` precede a `ERR_PERM` (observación de orden ya anotada por `revisor-seguridad`, sin impacto en corrupción de estado).
- **Include guards y dependencias:** guards únicos, sin ciclos.

---

## Verificación realizada en esta ronda

- Lectura completa de los seis headers de `include/`, los cinco `.c` de `src/`, `main.c` y el informe previo de `revisor-c`.
- Cotejo línea a línea de los cinco puntos a re-verificar contra el código actual (no contra el informe previo).
- Confirmación con `grep` de cada `audit_record_event`, firma con `caller_id` y centinela `-1` para fijar archivo:línea.
- Trazado del patrón "validar antes de mutar" y de cada escritura a arreglo/contador en los cinco módulos.
- No se ejecutó WP ni Eva. No se modificó ningún archivo del proyecto; la única escritura es este informe.

---

## Nota final: documentación desincronizada (fuera del alcance de código, pero relevante)

Varios documentos siguen describiendo el código **antiguo**: `docs/architecture.md` y `docs/formal_verification.md` y `verification/wp_results.txt` aún mencionan `experience_total_days` y su `..._signed_overflow_4`. El código ya no la tiene. Esto no es un defecto del código C que reviso, pero la línea base de WP (`wp_results.txt`) y la arquitectura quedaron desfasadas respecto a `src/`: conviene re-generar WP y actualizar esos documentos para que reflejen `experience_validated_count` y las firmas con `caller_id`. Lo señalo como aviso de coherencia; no es mi hallazgo de código.
