# Auditoría de seguridad — EduChain (agente `revisor-seguridad`) — 2ª ronda

**Rol:** especialista en seguridad / análisis de vulnerabilidades. Pienso como atacante,
busco caminos que violen los invariantes. **Solo reviso y reporto; no modifico código.**
No afirmo que el sistema sea "seguro por completo": reporto el estado real de cada control.

**Alcance auditado:** `src/security.c`, `src/credentials.c`, `src/experience.c`,
`src/professionals.c`, `src/audit.c`, `main.c`, headers en `include/`, pruebas
adversariales `tests/test_attacks.c`. Referencia de diseño: `docs/phase1_model.md`.

**Entregable asociado:** "Vulnerabilidades identificadas y controles implementados"
(prompt maestro). Cada hallazgo se enlaza a las reglas BR-* / propiedades P-* del modelo.

**Convención de evidencia:** `archivo:línea` sobre el código leído en esta auditoría.

**Contexto de esta ronda.** Desde la revisión anterior se corrigió `docs/phase1_model.md §5.1`:
el diagrama ASCII de la máquina de estados de credenciales ahora muestra explícitamente la
transición `PENDING → REVOKED` y deja de insinuar que sea ilegal. El código `.c` NO cambió desde
la última revisión. Las pruebas adversariales `tests/test_attacks.c` sí crecieron (casos A13–A18)
y cierran la mayoría de las brechas que listé antes. Esta ronda verifica la coherencia
diagrama ↔ tabla ↔ código y reevalúa el estado de cada regla, en particular R-7.

---

## Verificación de coherencia de la máquina de estados (foco de esta ronda)

**Resultado: diagrama, tabla y código ahora coinciden. No queda ninguna otra inconsistencia
en la máquina de estados de credenciales.**

| Transición        | Diagrama §5.1 | Tabla §5.1 | Código (`credentials.c`)                      | ¿Coherente? |
|-------------------|:-------------:|:----------:|-----------------------------------------------|:-----------:|
| PENDING → ACTIVE  | `activate` ✓  | ✓          | `credential_activate` (`:129-132`)            | sí          |
| PENDING → REVOKED | `revoke` ✓ (ahora dibujada) | ✓ | `credential_revoke` permite PENDING (`:141-165`) | **sí (corregido)** |
| ACTIVE → BLOCKED  | `block` ✓     | ✓          | `credential_block` (`:134-137`)               | sí          |
| ACTIVE → REVOKED  | `revoke` ✓    | ✓          | `credential_revoke` (`:141-165`)              | sí          |
| BLOCKED → ACTIVE  | `reactivate` ✓| ✓*         | `credential_reactivate` (`:139-142`)          | sí          |
| BLOCKED → REVOKED | `revoke` ✓    | ✓          | `credential_revoke` (`:141-165`)              | sí          |
| REVOKED → *       | terminal      | todo ✗     | `revoke` sobre REVOKED → `ERR_BAD_TRANSITION` (`:157-159`); ningún `from == REVOKED` en el resto | sí          |
| PENDING → BLOCKED | no dibujada (implícita ilegal) | ✗ | `block` exige `from == ACTIVE` → `ERR_BAD_TRANSITION` | sí |

- El diagrama ahora incluye la línea explícita "revoke permitido desde PENDING, ACTIVE o
  BLOCKED → REVOKED (terminal)" y traza la arista de revoke desde PENDING, en línea con la tabla
  (celda PENDING/REVOKED = ✓) y con `credential_revoke`, que solo excluye el origen `REVOKED`
  (`src/credentials.c:157-159`) y cuyo comentario confirma "se permite desde PENDING, ACTIVE o
  BLOCKED" (`src/credentials.c:141-142`).
- La máquina de experiencia (§5.2) sigue siendo coherente: `PENDING → VALIDATED` terminal por
  operaciones ordinarias, implementada en `experience_validate` con rechazo `ERR_ALREADY_VALID`
  (`src/experience.c:97-100`). Sin cambios ni inconsistencias.
- **No detecté ninguna otra divergencia** entre diseño, tabla y código en ninguna de las dos
  máquinas de estado.

---

## Resumen ejecutivo

| Regla | Tema | Estado (2ª ronda) | Cambio vs ronda 1 |
|------:|------|-------------------|-------------------|
| R-1 | Autorización para validar experiencias | **Protegida** | = |
| R-2 | No autovalidación | **Protegida** | = |
| R-3 | IDs duplicados | **Protegida** | = (brecha de prueba cerrada) |
| R-4 | Fechas inválidas / invertidas | **Parcial** | = |
| R-5 | Inmutabilidad de lo validado | **Parcial** | = |
| R-6 | Uso de credencial BLOCKED/REVOKED | **Protegida** | = (brecha de prueba cerrada) |
| R-7 | Máquina de estados de credenciales | **Protegida** | **Parcial → Protegida** |
| R-8 | Límites de almacenamiento | **Protegida** | = (brecha de prueba cerrada) |
| R-9 | Validación de parámetros | **Parcial** | = |
| R-10 | Errores con estado consistente | **Protegida** | = |

Ninguna regla aparece como **Vulnerable** (ruta de ataque directa sin control). **No afirmo que
el sistema sea seguro por completo**; las marcas **Parcial** señalan controles que funcionan en el
camino implementado pero dependen de un supuesto no verificado en código, o que dejan sin cubrir
una variante del ataque.

**Cambio principal de esta ronda: R-7 pasa de Parcial a Protegida** (justificación abajo).

---

## R-1 — Ningún usuario sin permisos puede validar experiencias → **Protegida**

**Escenario de ataque.** Un `ROLE_PRO` o `ROLE_VERIFIER` llama `experience_validate` para
marcar como validada una experiencia ajena y así "aprobarse" trayectoria.

**Control presente.** Decisión de autorización centralizada en `security_can`
(`src/security.c:7`). `experience_validate` consulta `security_can(caller_role, OP_VALIDATE_EXP)`
antes de mutar (`src/experience.c:86`) y retorna `ERR_PERM` si falla. `OP_VALIDATE_EXP` solo
acepta `ROLE_HR` o `ROLE_COORD` (`src/security.c:15-16`); `ADMIN`, `PRO` y `VERIFIER` quedan
fuera. La comprobación ocurre antes de cualquier mutación → atomicidad (BR-1, BR-10, P-1).

**Cobertura de prueba.** A1 verifica `ROLE_PRO` → `ERR_PERM` (`tests/test_attacks.c:24`).

**Qué falta.** Nada crítico. No hay caso negativo explícito para `ROLE_VERIFIER`/`ROLE_ADMIN`
sobre `validate_experience`; cubiertos por la lógica de `security_can`, pero un caso explícito
cerraría la matriz por completo.

---

## R-2 — Un profesional no puede validar su propia experiencia → **Protegida**

**Escenario de ataque.** Alguien con `ROLE_HR`/`ROLE_COORD` que además sea titular
(`owner_id == caller_id`) valida su propia experiencia, sorteando la separación de
responsabilidades (regla transversal R-SELF, §3).

**Control presente.** `experience_validate` compara `st->exps[idx].owner_id == caller_id` y
retorna `ERR_SELF_VALIDATE` **con independencia del rol** (`src/experience.c:93-95`), después de
localizar la experiencia y antes de mutar estado/validator. El control vive en la operación que
conoce al titular, no en `security_can` (ciego al `owner_id`). Correcto (BR-2, P-2).

**Cobertura de prueba.** A2: HR titular → `ERR_SELF_VALIDATE` (`tests/test_attacks.c:28`).

**Qué falta.** Nada. Nota de diseño: cualquier ruta de validación nueva debe replicar este
chequeo, porque el punto único de autorización no lo garantiza.

---

## R-3 — No se pueden registrar identificadores duplicados → **Protegida**

**Escenario de ataque.** Reusar un `id` ya existente (profesional, experiencia o credencial)
para colisionar un registro y confundir búsquedas posteriores.

**Control presente.**
- Profesionales: `professional_find` previo a insertar → `ERR_DUPLICATE` (`src/professionals.c:50-52`).
- Experiencias: `experience_find` → `ERR_DUPLICATE` (`src/experience.c:57-59`).
- Credenciales: doble unicidad — por `cred_id` (`cred_find`) y una credencial por titular vía
  `cred_find_by_owner` (`src/credentials.c:67-73`), coherente con "una credencial por
  profesional" (§4.1). (BR-3, P-3).

**Cobertura de prueba (ampliada).** A4: duplicado de profesional **y** de `exp_id`
(`tests/test_attacks.c:43-44`). A16 (nuevo): `cred_id` duplicado y segunda credencial por
`owner_id` → `ERR_DUPLICATE` (`tests/test_attacks.c:122-131`). **La brecha de prueba que señalé
en la ronda 1 queda cerrada.**

**Qué falta.** Nada pendiente; control y pruebas alineados.

---

## R-4 — No se aceptan fechas inválidas (incluye invertidas) → **Parcial**

**Escenario de ataque.** (a) Registrar experiencia con `end_date < start_date`; (b) fechas fuera
de rango; (c) fecha sintácticamente en rango pero inexistente en calendario, p. ej. `20210231`
(31 de febrero) o `20210000` (mes 00).

**Control presente.** `experience_register` valida rango con `date_in_range` sobre
`DATE_MIN..DATE_MAX` y `start_date > end_date → ERR_BAD_DATE` (`src/experience.c:47-53`, helper
en `:21-24`). Cubre (a) y (b) (BR-4, P-4).

**Qué falta / escenario no cubierto.** El caso (c) NO se controla: cualquier entero `YYYYMMDD`
entre `19000101` y `21001231` se acepta aunque el mes sea `00`/`13` o el día no exista. Es una
decisión consciente y documentada del modelo (validación de rango en vez de calendario completo;
calendario = mejora Fase 7, §4.1 y §10). Lo marco **Parcial** porque, como atacante, `20210231`
pasa el control.

**Corrección sugerida.** Si se requiere robustez de calendario, añadir validación de mes `01..12`
y día `01..días_del_mes(mes,año)` en un helper `date_is_calendar_valid`. Mientras sea decisión de
diseño, mantener documentado que la validez es de rango, no de calendario (ya en §4.1).

**Cobertura de prueba.** A5: fechas invertidas (`DATE_MAX`,`DATE_MIN`) → `ERR_BAD_DATE`
(`tests/test_attacks.c:47`). No hay prueba para día/mes imposibles dentro de rango.

---

## R-5 — No se pueden modificar experiencias ya validadas (operaciones normales) → **Parcial**

**Escenario de ataque.** Tras validar una experiencia, re-invocar `experience_validate` para
cambiar el `validator_id` o reusar el flujo para alterar el registro aprobado.

**Control presente.** `experience_validate` rechaza con `ERR_ALREADY_VALID` si el estado ya es
`EXP_VALIDATED` (`src/experience.c:97-100`), antes de cualquier escritura; en el éxito solo
escribe `state` y `validator_id`, nunca fechas ni `owner` (`src/experience.c:102-107`). No existe
función ordinaria que edite una experiencia validada (BR-5, P-5).

**Qué falta / matiz.** La inmutabilidad se apoya en que ningún otro módulo escriba `st->exps[]`.
`educhain_state_t` es una estructura pública (`include/common.h`): quien tenga el puntero `st`
puede modificar cualquier campo directamente sin pasar por las funciones. Esto es inherente al
diseño en C sin encapsulación y coincide con el supuesto del modelo (§10). Lo marco **Parcial**
por ese límite de confianza, no por un defecto en la ruta implementada: *ninguna operación
ordinaria* muta lo validado, y eso se cumple.

**Corrección sugerida.** Documentar explícitamente que la inmutabilidad es a nivel de API y
depende de que el caller no escriba `educhain_state_t` directamente (ya implícito en §10).

**Cobertura de prueba.** A3: re-validación → `ERR_ALREADY_VALID` y conteo intacto
(`tests/test_attacks.c:33-37`).

---

## R-6 — No se pueden utilizar credenciales en estado BLOCKED o REVOKED → **Protegida**

**Escenario de ataque.** Presentar una credencial `BLOCKED` o `REVOKED` como válida, o que
`credential_is_valid` devuelva verdadero en esos estados.

**Control presente.** `credential_is_valid` retorna 1 únicamente si el estado es `CRED_ACTIVE`
(`src/credentials.c:23-26`), apoyado en `credential_status` que devuelve `-1` si no existe
(`src/credentials.c:9-21`). `PENDING`, `BLOCKED` y `REVOKED` → no válida (BR-6, P-6).

**Cobertura de prueba (ampliada).** A6: credencial revocada no es válida
(`tests/test_attacks.c:50-53`). A17 (nuevo): `PENDING` y `BLOCKED` → `credential_is_valid == 0`
(`tests/test_attacks.c:133-143`). **La brecha de los cuatro estados queda cerrada.**

**Qué falta.** Nada pendiente.

---

## R-7 — Transiciones de estado de credenciales autorizadas → **Protegida** (antes Parcial)

**Escenario de ataque.** (a) Revivir una credencial terminal `REVOKED → ACTIVE`; (b) saltar
estados (`PENDING → BLOCKED` directo, `BLOCKED → PENDING`); (c) activar algo ya activo; (d) un
rol no autorizado dispara una transición.

**Control presente.** Las transiciones de `activate/block/reactivate` pasan por `cred_transition`
(`src/credentials.c:90-124`), que exige estado origen exacto (`from`) o devuelve
`ERR_BAD_TRANSITION`, y comprueba `security_can` antes de mutar. Mapeo:
- `activate`: `PENDING → ACTIVE` (`src/credentials.c:129-132`)
- `block`: `ACTIVE → BLOCKED` (`src/credentials.c:134-137`)
- `reactivate`: `BLOCKED → ACTIVE` (`src/credentials.c:139-142`)
- `revoke`: desde `PENDING/ACTIVE/BLOCKED`, rechaza solo si ya está `REVOKED`
  (`src/credentials.c:141-165`, condición terminal en `:157-159`).

Cualquier otra transición no tiene función que la realice → inalcanzable. `REVOKED` es terminal:
no hay `from == REVOKED` en ningún mapeo y `revoke` sobre `REVOKED` da `ERR_BAD_TRANSITION`
(`src/credentials.c:157-159`). Esto bloquea (a), (b) y (c). La autorización por rol la impone
`security_can` para cada `op_code` (`src/credentials.c:101`, `:150`), lo que bloquea (d).

**Por qué cambia de Parcial a Protegida.** En la ronda 1 marqué R-7 como **Parcial** por dos
motivos, ambos resueltos ahora:

1. **Inconsistencia de documentación (diagrama vs tabla sobre `PENDING → REVOKED`).** Era parte
   de por qué la regla estaba "Parcial": el diagrama ASCII de §5.1 no dibujaba la arista
   `PENDING → REVOKED` e insinuaba que era ilegal, mientras la tabla la permitía y el código la
   implementa. **Verificado en esta ronda: el diagrama ya muestra explícitamente la transición**
   ("revoke permitido desde PENDING, ACTIVE o BLOCKED → REVOKED (terminal)") y la arista de
   revoke desde PENDING. Diagrama ↔ tabla ↔ código (`src/credentials.c:141-165`) ahora coinciden
   (ver tabla de coherencia al inicio). No queda ninguna otra inconsistencia en la máquina de
   estados.
2. **Brecha de pruebas adversariales de transición.** Faltaban casos negativos y el caso
   positivo `PENDING → REVOKED`. **Ahora están cubiertos:**
   - A13: `PENDING → BLOCKED`, `reactivate` sobre PENDING, `activate`/`reactivate` sobre ACTIVE →
     `ERR_BAD_TRANSITION` (`tests/test_attacks.c:87-99`).
   - A14: rol no autorizado en `block`/`revoke` (`ROLE_PRO`, `ROLE_VERIFIER`, `ROLE_HR`) →
     `ERR_PERM` (`tests/test_attacks.c:101-111`).
   - A15: `PENDING → REVOKED` como transición **permitida** → `EDU_OK` y estado `CRED_REVOKED`
     (`tests/test_attacks.c:113-120`).
   - A7: `REVOKED → ACTIVE` vía `reactivate` → `ERR_BAD_TRANSITION` (`tests/test_attacks.c:56`).

Con la especificación alineada y cada transición (legal, ilegal y por rol) fijada por una prueba
ejecutable, el control cumple su objetivo en todas las rutas examinadas. Lo elevo a
**Protegida** (BR-7, P-7, P-8).

**Qué falta.** Nada pendiente para el camino implementado. Observación menor para completitud:
no hay un caso explícito para `activate`/`reactivate` ejecutados por un rol no autorizado (solo
se prueban `block`/`revoke` por rol en A14); la lógica de `security_can` lo cubre, pero un caso
cerraría la matriz de autorización de transiciones por completo. Esto no rebaja el estado.

---

## R-8 — No se pueden superar los límites de almacenamiento → **Protegida**

**Escenario de ataque.** Insertar hasta desbordar un arreglo (`pros`, `exps`, `creds`, `log`) o
forzar `seq` más allá de `INT_MAX`.

**Control presente.** Toda inserción comprueba `count >= MAX_*` → `ERR_FULL` antes de escribir:
profesionales (`src/professionals.c:53-55`), experiencias (`src/experience.c:62-64`), credenciales
(`src/credentials.c:74-76`). El log valida `log_count >= MAX_EVENTS → ERR_FULL`
(`src/audit.c:16-18`) y `seq` tiene tope defensivo: si el último `seq >= 2000000000` devuelve
`ERR_FULL` sin incrementar (`src/audit.c:23-28`). Las operaciones que auditan reservan espacio de
log **antes** de mutar (p. ej. `src/professionals.c:56-58`), conciliando capacidad y atomicidad
(BR-8, BR-11, P-9, P-10).

**Cobertura de prueba (ampliada).** A12: log lleno → `ERR_FULL` y profesional no agregado
(`tests/test_attacks.c:76-82`). A11: `seq` estrictamente creciente (`tests/test_attacks.c:67-73`).
A18 (nuevo): arreglo `pros` lleno exacto a `MAX_PROFESSIONALS` y siguiente registro → `ERR_FULL`
(`tests/test_attacks.c:145-158`). **La brecha de "arreglo lleno, no solo log" queda cerrada para
`pros`.**

**Qué falta.** A18 cubre `pros`; aún no hay prueba de `exps`/`creds` llenos ni del tope de `seq`
cerca de 2_000_000_000. Controles presentes; conviene fijar esos bordes restantes con pruebas.

---

## R-9 — Las operaciones validan correctamente los parámetros → **Parcial**

**Escenario de ataque.** Pasar `NULL`, IDs `<= 0`, IDs `> ID_MAX`, para provocar desreferencia
nula, acceso fuera de límites o aceptación de identificadores inválidos.

**Control presente.** Punteros: cada función pública chequea `st == 0 → ERR_NULL`
(`src/professionals.c:40`, `src/experience.c:30`, `src/credentials.c:54`, `src/audit.c:13`); las
de lectura devuelven `-1`/`0` ante `NULL` (`credential_status` `src/credentials.c:11-13`). IDs
validados contra `ID_MIN..ID_MAX` en registro de profesional (`src/professionals.c:44-46`),
experiencia incluyendo `caller_id`/`owner_id`/`exp_id` (`src/experience.c:33-37`), credenciales
(`src/credentials.c:56-58`) y validación (`src/experience.c:83-85`) (BR-9, P-11, P-13).

**Qué falta / escenario no cubierto.** El orden de comprobaciones en credenciales y validación
es **`ERR_BAD_ID` antes que `ERR_PERM`** (p. ej. `cred_transition` `src/credentials.c:98-102`;
`experience_validate` `src/experience.c:83-88`). Un atacante sin permiso que pasa un ID inválido
recibe `ERR_BAD_ID`, no `ERR_PERM`: no es fuga de estado (nada se muta), pero sí una **fuga de
información por código de error** que revela que el ID es inválido antes de comprobar autorización.
Igual en `professional_register`/`experience_register`. Un diseño "deny-first" comprobaría permiso
primero. Lo marco **Parcial** por esto y por la ausencia de validación de calendario (ver R-4).

Además, `audit_record_event` no valida rango de `actor_id`/`op_code`/`target_id` (datos de traza
interna, no entradas directas de usuario), aceptable porque siempre se invoca internamente con
valores ya validados.

**Corrección sugerida.** Decidir y documentar el orden de errores (permiso vs id). Si se prioriza
no filtrar validez de IDs a roles no autorizados, mover `security_can` antes de la comprobación de
rango. Mantenerlo como está es defendible (modelo académico sin autenticación real), pero debe ser
una decisión explícita.

**Cobertura de prueba.** A8: `NULL` en `professional_register` y `audit_record_event`
(`tests/test_attacks.c:60-61`). A9: id negativo, id `> ID_MAX` y `caller_id == 0`
(`tests/test_attacks.c:64-66`). No hay prueba para el orden perm-vs-id, ni para `new_id == 0`
(el caso `caller_id == 0` sí está en A9).

---

## R-10 — Los errores producen resultados controlados sin corromper el estado → **Protegida**

**Escenario de ataque.** Provocar un error a mitad de una operación para dejar un contador
incrementado sin el registro correspondiente, o un registro escrito sin auditar.

**Control presente.** Patrón consistente "validar TODO antes de mutar NADA"
(`src/professionals.c:38`). En toda operación de escritura, los chequeos (null, id, permiso,
duplicado, capacidad, reserva de log) ocurren antes de cualquier asignación; el incremento de
`count` y el `audit_record_event` van al final en el camino de éxito. Un error retorna código
negativo sin tocar la estructura (BR-10, P-1). El log es append-only: `audit_record_event` solo
escribe en `log[log_count]` y luego incrementa, nunca reescribe eventos previos
(`src/audit.c:30-37`).

**Cobertura de prueba.** A10: error de permiso no muta `pros_count` (`tests/test_attacks.c:70-72`);
A3 verifica conteo intacto tras `ERR_ALREADY_VALID`; A12 verifica que el profesional no se agrega
con log lleno (`tests/test_attacks.c:81`).

**Qué falta.** La reserva de log antes de mutar evita el caso "registro sin evento"; no hay prueba
del inverso para experiencias/credenciales (mutación parcial ante log lleno en esos módulos). La
lógica reserva el espacio de log antes de escribir en los tres módulos, así que el invariante se
mantiene; una prueba por módulo lo confirmaría.

---

## Brechas de prueba adversarial priorizadas (actualizadas)

Las brechas de la ronda 1 marcadas **[CERRADA]** ya tienen prueba ejecutable; el resto sigue
abierto. Ordenadas por impacto residual:

1. **[ABIERTA] R-9 — orden de error permiso-vs-id.** Decidir y documentar si `ERR_PERM` debe
   preceder a `ERR_BAD_ID`; añadir prueba del orden elegido. (Fuga de información menor.)
2. **[ABIERTA] R-4 — calendario:** día/mes imposibles dentro de rango (`20210231`, `20210000`),
   si se decide reforzar la validación más allá de rango.
3. **[ABIERTA] R-8 — bordes restantes:** arreglos `exps` y `creds` llenos (A18 solo cubre `pros`)
   y tope de `seq` cerca de 2_000_000_000.
4. **[ABIERTA] R-10 — mutación parcial ante log lleno** en los módulos de experiencia y
   credencial (hoy solo probado en profesionales vía A12).
5. **[ABIERTA] R-1 — matriz completa:** casos negativos explícitos de `ROLE_VERIFIER`/`ROLE_ADMIN`
   en `validate_experience`.
6. **[ABIERTA] R-7 — completitud de autorización de transición:** rol no autorizado en
   `activate`/`reactivate` (A14 solo prueba `block`/`revoke`).
7. **[ABIERTA] R-9 — borde `new_id == 0`** en `professional_register` (A9 ya cubre `caller_id == 0`).

**Cerradas en esta ronda (ya no son brechas):**
- [CERRADA] R-7 — transiciones ilegales (`PENDING→BLOCKED`, `reactivate`/`activate` sobre ACTIVE)
  → A13. Autorización de transición por rol en `block`/`revoke` → A14. `PENDING→REVOKED` permitida
  → A15.
- [CERRADA] R-3 — duplicados de `exp_id` (A4), `cred_id` y segunda credencial por `owner_id` (A16).
- [CERRADA] R-6 — `BLOCKED` y `PENDING` → no válidas (A17).
- [CERRADA] R-8 — arreglo `pros` lleno (A18).

---

## Hallazgos de documentación (fuera de código, reportados)

- **R-7: inconsistencia diagrama vs tabla en `docs/phase1_model.md §5.1` — RESUELTA.** El
  diagrama ASCII ya dibuja la arista `PENDING → REVOKED` y la anota explícitamente ("revoke
  permitido desde PENDING, ACTIVE o BLOCKED → REVOKED"), coherente con la tabla (celda
  PENDING/REVOKED = ✓) y con el código (`src/credentials.c:141-165`). Verificado en esta ronda.
  No queda ninguna otra inconsistencia en las máquinas de estado (§5.1 y §5.2).

---

## Límites de esta auditoría (honestidad requerida)

- **No afirmo que EduChain sea "seguro por completo".** Reporto el estado de cada control sobre el
  código leído. R-4, R-5 y R-9 permanecen **Parcial**.
- Varias garantías (inmutabilidad R-5, autorización R-1) dependen del supuesto del modelo (§10) de
  que la capa externa no escribe `educhain_state_t` directamente y provee `caller_role`/`caller_id`
  confiables. El núcleo verifica *autorización*, no *autenticación*.
- No ejecuté Frama-C (WP/Eva) ni los sanitizadores; las propiedades P-* siguen siendo candidatas no
  demostradas en esta revisión. Esta auditoría es inspección estática de código y revisión de
  pruebas, no una prueba formal. La elevación de R-7 a **Protegida** se basa en coherencia
  especificación↔código y en cobertura de pruebas adversariales, no en una demostración formal.
