# Instrucciones de ejecución

## Requisitos previos

- **WSL2** con la distro **Ubuntu** (no `docker-desktop`).
- **Frama-C 33.0 (Arsenic)** instalado vía opam, con los provers **Alt-Ergo 2.4.3** y
  **Z3 4.13.0** (detectado por why3). `frama-c` no está en `PATH` hasta ejecutar
  `eval "$(opam env)"`; el script lo hace automáticamente si hace falta.

Comprobación rápida del entorno:

```bash
wsl -d Ubuntu bash -lc 'eval "$(opam env)"; frama-c -version; alt-ergo --version'
```

## Ejecución

Desde la raíz del repositorio (`EduProyect/`):

```bash
wsl -d Ubuntu bash docs/experimento_mutacion/run_experimento.sh
```

El script:

1. Crea una copia aislada en `/tmp/educhain_mutacion/` (subcarpetas `original/` y `mutado/`)
   con solo los archivos necesarios para verificar `security.c`.
2. **FASE A** — verifica el `security.c` original → `Proved goals: 114 / 114`.
3. **FASE B** — aplica la mutación M1 (una línea) sobre la copia `mutado/`; genera el diff.
4. **FASE C** — verifica el `security.c` mutado dos veces:
   - con `alt-ergo,z3` → `[Timeout]` (contraste, **no** concluyente);
   - con `z3` → `[Failure]` (**detección** concluyente).
5. **FASE D** — vuelve a verificar el original → `Proved goals: 114 / 114`; descarta la copia
   mutada; imprime `git status` de los archivos preexistentes (debe salir vacío).

## Salidas

Las salidas completas de WP quedan en `/tmp/educhain_mutacion/salidas/`:

| Archivo | Fase |
|---------|------|
| `faseA_wp.txt` | A (original, 114/114) |
| `mutacion_M1.diff` | B (diff de la mutación) |
| `faseC_wp_alt-ergo.txt` | C.1 (Timeout, contraste) |
| `faseC_wp_z3.txt` | C.2 (Failure, detección) |
| `faseD_wp.txt` | D (original restaurado, 114/114) |
| `resumen.txt` | resumen agregado + evidencia git |
| `env_capturado.txt` | versiones de Frama-C/Alt-Ergo/Z3 regeneradas en la corrida |

En este directorio del repo se conserva una **copia congelada** de esas salidas en
[`salidas_wp/`](salidas_wp/) para consulta sin re-ejecutar.

## Notas importantes

- El script usa **`-wp-cache none`**. El repo no tiene `.gitignore` y la caché de WP
  (`.frama-c/wp/cache/`) está versionada; sin `-wp-cache none`, WP escribiría archivos nuevos
  dentro del repo. No lo cambies.
- El script **nunca escribe en el repositorio**; todo el trabajo ocurre en `/tmp`.
- Si re-ejecutas, `/tmp/educhain_mutacion/` se borra y se regenera (`rm -rf` al inicio).
- Para aislar una única obligación puedes añadir, p. ej., `-wp-prop` al comando `frama-c`
  dentro del script, pero no es necesario para el experimento.
