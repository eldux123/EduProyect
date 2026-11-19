# Revisión consolidada — EduChain fase4-acsl

**Veredicto de puerta: APPROVED**

Diff revisado: `a710c43` (v2) … `HEAD` (`375be25`), rama `fase4-acsl`, 4 commits sobre la base.
Alcance: cinco correcciones ACSL de Prioridad 1 (P1) + regeneración del bloque Eva (P2) en `include/credentials.h`, `src/credentials.c`, `include/experience.h`, `src/experience.c`, `docs/formal_verification.md`, más `verification/specifications.h` e `include/security.h` tocados de forma justificada bajo `#ifdef __FRAMAC__`.

## Resumen de veredictos por revisor

| Revisor | Especialidad | Veredicto | Findings bloqueantes |
|---|---|---|---|
| revisor-c | Seguridad de memoria / código C | APPROVED | Ninguno |
| revisor-seguridad | Propiedades P-1, P-2/R-2, P-7, P-8 | APPROVED | Ninguno |
| revisor-acsl | Corrección de especificación ACSL | APPROVED | Ninguno |
| revisor-verificacion | Honestidad de verificación formal | APPROVED | Ninguno |

Los cuatro coinciden en hechos clave: todo el diff vive en comentarios ACSL `/*@ ... */`, includes guardados por `#ifdef __FRAMAC__` y `docs/`; los cuerpos de función C son idénticos a v2 (coherente con regresión 99/99); ningún contrato se debilitó; no hay `assumes` injustificados; la línea base (`wp_results_baseline.txt` y la sección "LÍNEA BASE — NO MODIFICAR") permanece intacta; y la reclasificación de `cred_transition_ensures_3` de "Proved" a Timeout resolvió el bloqueante de la ronda previa alineando documento y artefacto versionado.

## Findings consolidados (ninguno bloqueante)

### Menores (legibilidad / documentación)

1. **[Menor · revisor-c]** `docs/formal_verification.md`, tabla "WP por módulo — después", fila `credentials.c`: reporta `298/315 (12F+5T)` frente a `299/315 (12F+4T)` citado en un mensaje de commit intermedio de BLOQUEANTE-5. El estado final reconcilia tabla y artefacto (298/315, Timeout); la discrepancia con la prosa de un commit intermedio es histórica, no un defecto del estado final.

2. **[Menor · revisor-c]** `docs/formal_verification.md`, entrada `audit.c`: mezcla "98/98" y "95/95" de dos corridas en el mismo bloque. Preexistente al alcance de esta fase; solo legibilidad.

3. **[Menor · revisor-acsl]** `docs/formal_verification.md`, tabla "Antes/Después": el "Antes" (226/239, 146/178) proviene de `wp_results.txt`, configuración distinta del baseline versionado (52/86, 75/106). El documento lo advierte ("no comparar porcentajes entre configuraciones"); la atribución de deltas por corrección solo es parcialmente verificable desde los artefactos. No afecta la corrección de la spec.

4. **[Menor · revisor-verificacion]** Discrepancia cosmética "175 goals scheduled" vs total 179 en el artefacto de experience, manejada correctamente por el documento.

### Informativos (postura de verificación, sin acción)

5. **[Info · revisor-seguridad / revisor-acsl / revisor-verificacion]** Postcondiciones de seguridad formalizadas pero NO demostradas en WP: P-7 (`cred_transition_ensures_4/5`, `*_ensures_2` de wrappers), P-2 (`experience_validate_ensures_4`), P-8 (`credential_revoke_ensures_4/5`) quedan en Failure/Timeout por límites genuinos del probador (framing del struct ~7 KB vía `audit_record_event`; ausencia de `state_inv` global de unicidad de id). El respaldo es dinámico (A2, A6, A7, A13, A15) + regresión 99/99. Reportado con honestidad; cumple la regla "no forzar 100% / documentar lo pendiente". Las propiedades están especificadas correctamente pero son candidatas no demostradas formalmente.

6. **[Info · revisor-seguridad]** `include/security.h` y `verification/specifications.h` tocados fuera de la lista literal de "archivos previstos". Justificado: `role_can` es necesario porque `cred_transition` recibe `op_code` y ACSL no invoca `security_can`; va bajo `#ifdef __FRAMAC__`, es fortalecimiento (security 105→114/114) y está documentado. No afecta el build gcc.

7. **[Info · revisor-seguridad]** `role_can` con cláusula redundante `(r == ROLE_VERIFIER ==> \false)`, ya implicada por las cláusulas por operación. Benigna, consistente con `security_can`.

8. **[Info · revisor-c / revisor-seguridad]** Los dos smoke-tests de dead-code sobre las guardas `if (st == 0)` son consecuencia esperada de combinar guarda defensiva con `\valid`/`\valid_read`; correctamente anotados en el doc, no son bug.

## Verificación de reglas inquebrantables del brief

- **No debilitar contratos:** confirmado por los cuatro revisores; las cinco correcciones añaden o fortalecen `ensures`/behaviors; ninguna relaja una postcondición previa.
- **No forzar 100%:** las obligaciones no cerradas quedan clasificadas honestamente (Failure/Timeout) con causa y respaldo dinámico.
- **Sin `assumes` injustificados:** los únicos `assumes` particionan por `log_count < MAX_EVENTS` vs `>= MAX_EVENTS` (framing), no presuponen la propiedad a demostrar.
- **Baseline intacta:** `wp_results_baseline.txt` byte-idéntico (diff exit 0) y la sección "LÍNEA BASE (NO MODIFICAR)" sin tocar.
- **Honestidad documental (ex-BLOQUEANTE-1 / P2):** `cred_transition_ensures_3` reclasificado a Timeout, coherente con `wp_results_fase4_credentials.txt`; Eva reporta 91 precond/17 assertions reales, 0 alarmas RTE; Z3 declarado configurado pero sin cerrar obligaciones.

## Conclusión

Ningún revisor reporta findings bloqueantes ni pide CHANGES_REQUESTED. Las cuatro propiedades de seguridad (P-1, P-2, P-7, P-8) siguen garantizadas a nivel de especificación; la máquina de estados, `caller_id` y la línea base permanecen intactos; la documentación es honesta sobre lo demostrado frente a lo pendiente. **Veredicto de puerta: APPROVED.**
