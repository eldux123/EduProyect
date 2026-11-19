# Implementation Plan — EduChain Fase 4 (ACSL P1 + honestidad documental P2)

Rama `fase4-acsl` @ base `a710c43` (v2). Trabajo directo sobre el checkout, SIN worktree.
Working tree limpio al inicio. Archivos en juego: `include/credentials.h`, `src/credentials.c`,
`include/experience.h`, `src/experience.c`, `docs/formal_verification.md` (solo P2).

Entorno: WSL2 distro **Ubuntu** (no la default `docker-desktop`). Frama-C 33.0 vía opam;
`frama-c` no está en PATH hasta `eval "$(opam env)"`. PowerShell rompe el quoting de
`$(opam env)`: invocar WP/Eva **siempre** vía `wsl -d Ubuntu bash <script>`, nunca inline.

Scripts existentes (ya en el repo, no crear):
- Módulo completo: `verification/_run_one.sh <archivo.c>`
- Función aislada: `verification/_run_fct.sh <funcion> <archivo.c>`
- Regresión 99/99 (build normal + ASan/UBSan): `run_tests.sh`

---

## Decisiones de diseño (los dos puntos abiertos)

### Decisión 1 — Orden de las cinco correcciones de P1

Orden elegido: **5 → 1 → 2 → 4 → 3**. Es decir, primero el bloque de credenciales en dependencia
interna (assigns por behavior, luego contrato de `cred_transition`, luego los wrappers, luego
`revoke`/P-8), y por último P-2 en experience.

Razonamiento:

1. **#5 primero (separar `assigns` por behavior de éxito/error en credentials).** El `assigns`
   actual de las operaciones de escritura (`credential_issue`, `cred_transition`, los wrappers,
   `credential_revoke`) nombra `st->creds[st->creds_count]` / el conjunto acotado por `cred_id`
   junto con `st->log[st->log_count]`, y en las ramas de error `log_count` puede valer `MAX_EVENTS`,
   lo que nombra `log[MAX_EVENTS]` (fuera de rango) y genera los sub-objetivos `*_assigns_part*`.
   Separar `assigns` por `behavior` (éxito escribe estado+log; error `assigns \nothing` o solo lo que
   realmente toca) es **fortalecimiento de spec** (es más preciso) y limpia ruido de sub-objetivos
   antes de medir el efecto de #1/#2. Hacerlo primero da una línea de medición más limpia: los deltas
   de #1/#2 se atribuyen a la transición, no al framing.

2. **#1 antes de #2 (contrato fuerte de `cred_transition` antes de formalizar la transición en los
   wrappers).** El brief lo dice literalmente: fortalecer `cred_transition` "desbloquea las
   obligaciones activate/block/reactivate_ensures_2 y cred_transition_ensures". Los wrappers
   (`credential_activate/block/reactivate`) son delgados y delegan en `cred_transition(... from, to)`.
   Para que WP pueda derivar en cada wrapper "en éxito el estado pasa de `from` a `to`" y P-1
   (autorización por `op_code`→rol), el contrato de la auxiliar debe exponer esos hechos como
   `ensures` parametrizados por `from`/`to`/`op_code`. Sin #1, los `ensures` de #2 no tienen de dónde
   derivarse. Por eso #1 precede a #2. Son el mismo módulo y se re-ejecuta WP una sola vez tras cada
   uno.

3. **#4 después (reescribir P-8 de `credential_revoke` anclado en `\old`).** `credential_revoke` no
   pasa por `cred_transition` (tiene su propio cuerpo), así que es independiente de #1/#2 en código;
   pero comparte el patrón de `assigns`/framing que #5 ya habrá limpiado, y conviene abordarlo cuando
   el resto de credentials ya está estabilizado para no mezclar su efecto existencial pesado (P-8)
   con la medición de los wrappers.

4. **#3 al final (P-2 no autovalidación en `experience_validate`).** Es el único cambio en
   `experience.c`/`experience.h`; aislarlo al final evita re-ejecutar WP de experience mientras se
   itera en credentials, y permite correr Eva + regresión una sola vez al terminar todo P1 (el
   procedimiento (e) del brief). Experience es además el módulo con más timeouts preexistentes, así
   que conviene tocarlo con la cabeza despejada y medir su delta aislado.

Resumen del orden: **credentials (#5 assigns → #1 cred_transition → #2 wrappers → #4 revoke/P-8)
luego experience (#3 P-2)**.

### Decisión 2 — Acotar las iteraciones WP por corrección

Decisión: **sí, acotar a un máximo de 3 re-ejecuciones de WP por corrección** (una medición inicial
+ hasta 2 reintentos de ajuste de la anotación). Si al tercer intento la obligación sigue en
Failure/Timeout, se **congela** como "pendiente / no demostrada", se documenta (regla de negocio +
estado formal + respaldo dinámico) y se avanza. No se sube el timeout de WP por encima de 20 s ni se
añaden `assumes` para forzar cierres.

Razonamiento:

- El historial del propio `formal_verification.md` ya muestra que el cuello de botella es el framing
  de WP sobre `educhain_state_t` (~7 KB) a través de `audit_record_event`, **no** la spec: los
  intentos previos de extraer auxiliares (`exp_insert` 41/41 aislada, `pros_insert` 21/21 aislada)
  se demostraron solos pero **no cerraron el call-site**, y en un caso empeoraron el neto. Repetir
  iteraciones más allá de 3 reproduce ese gasto sin ganancia esperable.
- El propio proyecto ya fijó un precedente de "3 iteraciones acordadas" (opción 3 en
  `professionals.c`). Mantener 3 da consistencia y un criterio de parada objetivo que respeta la
  regla inquebrantable "NO forzar 100%".
- 3 intentos son suficientes para distinguir las tres causas reales (anotación mejorable vs. framing
  del probador vs. existencial/cuantificador pesado) y clasificar honestamente Failure/Timeout/Unknown.

El cap es por **corrección** (no por sesión): cada una de las cinco tiene su propio presupuesto de
3 re-ejecuciones del módulo afectado.

---

## Reglas inquebrantables (se aplican a TODO P1)

- No debilitar contratos para subir el % demostrado. Todo cambio **añade o fortalece** spec.
- No forzar 100%: obligación que no cierre tras el cap → "pendiente / no demostrada" documentada.
- No introducir `assumes` injustificados que presupongan la propiedad a demostrar.
- Conservar **INTACTOS** `verification/wp_results_baseline.txt` y la sección "LÍNEA BASE
  (NO MODIFICAR)" de `formal_verification.md`. Los resultados nuevos se **añaden** debajo.
- Mantener `caller_id` real, la máquina de estados, la unicidad y la atomicidad existentes.
  Cero cambios funcionales de comportamiento salvo que una corrección de spec lo exija; en ese caso
  documentarlo explícitamente.
- Límites reales del probador (mantener descritos como límite genuino, NO intentar forzar):
  framing del struct ~7 KB que bloquea atomicidad de contadores a través de `audit_record_event`;
  ausencia de un `state_inv` global (unicidad/append-only global inexpresables).

Procedimiento por corrección (del brief): (a) medir WP del módulo sin modificar si aún no se midió en
esta sesión; (b) aplicar la corrección de spec; (c) re-ejecutar WP del módulo; (d) registrar qué
obligaciones cierran y cuáles quedan pendientes, distinguiendo Failure / Timeout / Unknown;
(e) al terminar todo P1, correr Eva + regresión completa.

---

# Implementation Plan

- [ ] 1. Medición inicial de WP de `credentials.c` sin modificar (si no se hizo en esta sesión), para
      tener el punto de partida del módulo antes de tocarlo.
      Files: (ninguno — solo medición)
      Verify: `wsl -d Ubuntu bash verification/_run_one.sh src/credentials.c` — esperado ~226/239,
      9 Failures, 4 Timeouts (estado de partida reportado). Anotar el desglose para el registro (d).

- [ ] 2. (Corrección #5) Separar `assigns` por `behavior` de éxito vs. error en las operaciones de
      escritura de credenciales. En `include/credentials.h`, reescribir el `assigns` de
      `credential_issue`, `credential_activate`, `credential_block`, `credential_reactivate` y
      `credential_revoke` dentro de `behavior` nombrados (p. ej. `behavior success` con
      `assumes \result == EDU_OK` que escribe el slot/estado + `st->log[st->log_count]` + `log_count`,
      y `behavior error` con `assumes \result != EDU_OK` y `assigns \nothing`), con
      `complete behaviors; disjoint behaviors;`. Aplicar el mismo patrón al `assigns` del contrato
      de `cred_transition` en `src/credentials.c`. Objetivo: eliminar los sub-objetivos
      `*_assigns_part*` que surgen porque el `assigns` nombra `creds[MAX]`/`log[MAX_EVENTS]` fuera de
      rango en ramas de error. NO cambiar el cuerpo de las funciones.
      Files: include/credentials.h, src/credentials.c
      Verify: `wsl -d Ubuntu bash verification/_run_one.sh src/credentials.c` — los `*_assigns_*part*`
      que antes estaban en Timeout/Failure deben cerrar o reducirse; ningún objetivo previamente
      probado debe pasar a Failure. Registrar delta (d). Cap: máx. 3 re-ejecuciones.

- [ ] 3. (Corrección #1) Fortalecer el contrato de la auxiliar `cred_transition` en
      `src/credentials.c`: hoy no menciona estado `from`/`to` ni roles. Añadir `ensures` que expongan
      lo que los wrappers necesitan para derivar P-1 y P-7: en éxito, la credencial con `id==cred_id`
      pasa de `from` a `to` (`\old(...state)==from` y `...state==to`), y P-1 por autorización
      (`!security_can(caller_role, op_code) ==> \result==ERR_PERM`), anclando los estados pre/post
      con `\old`. Esto desbloquea `activate/block/reactivate_ensures_2` y `cred_transition_ensures`.
      NO tocar el cuerpo. NO introducir `assumes` que presupongan la transición.
      Files: src/credentials.c
      Verify: `wsl -d Ubuntu bash verification/_run_fct.sh cred_transition src/credentials.c` y luego
      `verification/_run_one.sh src/credentials.c` — `cred_transition_ensures` debe cerrar (o quedar
      documentado como pendiente si el framing lo impide). Registrar (d). Cap: máx. 3 re-ejecuciones.

- [ ] 4. (Corrección #2) Formalizar P-7 (transición legal) en los wrappers `credential_activate`,
      `credential_block`, `credential_reactivate` en `include/credentials.h`: reemplazar los `ensures`
      casi idénticos por uno que exprese "en éxito, la credencial `cred_id` pasa de su estado `from`
      específico a `to`" usando `\old(st->creds[k].state)==from ==> ...==to` para la `k` con
      `id==cred_id`, derivándolo del contrato reforzado de `cred_transition` (dep. del ítem 3).
      Cada wrapper afirma su par concreto (PENDING→ACTIVE, ACTIVE→BLOCKED, BLOCKED→ACTIVE).
      Files: include/credentials.h
      Verify: `wsl -d Ubuntu bash verification/_run_one.sh src/credentials.c` —
      `activate/block/reactivate_ensures_2` deben cerrar o reducirse; lo que no cierre se documenta
      como pendiente por framing (no se debilita). Registrar (d). Cap: máx. 3 re-ejecuciones.

- [ ] 5. (Corrección #4) Reescribir P-8 (terminalidad de REVOKED) en `credential_revoke`
      (`include/credentials.h`) anclando todo en `\old`: conservar la mitad negativa (si estaba
      REVOKED ⇒ `ERR_BAD_TRANSITION`) y **añadir** la mitad positiva: en éxito, la credencial
      `cred_id` que no estaba REVOKED queda REVOKED (`\old(state)!=CRED_REVOKED && \result==EDU_OK
      ==> state==CRED_REVOKED`). Evitar la mezcla frágil `\old`/post usando `\old` para el estado
      previo y el estado post sin `\old`, ambos para la misma `k` con `id==cred_id`.
      Files: include/credentials.h
      Verify: `wsl -d Ubuntu bash verification/_run_one.sh src/credentials.c` — `revoke_ensures*`
      cierran o quedan documentados como pendientes (existencial pesado / framing), respaldados por
      A7/A13/A15 en `tests/test_attacks.c`. Registrar (d). Cap: máx. 3 re-ejecuciones.

- [ ] 6. Medición inicial de WP de `experience.c` sin modificar (si no se hizo en esta sesión).
      Files: (ninguno — solo medición)
      Verify: `wsl -d Ubuntu bash verification/_run_one.sh src/experience.c` — esperado ~146/178,
      8 Failures, 24 Timeouts. Anotar desglose (d).

- [ ] 7. (Corrección #3) Formalizar P-2 (no autovalidación, regla R-2) en `experience_validate`
      (`include/experience.h`): hoy solo está en comentarios del `.c`. Añadir como `ensures`:
      `(\exists k; 0<=k<exps_count && exps[k].id==exp_id && \old(exps[k].owner_id)==caller_id)
      ==> \result==ERR_SELF_VALIDATE` (y, si cierra, el dual: en éxito el validador difiere del
      owner, `validator_id==caller_id != owner_id`). NO debilitar el `assigns` acotado por
      set-comprehension ya existente (inmutabilidad R-5/P-5). NO tocar el cuerpo.
      Files: include/experience.h
      Verify: `wsl -d Ubuntu bash verification/_run_one.sh src/experience.c` — el `ensures` de P-2
      cierra o queda documentado como pendiente por framing, respaldado por A2 (autovalidación
      rechazada) en `tests/test_attacks.c`. Registrar (d). Cap: máx. 3 re-ejecuciones.

- [ ] 8. Ejecutar Eva completo (paso (e) del procedimiento) para obtener los números reales actuales
      y detectar smoke-tests de dead-code.
      Files: (ninguno — solo ejecución; actualiza `verification/eva_results.txt` si el script lo vuelca)
      Verify: ejecutar vía script bash (no inline en PowerShell):
      `eval "$(opam env)"; cd /mnt/c/.../EduProyect; frama-c -eva -eva-domains cvalue
      -cpp-extra-args="-Iinclude -Iverification" src/security.c src/audit.c src/professionals.c
      src/experience.c src/credentials.c verification/eva_harness.c -main eva_main`.
      Esperado por el brief: ~91 precondiciones válidas y ~17 assertions, 0 alarmas. **Usar los
      números que arroje ESTA ejecución**, no copiar 91/17 a ciegas; si difieren, usar los reales y
      señalarlo en el documento.

- [ ] 9. Ejecutar la regresión completa (paso (e)) para confirmar cero cambios funcionales.
      Files: (ninguno — solo ejecución)
      Verify: `wsl -d Ubuntu bash run_tests.sh` — build normal + ASan/UBSan, esperado **99/99** en
      ambas builds, sin fallos de sanitizer.

- [ ] 10. (P2 — honestidad documental) Actualizar `docs/formal_verification.md` **añadiendo** una
      nueva entrada de ejecución debajo del historial (sin tocar la sección "LÍNEA BASE
      (NO MODIFICAR)" ni `wp_results_baseline.txt`): (a) regenerar el bloque de resultados de Eva con
      los números reales del ítem 8 (el doc dice 54/10, desactualizado; sustituir por los actuales,
      p. ej. 91 precondiciones / 17 assertions si la ejecución los confirma); (b) anotar los 2
      smoke-tests de dead-code benignos detectados por revisor-verificación (hoy no mencionados);
      (c) registrar los deltas de WP por corrección de los ítems 2–7 (Failure/Timeout/Unknown y qué
      quedó pendiente con su regla de negocio y respaldo dinámico); (d) documentar como mejoras
      futuras (NO implementar) P3 (S-12, usar `event_t.result`, cambio funcional) y P4 (deuda de
      prueba: actor_id real en transiciones, arreglos llenos, log lleno); (e) mantener descritos como
      límites genuinos del probador el framing del struct ~7 KB y la ausencia de `state_inv` global.
      Files: docs/formal_verification.md
      Verify: relectura del documento: la sección LÍNEA BASE intacta, los números de Eva coinciden con
      el ítem 8, P3/P4 están como mejoras futuras, y los límites del probador siguen descritos como
      límite (no como objetivo pendiente por mala spec). `git diff --stat` no debe mostrar cambios en
      `verification/wp_results_baseline.txt`.

- [ ] 11. Verificación final integral: re-ejecutar WP de ambos módulos y la regresión una última vez,
      y confirmar que la línea base sigue intacta.
      Files: (ninguno)
      Verify: `wsl -d Ubuntu bash verification/_run_one.sh src/credentials.c`,
      `wsl -d Ubuntu bash verification/_run_one.sh src/experience.c`,
      `wsl -d Ubuntu bash run_tests.sh` (99/99), y
      `git diff --exit-code verification/wp_results_baseline.txt` (sin cambios → exit 0).
      Ningún objetivo previamente probado debe haber pasado a Failure respecto a la medición inicial.

---

## Digest

Comandos de build/verificación (descubiertos en los scripts del repo; no inventar):
- WP módulo: `wsl -d Ubuntu bash verification/_run_one.sh <archivo.c>`
  (= `frama-c -wp -wp-rte -wp-prover alt-ergo,z3 -wp-timeout 20 -cpp-extra-args="-Iinclude -Iverification" <archivo.c>`)
- WP función: `wsl -d Ubuntu bash verification/_run_fct.sh <funcion> <archivo.c>`
- Eva: `frama-c -eva -eva-domains cvalue -cpp-extra-args="-Iinclude -Iverification" src/security.c src/audit.c src/professionals.c src/experience.c src/credentials.c verification/eva_harness.c -main eva_main` (vía script bash; PowerShell rompe `$(opam env)`)
- Regresión: `wsl -d Ubuntu bash run_tests.sh` (gcc c99 normal + ASan/UBSan)
- Estado hoy (verificado por lectura del baseline/doc): credentials.c 226/239 (9F+4T),
  experience.c 146/178 (8F+24T), Eva 0 alarmas (números de precond/assert a regenerar), regresión 99/99.

### include/credentials.h
- Bloque `assigns` de `credential_issue` (líneas del contrato ~18–21), `credential_activate`
  (~35–36), `credential_block` (~48–49), `credential_reactivate` (~61–62), `credential_revoke`
  (~74–75): hoy un único `assigns` que nombra el slot/estado + `st->log[st->log_count], log_count`.
  Cambio (#5): particionar por `behavior success`/`error`. — Ítem 2.
- `ensures` de transición de `credential_activate`/`block`/`reactivate` (hoy casi idénticos, solo
  result-set + P-1 + `creds_count` sin cambio): añadir P-7 `from→to` con `\old`. Cambio (#2). — Ítem 4.
- `ensures` de P-8 en `credential_revoke` (hoy solo la mitad negativa, el `\exists ... \old(state)
  ==CRED_REVOKED ==> ERR_BAD_TRANSITION`): añadir mitad positiva anclada en `\old`. Cambio (#4). — Ítem 5.
- Test que cubre: `tests/test_attacks.c` A6/A7/A13/A14/A15 (transiciones legales/ilegales, revoke
  terminal) y `tests/test_credentials.c`.
- Verificado hoy: `_run_one.sh src/credentials.c` → 226/239, 9F+4T (reportado); regresión 99/99.

### src/credentials.c
- Contrato de la auxiliar estática `cred_transition` (justo encima de su definición): hoy
  `requires \valid(st)` + contadores, `assigns {creds[k].state | ...}, log[...], log_count`,
  result-set, `creds_count == \old(creds_count)`. **No** menciona `from`/`to` ni roles.
  Cambio (#1): añadir `ensures` de P-7 (`\old(state)==from`/`state==to` en éxito) y P-1
  (`!security_can(caller_role,op_code) ==> ERR_PERM`); y (#5) particionar su `assigns` por behavior.
  El cuerpo NO cambia. — Ítems 2 y 3.
- Wrappers `credential_activate/block/reactivate` (cuerpos de 1 línea que delegan en
  `cred_transition`): no se tocan; sus `ensures` viven en el `.h`.
- `credential_revoke` (cuerpo propio, sin pasar por `cred_transition`): cuerpo NO cambia; su `ensures`
  P-8 vive en el `.h`.
- Test que cubre: `tests/test_credentials.c`, `tests/test_attacks.c` (A6/A7/A13–A15).
- Verificado hoy: mismo `_run_one.sh` arriba.

### include/experience.h
- Contrato de `experience_validate`: hoy result-set, P-1 por rol, `exps_count` sin cambio, y un bloque
  de comentario que dice que P-2/efecto quedaron pendientes. El `assigns` ya está acotado por
  set-comprehension (inmutabilidad R-5/P-5) — **no debilitar**.
  Cambio (#3): añadir `ensures` de P-2 (no autovalidación) con `\exists ... owner_id==caller_id
  ==> ERR_SELF_VALIDATE`, anclado en `\old`. — Ítem 7.
- Test que cubre: `tests/test_attacks.c` A1/A2/A3 (escalada, autovalidación, ya-validada) y
  `tests/test_experience.c`.
- Verificado hoy: `_run_one.sh src/experience.c` → 146/178, 8F+24T (reportado); regresión 99/99.

### src/experience.c
- `experience_validate` (cuerpo): ya rechaza autovalidación (`exps[idx].owner_id == caller_id ==>
  ERR_SELF_VALIDATE`) y ya-validada (`ERR_ALREADY_VALID`). El cuerpo NO cambia; solo se formaliza en
  el `.h` lo que ya hace el código. — Ítem 7.
- Test que cubre: `tests/test_attacks.c` (A1/A2/A3), `tests/test_experience.c`.

### docs/formal_verification.md
- Sección "LÍNEA BASE — Ejecución 2026-11-18 (NO MODIFICAR)": **intacta**.
- Bloque "Ejecución Eva 2026-11-18" (hoy: `Assertions 2 valid / 8 unknown / 10 total`,
  `Preconditions 54 valid / 54 total`): regenerar con los números reales del ítem 8 (esperado
  ~91 precond / ~17 assertions). Añadir nueva entrada de ejecución al final del historial con los
  deltas de WP por corrección, los 2 smoke-tests de dead-code, P3/P4 como mejoras futuras, y los
  límites del probador. — Ítem 10.
- Verificado: `git diff --exit-code verification/wp_results_baseline.txt` debe dar exit 0.

### verification/wp_results_baseline.txt
- INTACTO. No se edita. Verificación: `git diff --exit-code` → sin cambios.
