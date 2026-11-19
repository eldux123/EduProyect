# Revision de honestidad y rigor - Experimento de mutacion (Frama-C WP)

**Revisor:** Revisor de Verificacion Formal (Frama-C WP/Eva).
**Alcance de la revision:** honestidad y rigor de los entregables en
`docs/experimento_mutacion/`. Solo reviso y reporto; no modifico archivos del proyecto.
**Veredicto:** **APROBADO** (APPROVED). Los seis puntos exigidos se cumplen y la conclusion
(deteccion por `[Failure]` de Z3, no por Timeout) esta respaldada por salida WP real que
**reproduje en vivo** en este entorno.

---

## 1. Reproduccion independiente (ejecucion en vivo)

Frama-C SI esta instalado en este entorno (WSL2 Ubuntu). Ejecute el script entregado:

    wsl -d Ubuntu bash docs/experimento_mutacion/run_experimento.sh

Resultado de MI corrida (2026-11-19 13:20 UTC), identico a los artefactos congelados:

| Fase | Prover | Resultado reproducido | Coincide con artefacto |
|------|--------|-----------------------|------------------------|
| A (original)   | alt-ergo,z3 | `Proved goals: 114 / 114` | Si (faseA_wp.txt) |
| C.1 (mutado)   | alt-ergo,z3 | `112 / 115`, **3 [Timeout]** (ensures_4/8/9_part7) | Si (faseC_wp_alt-ergo.txt) |
| C.2 (mutado)   | z3          | `104 / 115`, **11 [Failure]** (incl. ensures_8_part7 (Z3)) | Si (faseC_wp_z3.txt) |
| D (restaurado) | alt-ergo,z3 | `Proved goals: 114 / 114` | Si (faseD_wp.txt) |

Tras la corrida: `git status --porcelain` de preexistentes = vacio; la copia mutada
`/tmp/educhain_mutacion/mutado` quedo descartada. El experimento es reproducible y
deterministico en este entorno.

---

## 2. Tabla por obligacion: Afirmado vs. Evidencia real vs. Veredicto

| Obligacion WP | Afirmado | Evidencia real (salida WP) | Veredicto |
|---------------|----------|----------------------------|-----------|
| security_can (114 metas), FASE A | Propiedad demostrada 114/114 | `[wp] Proved goals: 114 / 114` (Qed 103, Alt-Ergo 9) en faseA_wp.txt; reproducido | **Demostrado** |
| typed_security_can_ensures_8_part7 (VERIFIER ==> result==0), FASE C.2 | refutada / detectada | `[wp] [Failure] typed_security_can_ensures_8_part7 (Z3)` en faseC_wp_z3.txt; reproducido | **Failure (deteccion concluyente)** |
| typed_security_can_ensures_4_part7 (bicond. OP_VALIDATE_EXP), FASE C.2 | refutada | `[wp] [Failure] typed_security_can_ensures_4_part7 (Z3)` | **Failure (deteccion)** |
| typed_security_can_ensures_9_part1..9 (equiv. role_can), FASE C.2 | refutada (9 sub-metas) | 9 lineas `[Failure] ...ensures_9_partN (Z3)` | **Failure (deteccion)** |
| Mismas 3 obligaciones, FASE C.1 (alt-ergo,z3) | Timeout, NO concluyente | `[Timeout] ensures_4/8/9_part7 (Alt-Ergo)`, `Timeout: 3` | **Timeout (NO prueba deteccion)** |
| security_can 114 metas, FASE D | estado inicial recuperado | `[wp] Proved goals: 114 / 114` | **Demostrado** |

La **misma** obligacion (ensures_8_part7) pasa de estar incluida en las 114 demostradas
(FASE A) a `[Failure]` tras la mutacion (FASE C.2): el antes/despues es sobre el mismo objetivo.

---

## 3. Verificacion punto por punto de lo exigido

### (1) Evidencia real A->C y distincion Failure/Timeout/no-demostrado -> CUMPLE
- FASE A: obligacion cerrada (`Proved goals: 114/114`), salida completa presente y reproducida.
- FASE C.2: la misma obligacion pasa a `[Failure]` con contraejemplo (Z3 refuta). Salida real presente y reproducida.
- El resultado que sustenta la conclusion es el **[Failure]**, NO el Timeout. Los documentos
  (comparacion.md, README.md, propiedad.md) incluyen una tabla explicita que separa
  **Failure** (negativo decisivo, prueba deteccion) de **Timeout** (no concluyente) y de
  **Unknown/no-demostrado** (no concluyente). El Timeout de Alt-Ergo se presenta como contraste,
  nunca como exito.

### (2) No se inventan resultados -> CUMPLE
Cada cifra de los .md cita su artefacto fuente (salidas_wp/*.txt). Compare las citas con los
archivos y con mi corrida en vivo: coinciden. No hay afirmacion de deteccion sin la salida que la prueba.

### (3) Alcance honesto (detecta UNA violacion, NO prueba todo EduChain) -> CUMPLE
README.md (bloque destacado) y comparacion.md (seccion Alcance) declaran explicitamente que
se demuestra la deteccion de una violacion concreta en security_can y que **NO** se demuestra la
correccion global de EduChain. propiedad.md mantiene la misma delimitacion.

### (4) Versiones de Frama-C/WP/solvers -> CUMPLE
env_capturado.txt y no_modificacion.md: Frama-C 33.0 (Arsenic), Alt-Ergo 2.4.3-free,
Z3 4.13.0 (listado por why3 config list-provers). WP es el plugin de Frama-C 33.0. Confirmado
en mi reproduccion.

### (5) Repo original y linea base intactos -> CUMPLE
`git status --porcelain` de src/ include/ verification/ .frama-c/ tests/ main.c y docs
preexistentes: **sin cambios**. El commit 268cc7c solo agrega 13 archivos nuevos bajo
docs/experimento_mutacion/ (692 inserciones, 0 en archivos existentes). src/security.c y el
contrato ACSL de include/security.h coinciden exactamente con lo citado en los documentos
(incluida la clausula VERIFIER ==> result == 0). La unica entrada untracked,
docs/diseno_flujo_solicitudes.md, es preexistente y ajena al experimento.
La linea base verification/wp_results_baseline.txt y los historicos WP/Eva no cambiaron.

### (6) Aislamiento en /tmp y reproducibilidad -> CUMPLE
El script trabaja solo en /tmp/educhain_mutacion/ (original/ y mutado/), usa -wp-cache none
para no tocar la cache versionada, y descarta la copia mutada al final. Verifique que mutado/
no existe tras la corrida. Reproducible via el script, como confirme ejecutandolo.

---

## 4. Afirmaciones a corregir

Ninguna bloqueante. Dos observaciones menores (opcionales, no condicionan la aprobacion):

1. **Precision terminologica en comparacion.md.** El bloque C.2 rotula ensures_9_part1..9 con
   el comentario que sugiere que Qed las parte. En la salida real las sub-metas part1..part9 las
   **refuta Z3** (aparecen como `[Failure] ... (Z3)`); el `(Qed Nms)` que antecede es el
   preprocesado Qed del objetivo antes de pasar al prover. Redaccion honesta sugerida:
   ensures #9 (equivalencia con role_can), descompuesta en 9 sub-metas part1..part9, todas
   refutadas por Z3. Es matiz de redaccion; la evidencia y la conclusion no cambian.

2. **Distro WSL.** instrucciones.md/README.md indican distro Ubuntu (no docker-desktop). En este
   equipo la distro util es Ubuntu (coincide); docker-desktop existe pero esta detenida; sin
   incidencia. La reproduccion se hizo con wsl -d Ubuntu.

---

## 5. Conclusion

El experimento es **honesto y riguroso**:
- La deteccion se apoya en un `[Failure]` real de Z3 sobre la misma obligacion que FASE A
  demuestra, reproducido en vivo; el Timeout de Alt-Ergo se presenta correctamente como no concluyente.
- No hay resultados inventados ni confusion entre pruebas unitarias y verificacion formal.
- El alcance se declara con precision (detecta una violacion concreta; no certifica todo EduChain).
- Las versiones de herramientas estan registradas.
- El repositorio original, su linea base y los historicos no fueron modificados; solo se anadieron los entregables.
- El trabajo corrio aislado en /tmp y es reproducible por el script.

**Veredicto: APROBADO.** Las dos observaciones son mejoras opcionales de redaccion, no condiciones.