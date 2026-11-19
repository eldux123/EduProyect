# Revisión de honestidad de la verificación formal — EduChain · agente `revisor-verificacion` (ronda 2)

**Rol:** experto en Frama-C (WP / Eva) y en el uso honesto de la verificación formal. Guardián contra el exceso de confianza. **Solo reviso y reporto; no modifico archivos del proyecto.** Sesgo por defecto: ante la duda, una propiedad está **Pendiente**, no demostrada.

**Alcance auditado:** `docs/phase1_model.md`, `docs/formal_verification.md`, `include/`, `src/`, `main.c`, `verification/` (incluye `verify.sh`, `eva_harness.c`, `wp_results.txt`, `eva_results.txt`), `tests/`, y la revisión previa (`.agents/reviews/revisor-verificacion.md`, ronda 1).

**Objeto de esta ronda:** verificar que las ediciones de honestidad hechas desde la ronda 1 quedaron **fieles a la evidencia** y que **no se introdujo ninguna afirmación nueva sin respaldo**.

---

## 0. Entorno de esta revisión

**Frama-C NO está instalado en este entorno (Windows).** No pude ejecutar WP ni Eva para contrastar; esta revisión se basa en los **artefactos presentes** en el repo (principalmente `verification/wp_results.txt` y `verification/eva_results.txt`). No invento una ejecución que no hice.

Comprobaciones objetivas realizadas sobre los artefactos:

| Comprobación | Resultado |
|--------------|-----------|
| `grep "Z3"` sobre `verification/wp_results.txt` | **0 coincidencias** (confirmado) |
| Tamaño de `verification/eva_results.txt` | **0 bytes** (vacío, confirmado) |
| Etiqueta `Completo`/`COMPLETO` en `formal_verification.md` | **0 coincidencias** (confirmado: ya no existe) |
| `security.c` en `wp_results.txt` | `Proved goals: 69/69 · Terminating:1 · Unreachable:1 · Qed:67` |

> La fecha del sistema (2026-11-18) se toma como **real del reloj WSL** según lo establecido en el contexto del prompt; no la trato como inventada. Sí sigue siendo una fecha futura respecto al calendario, lo que conviene no ocultar, pero ya no la marco como "sospechosa/posiblemente ficticia".

---

## 1. Veredicto global de la ronda 2

Las tres ediciones de honestidad se aplicaron y **son fieles a la evidencia**. No detecté que las correcciones introdujeran ninguna afirmación nueva sin respaldo. En concreto:

- **V-1 (Z3):** corregida y exacta. La categoría ahora es "Probado (Qed / Alt-Ergo)" y el texto aclara que se configuró `-wp-prover alt-ergo,z3` pero **ninguna** obligación se cerró con Z3. Coincide con 0 ocurrencias de "Z3" en `wp_results.txt`.
- **V-2 (`security.c`):** corregida y exacta. La etiqueta pasó de "Completo" a "obligaciones generadas demostradas", con nota al pie y aclaración de que P-1 de extremo a extremo está pendiente en los llamadores. Coincide con 69/69 (67 Qed + 1 Terminating + 1 Unreachable).
- **V-3 (plataforma/fecha):** aclarada de forma honesta. Se documenta que el target nominal era "WSL2 Debian" y la ejecución real fue en Ubuntu 24.04; la fecha se trata como la del reloj del sistema.
- **Diagrama §5.1 (PENDING → REVOKED):** corregido. El ASCII ya muestra la arista y está alineado con la tabla y con BR-7.

Dos correcciones **siguen pendientes en los docs** (no eran objeto de las ediciones de esta ronda): **V-4 (Eva)** y **V-5 (tabla de amenazas)**. Ninguna es un error introducido; son brechas que aún no se cerraron.

---

## 2. Verificación detallada de cada edición

### 2.1 Z3 — categoría "Probado (Qed / Alt-Ergo)" (corrige V-1)

**Afirmado ahora** (`docs/formal_verification.md`):
- Categoría de la taxonomía: **"Probado (Qed / Alt-Ergo)"** (ya no incluye Z3).
- Texto: WP se invocó con `-wp-prover alt-ergo,z3`, pero en la línea base **todas** las obligaciones cerradas lo fueron por Qed o Alt-Ergo; **ninguna se cerró con Z3**; why3 consulta en orden y Alt-Ergo respondió primero; no se presenta Z3 como contribuyente hasta que una obligación se cierre explícitamente con él.
- Tabla de entorno: "Z3 4.13.0 configurado (vía why3) pero sin ninguna obligación cerrada en esta línea base".

**Evidencia real:**
- `verification/wp_results.txt` → **0 ocurrencias de "Z3"**. Los desgloses por módulo solo listan `Qed:` y `Alt-Ergo :`.
- `verify.sh` incluye `Z3` en el filtro `grep -E`, de modo que si Z3 hubiera cerrado alguna obligación, aparecería en el artefacto. No aparece.

**Veredicto:** **Correcto / fiel a la evidencia.** La redacción es honesta y no sobreafirma. Matiz menor (no bloqueante): el doc dice que Z3 "se configuró", lo cual es cierto a nivel de línea de comandos (`-wp-prover alt-ergo,z3`), pero su **disponibilidad efectiva en why3** sigue sin un artefacto que la pruebe (no se guardó la salida de `why3 config list-provers`). La nueva redacción no afirma que Z3 esté "en uso", así que no es una afirmación sin respaldo; solo queda como mejora de reproducibilidad.

### 2.2 `security.c` — "obligaciones WP generadas demostradas" (corrige V-2)

**Afirmado ahora** (`docs/formal_verification.md`):
- Resumen por módulo: estado **"Obligaciones generadas demostradas*"** (ya no "Completo").
- Detalle §1: "69/69 obligaciones WP **generadas** demostradas (67 Qed + 1 Terminating + 1 Unreachable), sobre un `switch` puro sin bucles"; "**No equivale a 'P-1 verificada'**"; "P-1 de extremo a extremo está **pendiente**" en los módulos llamadores.
- Nota al pie: "obligaciones generadas demostradas" = WP cerró todas las obligaciones que **generó** (RTE + terminación); no implica propiedades funcionales de negocio de extremo a extremo.

**Evidencia real** (`wp_results.txt`, bloque `src/security.c`):
```
[wp] Proved goals:   69 / 69
  Terminating:     1
  Unreachable:     1
  Qed:            67
```

**Veredicto:** **Correcto / fiel a la evidencia.** Los números (69/69; 67 Qed; 1 Terminating; 1 Unreachable; 0 Alt-Ergo; 0 Z3) coinciden exactamente. La frase "mayormente Qed" es precisa (67 de 69 son Qed). La aclaración de que P-1 completa vive en los llamadores (donde hay Failures/Timeouts) es coherente con `professionals.c` (5 Failure en `*_register_ensures*`) y con `experience.c` (Failure en `experience_validate_ensures`). No se introdujo ninguna afirmación nueva sin respaldo.

### 2.3 Plataforma y fecha (corrige V-3 parcialmente)

**Afirmado ahora** (`docs/formal_verification.md`, tabla de entorno):
- Plataforma: "WSL2 Ubuntu 24.04, instalación vía opam (el target nominal en phase1_model.md §11 era 'WSL2 Debian'; la ejecución real fue en Ubuntu 24.04, compatible vía mismo apt/opam)".
- Fecha: "2026-11-18 19:46 UTC".

**Evidencia / contexto:**
- `phase1_model.md` §11 efectivamente fija el target "WSL2 Debian" y admite que la máquina tiene Ubuntu; la aclaración del doc de verificación es consistente con ello.
- La fecha 2026-11-18 se toma como la **real del reloj del sistema WSL** (establecido en el prompt). No la trato como inventada.

**Veredicto:** **Correcto / fiel a la evidencia.** La divergencia Debian↔Ubuntu quedó explícita y honesta. Observación menor de reproducibilidad (no bloqueante): las versiones de herramientas (Frama-C 33.0, Alt-Ergo, Z3 4.13.0) siguen documentadas solo en prosa; guardar la salida de `env_check.sh` / `_versions.sh` junto a `wp_results.txt` las haría verificables. No es una afirmación sin respaldo, es una mejora pendiente.

### 2.4 Diagrama de estados §5.1 (PENDING → REVOKED)

**Antes:** el ASCII insinuaba que `PENDING → REVOKED` no era posible (solo mostraba `revoke` desde la zona ACTIVE/BLOCKED), en contradicción con la tabla, que sí lo permite.

**Ahora** (`docs/phase1_model.md` §5.1): el diagrama incluye la rama `revoke` desde `PENDING` hacia `REVOKED` y la línea explícita: "revoke permitido desde PENDING, ACTIVE o BLOCKED → REVOKED (terminal)". La tabla de transiciones marca `PENDING → REVOKED` como ✓ y BR-7 dice "`REVOKED` es terminal".

**Veredicto:** **Correcto / alineado.** Diagrama ↔ tabla ↔ regla (BR-7) ahora coinciden. Resuelve el defecto de documentación que habían señalado `revisor-seguridad` y `revisor-c`.

---

## 3. Tabla por propiedad — Afirmado vs. Evidencia real vs. Veredicto (actualizada)

Veredictos: **Demostrado** (WP cerró las obligaciones de esa propiedad) · **Advertencia** (Failure/Timeout/Unknown) · **Pendiente** (no intentado o sin salida, p. ej. Eva) · **Sin evidencia** (se afirma pero el artefacto no lo respalda).

| Prop. | Afirmado (modelo §8 / doc) | Evidencia real en artefactos | Veredicto |
|------|-----------------------------|------------------------------|-----------|
| **P-1** autorización ⇒ ERR_PERM, estado intacto | WP | `security.c` 69/69 (Qed) cubre solo el decisor; la propiedad funcional vive en llamadores: `professional_register_ensures*` = **5 Failure**; otras ramas en Timeout. El doc ahora lo dice así. | **Advertencia** (parcial; Failure en el camino que la materializa) |
| **P-2** no autovalidación ⇒ ERR_SELF_VALIDATE | WP | `experience_validate_ensures` = **Failure**; `ensures_2`/`assigns` en **Timeout** | **Advertencia** |
| **P-3** unicidad de IDs tras registro | WP | `*_register_ensures` en **Failure** + timeouts; ninguna obligación de unicidad cerrada visible | **Advertencia / Pendiente** |
| **P-4** experiencia con start≤end en rango | WP | `experience_validate`/`register` con Failure+Timeout; no consta cerrada | **Advertencia** |
| **P-5** VALIDATED inmutable (start/end/owner) | WP | `experience_validate_assigns_normal_part13` = **Failure**; parte de `assigns` en Timeout | **Advertencia** |
| **P-6** is_valid ⇔ ACTIVE | WP | `credentials.c`: `credential_is_valid_assigns_normal_part1` en **Timeout**; `credential_status` RTE en Timeout | **Advertencia / Pendiente** |
| **P-7** transición ∈ tabla permitida | WP | `cred_transition_*` RTE y `requires` de audit en **Timeout** | **Pendiente** |
| **P-8** REVOKED terminal | WP | Sin obligación específica cerrada aislable; dentro de los timeouts de `credentials.c`. (Modelo/diagrama §5.1 ya correcto, pero eso es documentación, no prueba WP.) | **Pendiente** |
| **P-9** accesos a arreglo en 0..count-1 | WP + Eva | WP: RTE `index_bound`/`mem_access` de bucles **mayormente en Timeout**. Eva: **sin salida** | **Advertencia (WP) / Pendiente (Eva)** |
| **P-10** sin overflow en contadores / seq | WP + Eva | WP: `experience_total_days_..._signed_overflow_4` en **Timeout**. Eva: **sin salida** | **Advertencia / Pendiente** |
| **P-11** assigns vacío en funciones de lectura | WP | `credential_is_valid_assigns` en Timeout; sin cierre explícito | **Pendiente** |
| **P-12** log monótono, seq creciente | WP | `audit.c` 68/74, pero RTE de `audit_verify_event` en **Timeout**; seq creciente no aparece como obligación cerrada aislable | **Advertencia** |
| **P-13** ausencia de UB (punteros, no-init) | Eva | `eva_results.txt` **vacío (0 bytes)** | **Pendiente (sin evidencia)** |
| **Z3 integrado y en uso** | ~~doc~~ (ya corregido) | **0** obligaciones cerradas por Z3 en `wp_results.txt`. El doc **ya no** lo presenta como contribuyente. | **Sin evidencia** (afirmación retirada; correcto) |
| **Eva ejecutado** | verify.sh | salida **vacía**; el doc lo marca pendiente | **Pendiente** |
| **`security.c` "Completo"** | ~~doc~~ (ya corregido) | 69/69 **generadas** cerradas (Qed); etiqueta cambiada a "obligaciones generadas demostradas" + P-1 pendiente | **Demostrado (solo obligaciones generadas); etiqueta ya honesta** |

> **Resumen:** **ninguna** propiedad de negocio P-1…P-13 está hoy en estado **Demostrado de extremo a extremo**. Lo único demostrado son las obligaciones WP **generadas** para `security.c` y los Qed/Alt-Ergo dispersos de otros módulos (condiciones locales: RTE, terminación, postcondiciones parciales), no las propiedades P-* completas. Esta situación no cambió con las ediciones; lo que cambió es que la **documentación ya no sobreafirma**.

---

## 4. Estado de las correcciones V-1..V-5

| ID  | Corrección | Estado en los docs | Verificación contra evidencia |
|-----|------------|--------------------|-------------------------------|
| **V-1** | Z3 no presentado como solucionador en uso | **RESUELTA** | Categoría "Probado (Qed / Alt-Ergo)"; texto aclara `-wp-prover alt-ergo,z3` sin obligaciones cerradas por Z3. Coincide con 0 ocurrencias de "Z3" en `wp_results.txt`. Fiel. |
| **V-2** | `security.c` dejar de etiquetarse "Completo" | **RESUELTA** | Etiqueta → "obligaciones generadas demostradas" + nota al pie + P-1 de extremo a extremo pendiente. Coincide con 69/69 (67 Qed + 1 Terminating + 1 Unreachable). Fiel. |
| **V-3** | Entorno/fecha de la línea base | **RESUELTA** (lo esencial) | Debian (nominal) vs. Ubuntu 24.04 (real) aclarado explícitamente; fecha tratada como reloj real del sistema. Pendiente opcional, no bloqueante: guardar salida de versiones/`why3 config list-provers` como artefacto. |
| **V-4** | Eva: no leer la ausencia como éxito | **PENDIENTE** | `eva_results.txt` sigue **vacío (0 bytes)**. El doc lo marca "pendiente" (bien), pero no hay resultados de Eva; P-13 y la parte Eva de P-9/P-10 siguen **Pendientes / sin evidencia**. No se introdujo afirmación falsa; la brecha persiste. |
| **V-5** | Tabla de amenazas que mezcla "test" y P-* (WP/Eva) | **PENDIENTE** | `phase1_model.md` §9 mantiene la columna "Prueba" con "test + Eva/WP" para desbordamientos y NULL. Mientras las P-* sigan Pendientes/Timeout/Failure, el respaldo real de esas amenazas es el **test adversarial**, no la verificación. Conviene separar "cubierto por test (observacional)" de "demostrado por WP/Eva (pendiente)". |

**Correcciones nuevas de esta ronda, fuera de V-1..V-5:**
- **Diagrama §5.1 (PENDING → REVOKED):** **RESUELTA.** Diagrama ↔ tabla ↔ BR-7 alineados.

---

## 5. ¿Las ediciones introdujeron alguna afirmación nueva sin respaldo?

**No.** Revisé los textos editados buscando sobreafirmaciones nuevas:

- No reaparece "Completo"/"COMPLETO" en `formal_verification.md` (0 coincidencias).
- No se afirma que Z3 demostró nada; al contrario, se niega explícitamente.
- No se afirma que P-1 esté verificada; se dice que está pendiente de extremo a extremo.
- El diagrama §5.1 no añade transiciones que la tabla no permita (solo alinea `PENDING → REVOKED`, que la tabla ya marcaba ✓).
- La fecha y plataforma se describen como lo que son, sin inflar.

El proyecto conserva su disciplina honesta de la ronda 1 (taxonomía Probado / No demostrado / Timeout; "Failure/Timeout ≠ contraejemplo"; Eva marcado pendiente; `main.c` "NO verificada"; `verify.sh` aborta si falta `frama-c`).

---

## 6. Recomendaciones pendientes (no bloqueantes, para cerrar del todo)

1. **V-4:** cuando se ejecute Eva con un harness/punto de entrada válido, guardar su salida real en `eva_results.txt`; hasta entonces, el informe final no debe listar P-13 ni la parte Eva de P-9/P-10 como respaldadas.
2. **V-5:** en `phase1_model.md` §9, separar explícitamente "cubierto por test adversarial (observacional)" de "demostrado por WP/Eva (pendiente)".
3. **V-3 (opcional):** guardar como artefacto la salida de versiones y de `why3 config list-provers`, para que Frama-C 33.0 / Alt-Ergo / Z3 4.13.0 y la disponibilidad efectiva de Z3 sean verificables, no solo afirmadas.

---

## 7. Límites de esta revisión

- **No ejecuté Frama-C** (no está instalado en este entorno). Los números de WP se toman tal cual de `wp_results.txt`; no los recalculé ni los reproduje.
- No modifiqué ningún archivo del proyecto. La **única escritura** es este informe (`.agents/reviews/revisor-verificacion.md`).
- Ante la duda, clasifiqué como **Pendiente / Advertencia**, no como Demostrado.
