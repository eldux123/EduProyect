# Evidencia de no-modificación del repositorio original

El experimento trabaja **exclusivamente** en una copia aislada en `/tmp/educhain_mutacion/` y
**no modifica ningún archivo preexistente** del repositorio. Lo único que se agrega al repo es
este directorio nuevo `docs/experimento_mutacion/` con los entregables.

## Versiones de herramientas registradas

| Herramienta | Versión |
|-------------|---------|
| Frama-C | 33.0 (Arsenic) |
| Alt-Ergo | 2.4.3-free |
| Z3 | 4.13.0 (detectado por why3; ver `verification/env_captured.txt`) |

Rama git: `fase4-acsl`.

Las versiones **regeneradas durante esta ejecución** (en la copia aislada, no en el repo) se
conservan en [`salidas_wp/env_capturado.txt`](salidas_wp/env_capturado.txt): Frama-C
33.0 (Arsenic), Alt-Ergo 2.4.3-free, y Z3 4.13.0 listado por `why3 config list-provers`
(el prover decisivo de FASE C.2).

## `git status` de archivos preexistentes (debe estar limpio)

Comando y salida reales tras ejecutar el experimento completo (A→B→C→D):

```
$ git status --porcelain src/ include/ verification/ .frama-c/ tests/ main.c \
    run_tests.sh run_demo.sh docs/architecture.md docs/explicacionv1.md \
    docs/formal_verification.md docs/phase1_model.md docs/diseno_flujo_solicitudes.md
?? docs/diseno_flujo_solicitudes.md
```

Interpretación:

- **Sin cambios** en todo el código (`src/`), contratos y cabeceras (`include/`), artefactos de
  verificación (`verification/`, incluida la **línea base** `wp_results_baseline.txt` y los
  resultados históricos WP/Eva), la caché WP versionada (`.frama-c/`), las pruebas (`tests/`),
  la CLI (`main.c`) y los documentos existentes (`docs/architecture.md`, `explicacionv1.md`,
  `formal_verification.md`, `phase1_model.md`).
- La **única** entrada listada, `?? docs/diseno_flujo_solicitudes.md`, es un archivo **sin
  seguimiento (untracked)** que **ya estaba en ese estado antes de iniciar el experimento**.
  Este trabajo **no lo creó, no lo modificó y no lo abrió para escritura**: solo leyó documentos
  vecinos. Un archivo untracked no tiene cambios respecto al índice porque git no lo rastrea; su
  presencia es ortogonal al experimento.

## Caché WP versionada (`.frama-c/wp/cache/`)

```
$ git status --porcelain .frama-c/ | wc -l
0
```

**0 archivos con cambios.** El repo no tiene `.gitignore` y `.frama-c/wp/cache/` está versionado
(~962 archivos). Para no ensuciarlo, el experimento ejecuta WP con **`-wp-cache none`**, de modo
que no escribe ni actualiza entradas de caché dentro del repo. (Durante la fase de diseño, una
corrida de diagnóstico con caché por defecto llegó a escribir archivos de caché; se restauraron
y eliminaron por completo antes de la ejecución, dejando este conteo en 0.)

## Único delta que el experimento aporta

```
$ git status --porcelain docs/experimento_mutacion/
?? docs/experimento_mutacion/
```

Directorio **nuevo** con los seis entregables (script, instrucciones, comparación, salidas WP,
explicación de la propiedad y esta evidencia). No toca nada preexistente.

## FASE D — confirmación de restauración

La FASE D del script vuelve a verificar el `security.c` original y obtiene de nuevo
`Proved goals: 114 / 114` (ver `salidas_wp/faseD_wp.txt`), idéntico a FASE A. La copia mutada en
`/tmp` se descarta (`rm -rf`) al final de la ejecución. El estado del repositorio y su línea base
quedan intactos.
