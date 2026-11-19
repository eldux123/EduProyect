---
marp: true
title: EduChain — Verificación formal de un contrato inteligente en C
paginate: true
---

# EduChain

### Simulación de un contrato inteligente en **C**, verificada formalmente con **Frama-C**

Proyecto académico · EduCorp (ficticio)

*Registro de profesionales, experiencia y credenciales con propiedades de seguridad demostradas.*

---

## Qué es y qué NO es

**Es:**
- Un núcleo en C que modela la *lógica* de un contrato inteligente.
- Funciones deterministas + máquina de estados + log append-only.
- Verificado con Frama-C (WP + Eva) y ACSL.

**NO es:**
- Una blockchain real · no despliega en ninguna red.
- No usa criptografía ni datos reales de personas/instituciones.

> "Contrato inteligente" aquí = metáfora de diseño, no tecnología blockchain.

---

## El problema

Un sistema que gestiona credenciales y experiencia laboral debe **impedir abusos**:

- Que alguien se acredite experiencia a sí mismo.
- Que un usuario sin permiso valide o emita credenciales.
- Que una credencial revocada vuelva a usarse.
- Que se dupliquen o alteren registros históricos.

**Objetivo:** no solo *programar* estas reglas, sino **demostrar matemáticamente** que se cumplen.

---

## Enfoque: 3 niveles de garantía

| Nivel | Herramienta | Qué da |
|-------|-------------|--------|
| **Demostración** | Frama-C **WP** | Prueba deductiva para *todas* las entradas |
| **Análisis** | Frama-C **Eva** | Ausencia de errores de memoria en las trazas |
| **Observación** | Tests + ASan/UBSan | Comprobación en las entradas ejecutadas |

> La clave del proyecto: **distinguir siempre** qué está *demostrado* de qué está solo *probado*.

---

## El modelo: actores y permisos

5 roles, mínimo privilegio, separación de responsabilidades:

| Rol | Puede |
|-----|-------|
| ADMIN | gestionar, bloquear/revocar |
| HR | registrar profesionales y experiencia |
| COORD | validar, emitir/activar credenciales |
| PRO | registrar lo propio, consultar |
| VERIFIER | solo lectura |

**Regla transversal:** nadie valida su propia experiencia (sea cual sea su rol).
La autorización se decide en **un único punto** (`security_can`).

---

## El modelo: máquina de estados de la credencial

```
   issue        activate         block
 (∅) --> PENDING --> ACTIVE --> BLOCKED
             |          |          |
           revoke     revoke    reactivate / revoke
             v          v          v
          REVOKED    REVOKED     ACTIVE / REVOKED
```

- **Válida para uso == solo ACTIVE.**
- **REVOKED es terminal:** ninguna operación la saca de ahí.
- `revoke` permitido desde PENDING, ACTIVE o BLOCKED.

---

## Demo en vivo

```bash
wsl -d Ubuntu bash run_demo.sh
```

Los códigos de retorno muestran la seguridad en acción:

| Salida | Significado |
|--------|-------------|
| `register (PRO): -2` | un no autorizado no registra (ERR_PERM) |
| `self-validate: -3` | nadie valida su propia experiencia |
| `fechas invertidas: -6` | fecha inválida rechazada |
| `reactivate REVOKED: -9` | credencial revocada no revive |

---

## Pruebas: 241/241

```bash
wsl -d Ubuntu bash run_tests.sh
```

- **241 pruebas, 0 fallos**, en build normal **y** bajo AddressSanitizer + UndefinedBehaviorSanitizer.
- Incluye **18 pruebas adversariales (A1–A18)**: escalada de privilegios, autovalidación,
  transiciones ilegales, duplicados, arreglos llenos, credenciales revocadas...

> Las pruebas *observan*: comprueban los casos ejecutados. No demuestran todos los casos.

---

## Verificación formal: WP

```bash
frama-c -wp -wp-rte -wp-prover alt-ergo,z3 -wp-timeout 20 ...
```

| Módulo | Probados | Total | Pendientes |
|--------|---------:|------:|:----------:|
| security.c | **114** | 114 | 0 |
| audit.c | **98** | 98 | 0 |
| professionals.c | 78 | 81 | 3 |
| experience.c | 146 | 179 | 33 |
| credentials.c | 362 | 382 | 20 |
| **TOTAL** | **798** | **854** | **56** |

**798 obligaciones demostradas.** Dos módulos al 100%.

---

## Verificación formal: Eva

```bash
frama-c -eva -eva-domains cvalue ...
```

- **23/23 funciones analizadas** (100%).
- **0 alarmas**: sin accesos fuera de límites, overflow, no-init ni división por cero
  en las trazas exploradas.
- **Precondiciones válidas**, 0 inválidas.

> Honesto: 81% de sentencias alcanzadas. "0 alarmas" = sin errores *en lo explorado*,
> no una garantía total.

---

## Lo que NO se pudo demostrar (y por qué)

**56 obligaciones pendientes — clasificadas honestamente:**

- **Límite del probador (la mayoría):** WP no encadena la prueba a través de la llamada de
  auditoría sobre una estructura de ~7 KB. La propiedad **es cierta** (verificado: no hay
  contraejemplos), pero el solver no la cierra en el tiempo dado.
- **Especificación mejorable:** algunos contratos de la máquina de estados aún pueden
  afinarse (trabajo en curso identificado por revisión propia).

> **Nunca se declaró "verificado" algo que WP no demostró.** Esa es la regla del proyecto.

---

## Honestidad como método

Lo que distingue este trabajo:

- Cada número de WP/Eva es **reproducible** con los comandos documentados.
- Los pendientes están **listados con su causa**, no escondidos.
- Se distingue *demostrado* / *analizado* / *probado* / *pendiente*.
- **Z3** está configurado pero cerró 0 obligaciones → no se le atribuye mérito.
- Una **línea base** conservada permite medir el progreso objetivamente.

> Un "100% verificado" sería más vistoso, pero falso. Esto es defendible.

---

## Lo que el núcleo garantiza vs. delega

**Garantiza (en el código, respaldado por tests + lo demostrado):**
autorización, no autovalidación, unicidad, validez de fechas (rango),
máquina de estados, terminalidad de REVOKED, atomicidad en error, auditoría append-only.

**Delega a la capa externa:**
autenticación (el núcleo verifica *autorización*, no identidad),
persistencia, cifrado, concurrencia, enforcement del uso de credenciales.

---

## Estado y trabajo futuro

**Hecho:** modelo, arquitectura, implementación, 241 pruebas, WP (798/854), Eva (0 alarmas), documentación honesta.

**Pendiente (identificado por revisión):**
- Cerrar obligaciones de especificación mejorable en `credentials`/`experience`.
- Invariante global de estado (`state_inv`) para unicidad/append-only global.
- Auditar también los intentos rechazados (hoy solo se auditan los exitosos).
- Validación de calendario completa (hoy solo rango).

---

## Conclusión

- EduChain **demuestra** propiedades de seguridad de un contrato inteligente modelado en C,
  no solo las programa.
- **798/854 obligaciones** demostradas con Frama-C; **241/241** pruebas; **0 alarmas** en Eva.
- Lo no demostrado está **documentado con rigor**, no ocultado.

### El valor no es un número perfecto: es poder defender cada afirmación.

*Gracias.*
