# Comparación antes / después

Resultados **reales** de las corridas de Frama-C WP ejecutadas por
[`run_experimento.sh`](run_experimento.sh). Las salidas completas están en
[`salidas_wp/`](salidas_wp/).

- Frama-C 33.0 (Arsenic) · Alt-Ergo 2.4.3 · Z3 4.13.0 (vía why3)
- Función: `security_can` en `src/security.c`
- Comando base: `frama-c -wp -wp-rte -wp-prover <P> -wp-cache none -wp-timeout 20 -wp-fct security_can -cpp-extra-args="-Iinclude -Iverification" src/security.c`

## Tabla resumen

| Fase | Código | Prover | Resultado WP | Lectura |
|------|--------|--------|--------------|---------|
| A | original | `alt-ergo,z3` | `Proved goals: 114 / 114` (0 Timeout, 0 Failed) | Propiedad demostrada |
| C.1 | mutado (M1) | `alt-ergo,z3` | `112 / 115`, **3 Timeout** (`ensures_4/8/9`) | **No concluyente** |
| C.2 | mutado (M1) | `z3` | `104 / 115`, **11 Failed** (`ensures_4/8/9`) | **Detección (refutación)** |
| D | original (restaurado) | `alt-ergo,z3` | `Proved goals: 114 / 114` | Estado inicial recuperado |

## FASE A — estado original (demostrado)

```
[wp] Proved goals:  114 / 114
  Qed:           103
  Alt-Ergo :       9
```

Las 114 obligaciones de `security_can` se demuestran: la implementación satisface su contrato
ACSL, incluidas las cláusulas de autorización por rol. (Fuente: `salidas_wp/faseA_wp.txt`.)

## FASE B — mutación M1

Un cambio de una sola línea en `src/security.c`, **sin tocar el contrato ACSL**:

```diff
     case OP_VALIDATE_EXP:
-        return (caller_role == ROLE_HR || caller_role == ROLE_COORD) ? 1 : 0;
+        return (caller_role == ROLE_HR || caller_role == ROLE_COORD || caller_role == ROLE_VERIFIER) ? 1 : 0;
```

Ahora `security_can(ROLE_VERIFIER, OP_VALIDATE_EXP)` devuelve `1`, pero el contrato sigue
exigiendo `0` para un `VERIFIER`. (Fuente: `salidas_wp/mutacion_M1.diff`.)

## FASE C — detección

### C.1 — `alt-ergo,z3` → Timeout (contraste, NO prueba detección)

```
[wp] [Timeout] typed_security_can_ensures_4_part7 (Alt-Ergo)
[wp] [Timeout] typed_security_can_ensures_8_part7 (Alt-Ergo)
[wp] [Timeout] typed_security_can_ensures_9_part7 (Alt-Ergo)
[wp] Proved goals:  112 / 115
  Timeout:         3
```

Alt-Ergo agota el tiempo sobre las tres obligaciones afectadas. **Un `Timeout` NO demuestra
detección**: significa solo que el probador no decidió dentro del límite; no distingue entre
"la propiedad es falsa" y "el probador es demasiado lento". Si el experimento se quedara aquí,
la conclusión sería *no concluyente*. (Fuente: `salidas_wp/faseC_wp_alt-ergo.txt`.)

### C.2 — `z3` → Failure (resultado CONCLUYENTE: detección)

```
[wp] [Failure] typed_security_can_ensures_4_part7 (Z3)   <- bicondicional OP_VALIDATE_EXP
[wp] [Failure] typed_security_can_ensures_8_part7 (Z3)   <- (VERIFIER ==> \result == 0)
[wp] [Failure] typed_security_can_ensures_9_part1..9 (Z3) <- equivalencia con role_can
[wp] Proved goals:  104 / 115
  Failed:         11
```

Z3 **refuta** las obligaciones: existe un modelo (`caller_role == ROLE_VERIFIER`,
`op_code == OP_VALIDATE_EXP`) en el que la implementación mutada contradice el contrato.
`[Failure]` es un resultado **negativo decisivo** (no "no sé"): WP ha **detectado** que la
implementación ya no cumple la propiedad. Las 11 metas fallidas corresponden a las cláusulas
`ensures #4` (bicondicional de `OP_VALIDATE_EXP`), `ensures #8` (regla del `VERIFIER`) y
`ensures #9` (equivalencia con `role_can`, partida por Qed en sub-metas `part1..part9`).
(Fuente: `salidas_wp/faseC_wp_z3.txt`.)

## FASE D — restauración

```
[wp] Proved goals:  114 / 114
```

Tras descartar la copia mutada, el `security.c` original vuelve a demostrarse `114 / 114`,
idéntico a FASE A. (Fuente: `salidas_wp/faseD_wp.txt`.)

## Distinción Failure / Timeout / no-demostrado

| Resultado WP | Significado | ¿Prueba detección? |
|--------------|-------------|--------------------|
| `[Failure]` | El prover encontró un contraejemplo: la obligación es **falsa**. | **Sí** (negativo decisivo). |
| `[Timeout]` | El prover no decidió dentro del límite de tiempo. | **No** (no concluyente). |
| meta no demostrada (`Unknown`) | El prover no pudo probarla ni refutarla. | **No**. |

El resultado que sustenta la conclusión de este experimento es el **`[Failure]` de Z3** en
FASE C.2, no el Timeout de Alt-Ergo.

## Alcance (qué NO se demuestra)

El experimento demuestra que **WP detecta esta violación concreta** de la autorización por rol.
**No** demuestra que todo EduChain sea correcto: solo cubre la obligación mutada en `security_can`.
Una prueba de mutación exitosa confirma que el verificador "no aprueba todo"; no es un certificado
de corrección global.

## Mutación de respaldo (M2)

No fue necesaria: M1 produjo el `[Failure]` esperado. Si en otro entorno M1 no reprodujera el
Failure, la alternativa verificada durante el diseño es **M2**: en los casos `OP_ISSUE_CRED` /
`OP_ACTIVATE_CRED` sustituir `return (caller_role == ROLE_COORD) ? 1 : 0;` por `return 1;`
(cualquier rol queda autorizado a emitir/activar credenciales). Con Z3 también da `[Failure]`
(13 metas falladas, incluidas `ensures_5` y `ensures_9`).
