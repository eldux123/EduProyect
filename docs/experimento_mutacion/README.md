# Experimento de mutación — verificación formal de EduChain (Frama-C WP)

Demostración reproducible **antes/después** de que la verificación formal de EduChain
(Frama-C WP) **detecta** una violación deliberada de una propiedad de seguridad.

> **Qué demuestra y qué NO.** Este experimento demuestra que WP **detecta una violación
> concreta** de una propiedad de autorización en `security_can`. **NO** demuestra que todo
> EduChain sea correcto: solo cubre la obligación mutada en un módulo. Una prueba de mutación
> exitosa valida que el verificador "no aprueba todo", nada más.

## Propiedad bajo prueba

Un rol sin permiso no puede ejecutar una operación protegida. En concreto:
**`ROLE_VERIFIER` (rol de solo lectura) NO está autorizado para `OP_VALIDATE_EXP`**
(ni para ninguna operación de escritura). Detalle técnico en [`propiedad.md`](propiedad.md).

## Patrón antes/después

| Fase | Código | Resultado WP esperado |
|------|--------|-----------------------|
| A | `security.c` original | `Proved goals: 114 / 114` |
| B | mutación M1 (1 línea en C; contrato ACSL intacto) | — |
| C | `security.c` mutado | **`[Failure]`** con Z3 (refutación real) |
| D | `security.c` original restaurado | `Proved goals: 114 / 114` |

## El prover decisivo: Z3 da Failure, Alt-Ergo da Timeout

Hallazgo clave de este experimento. Sobre la MISMA obligación violada:

- `-wp-prover alt-ergo,z3` → **`[Timeout]`**. Alt-Ergo no decide dentro del límite de tiempo.
  **Un Timeout NO prueba detección**: no distingue "la propiedad es falsa" de "el probador
  es lento". Es un resultado *no concluyente*.
- `-wp-prover z3` → **`[Failure]`**. Z3 **refuta** la obligación (existe un modelo/contraejemplo
  que la viola). **Este es el resultado concluyente** que prueba que WP detectó la mutación.

Por eso el experimento corre WP **dos veces en FASE C** y registra ambas salidas: la de
`alt-ergo,z3` (Timeout, para mostrar el contraste) y la de `z3` (Failure, la conclusión).

## Requisitos de entorno

- WSL2 distro **Ubuntu** (no `docker-desktop`).
- Frama-C 33.0 (Arsenic) vía opam; `frama-c` NO está en PATH hasta `eval "$(opam env)"`.
- Provers: Alt-Ergo 2.4.3, Z3 4.13.0 (detectado por why3).

## Cómo ejecutarlo

```bash
wsl -d Ubuntu bash docs/experimento_mutacion/run_experimento.sh
```

El script trabaja **exclusivamente** en una copia temporal aislada en `/tmp/educhain_mutacion/`
y **nunca escribe en el repositorio**. Usa `-wp-cache none` para no tocar la caché WP
versionada en `.frama-c/wp/cache/` (el repo no tiene `.gitignore`). Al terminar imprime las
tablas de resultados; las salidas completas quedan en `/tmp/educhain_mutacion/salidas/`.

Instrucciones detalladas en [`instrucciones.md`](instrucciones.md).

## Archivos de este directorio

| Archivo | Contenido |
|---------|-----------|
| `run_experimento.sh` | Script reproducible A→B→C→D (entregable 1). |
| `instrucciones.md` | Instrucciones de ejecución (entregable 2). |
| `comparacion.md` | Comparación antes/después con salidas reales (entregable 3). |
| `salidas_wp/` | Salidas WP completas de FASE A y FASE C, más la D y el diff (entregable 4). |
| `propiedad.md` | Explicación técnica de la propiedad seleccionada (entregable 5). |
| `no_modificacion.md` | Evidencia de no-modificación del repo + versiones (entregable 6). |
