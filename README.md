# EduChain

Simulación en lenguaje **C** de la *lógica* de un contrato inteligente, verificada formalmente
con **Frama-C** (WP + Eva) y ACSL. Proyecto **académico y ficticio**.

> **Qué es y qué NO es.** EduChain modela las reglas de un registro de profesionales, experiencia
> y credenciales como funciones deterministas con una máquina de estados y un log *append-only*.
> **No es una blockchain real**, no despliega contratos en ninguna red, no usa criptografía ni datos
> reales de personas o instituciones. "Contrato inteligente" es aquí una metáfora de diseño.
> Organización ficticia: **EduCorp**.

---

## 1. Objetivo

Desarrollar un núcleo en C, pequeño y modular, y **demostrar formalmente** (hasta donde la
herramienta lo permite) que cumple reglas de seguridad y consistencia: autorización, no
autovalidación, unicidad, validez de fechas, inmutabilidad de lo validado, máquina de estados de
credenciales, límites de almacenamiento, atomicidad en error y auditoría append-only.

La prioridad del proyecto es la **honestidad de la verificación**: no se declara "verificado" nada
que Frama-C no haya demostrado. Se distingue siempre entre **propiedad demostrada por WP**,
**analizada por Eva**, **probada dinámicamente** (tests) y **pendiente**.

---

## 2. Estructura del proyecto

```
EduChain/
├── src/                     núcleo C (lógica del "contrato")
│   ├── security.c           matriz de permisos de los módulos base (security_can)
│   ├── professionals.c      registro de profesionales
│   ├── experience.c         registro y validación de experiencia
│   ├── credentials.c        credenciales como máquina de estados
│   ├── requests.c           ciclo de vida de solicitudes (Opción 2, ver §3-bis)
│   └── audit.c              historial append-only
├── include/                 headers con contratos ACSL (/*@ ... */)
│   ├── common.h             tipos, enums, constantes, códigos de error, estado
│   ├── security.h  professionals.h  experience.h  credentials.h  requests.h  audit.h
├── verification/
│   ├── verify.sh            ejecuta WP + Eva
│   ├── eva_harness.c        punto de entrada para Eva (entradas no deterministas)
│   ├── specifications.h / invariants.h   (stubs reservados; ver Limitaciones)
│   ├── wp_results.txt            resultados WP actuales (Fase 4)
│   ├── wp_results_baseline.txt   LÍNEA BASE original (no modificar)
│   ├── eva_results.txt           salida de Eva
│   └── env_captured.txt          versiones de herramientas
├── tests/                   241 pruebas (unitarias + adversariales)
│   ├── test_professionals.c  test_experience.c  test_credentials.c
│   ├── test_requests.c      pruebas del módulo de solicitudes
│   ├── test_attacks.c       pruebas adversariales A1–A18
│   └── test_framework.h  test_main.c
├── tools/
│   ├── probar_profesional.c   CLI de demo: registrar un profesional en vivo
│   └── probar_profesional.sh  compila y ejecuta esa CLI
├── docs/
│   ├── phase1_model.md          modelo: actores, reglas, estados, propiedades
│   ├── architecture.md          arquitectura y firmas
│   ├── explicacionv1.md         explicación técnica completa del sistema
│   ├── explicacion_ataques.txt  las pruebas adversariales A1–A18 explicadas
│   └── formal_verification.md   resultados WP/Eva, línea base e historial
├── main.c                   demo mínima (NO verificada, fuera del alcance de WP/Eva)
├── run_tests.sh             compila y corre las 241 pruebas (normal + ASan/UBSan)
└── run_demo.sh              compila y corre la demo
```

---

## 3. Modelo (resumen)

**Actores (roles):** `ROLE_ADMIN`, `ROLE_HR`, `ROLE_COORD`, `ROLE_PRO`, `ROLE_VERIFIER`.
Mínimo privilegio y separación de responsabilidades. La autorización se decide en un único punto
(`security_can`). La **autenticación es externa**: el núcleo recibe `caller_role`/`caller_id` como
datos confiables y verifica *autorización*, no *autenticación*.

**Máquina de estados de la credencial:**
```
            issue           activate            block
   (∅) ----------> PENDING ----------> ACTIVE ----------> BLOCKED
                      |                   |                   |
                      | revoke            | revoke            | reactivate / revoke
                      v                   v                   v
                   REVOKED  <---------  REVOKED  <--------  ACTIVE / REVOKED
   REVOKED es TERMINAL. Válida para uso == únicamente ACTIVE.
   revoke permitido desde PENDING, ACTIVE o BLOCKED.
```

**Experiencia:** `PENDING --validate--> VALIDATED` (validado es inmutable por operaciones ordinarias;
nadie valida su propia experiencia).

**Reglas de negocio (BR-1..BR-11)** y **propiedades formales (P-1..P-13)** están en
`docs/phase1_model.md`. Códigos de retorno en `include/common.h` (`EDU_OK=0`, errores negativos).
**Atomicidad:** un error nunca modifica el estado (se valida todo antes de mutar nada).

---

## 3-bis. Módulo `requests` (Opción 2)

Añade el **ciclo de vida de las solicitudes** que alimentan el historial oficial y la elegibilidad
de credenciales. Opera sobre un estado propio `requests_state_t`, separado de `educhain_state_t`.

- **Ciclo:** una solicitud nace `PENDING` (o `ENDORSED` si requiere respaldo), se verifica y se
  **incorpora** al historial como `REQ_RECORDED`.
- **Correcciones trazables (`prev_id`):** una corrección apunta al registro que sustituye; al
  incorporarse, el anterior pasa a **`REQ_SUPERSEDED`** — se conserva el historial sin sobrescribir.
- **Credenciales dependientes de trayectoria:** un profesional es elegible cuando tiene al menos un
  registro vigente `REQ_RECORDED` de las categorías oficiales.

### Autorización: dos decisores (aclaración importante)

El proyecto tiene **dos** funciones de autorización por rol, no una:

- `security_can` (en `security.c`) — decisor de los módulos base (professionals, experience,
  credentials). Verificado en WP 114/114.
- `request_role_can` (interno de `requests.c`) — decisor **propio** del módulo de solicitudes, mismo
  patrón (`switch` puro) pero con su propia matriz. `requests.c` **no** llama a `security_can`.

Son dos puntos de decisión análogos, cada uno para su dominio. (Donde la documentación hable de
"punto único de autorización", se refiere a los módulos base; `requests` tiene el suyo.)

### Dependencia institucional `auth[]` (limitación del prototipo)

`requests.c` consulta un arreglo `auth[]` (vínculos usuario→institución) para comprobar
autorizaciones institucionales. **La Opción 2 NO implementa `authz_grant` / `authz_revoke`**: la
gestión de esos vínculos es responsabilidad de una capa administrativa no incluida en este alcance.

**Consecuencia operativa:** las autorizaciones institucionales deben estar **precargadas en el estado
inicial** `requests_state_t` antes de usar el módulo (p. ej. inicializando `rs->auth[...]` con
`user_id`, `institution_id`, `active = 1` en la configuración del prototipo). No es un defecto: es
una frontera de alcance documentada.

---

## 4. Requisitos y cómo ejecutar

### Entorno
- **WSL2 Ubuntu** (o Debian) con **Frama-C 33.0**, **Alt-Ergo** y **Z3 4.13.0** instalados vía opam.
- `gcc` para compilar y correr las pruebas.

Instalación del toolchain (una vez):
```bash
sudo apt update && sudo apt install -y make m4 gcc opam z3
opam init --disable-sandboxing --shell-setup
eval "$(opam env)"
opam install -y frama-c alt-ergo
why3 config detect
```

### Compilar y probar (241/241)
```bash
bash run_tests.sh      # build normal (-Wall -Wextra) + build con ASan/UBSan, corre los tests
bash run_demo.sh       # demo interactiva mínima
```

### Verificación formal
```bash
# todo (WP + Eva):
bash verification/verify.sh

# WP de un módulo concreto:
eval "$(opam env)"
frama-c -wp -wp-rte -wp-prover alt-ergo,z3 -wp-timeout 20 \
  -cpp-extra-args="-Iinclude -Iverification" src/credentials.c

# Eva (análisis de valores):
frama-c -eva -eva-domains cvalue \
  -cpp-extra-args="-Iinclude -Iverification" \
  src/security.c src/audit.c src/professionals.c src/experience.c src/credentials.c \
  verification/eva_harness.c -main eva_main
```

> En WSL, `frama-c` no está en PATH hasta ejecutar `eval "$(opam env)"`. La distro debe ser la de
> Ubuntu (`wsl -d Ubuntu`), no la default si hay varias.

---

## 5. Estado de la verificación (honesto)

### Pruebas dinámicas
**241/241 pruebas pasan**, en build normal y bajo **AddressSanitizer + UndefinedBehaviorSanitizer**
(0 errores de memoria / comportamiento indefinido). Incluye pruebas adversariales (A1–A18:
escalada de privilegios, autovalidación, transiciones ilegales, duplicados, arreglos llenos, etc.).

### WP (verificación deductiva) — estado actual

| Módulo            | Probados | Total | Failures | Timeouts |
|-------------------|---------:|------:|---------:|---------:|
| `security.c`      | **114**  | 114   | 0        | 0        |
| `audit.c`         | **98**   | 98    | 0        | 0        |
| `professionals.c` | 78       | 81    | 2        | 1        |
| `experience.c`    | 146      | 179   | 9        | 24       |
| `credentials.c`   | 362      | 382   | 16       | 4        |
| `requests.c`      | 739      | 766   | 24       | 3        |
| **TOTAL**         | **1537** | 1620  | 51       | 32       |

`security.c` y `audit.c` están **completos** (0 pendientes). Los demás tienen obligaciones no
demostradas, documentadas en `docs/formal_verification.md` (límite del probador sobre el struct
grande + especificación mejorable; ninguna es contraejemplo).

### Eva (análisis de valores)
`23/23 funciones (100%)`, `440/544 sentencias (81%)`, **0 alarmas** (sin accesos fuera de límites,
overflow, no-init ni división por cero en las trazas exploradas), `54/54 precondiciones válidas`,
`0 inválidas`. *(Los conteos de assertions/precondiciones de `eva_results.txt` corresponden a una
corrida previa y se regeneran al re-ejecutar; 0 alarmas y 0 inválidas se mantienen.)*

### Distinción herramienta-por-herramienta
- **WP** *demuestra* obligaciones de prueba deductivas.
- **Eva** es análisis de valores: ausencia de ciertos errores **en las trazas analizadas** (81% de
  sentencias, no todas). 0 alarmas **no** significa "sin errores en todo caso".
- **Tests + sanitizadores** solo *observan* comportamiento en las entradas ejecutadas.
- **Z3** está configurado y disponible, pero en esta corrida **todas** las obligaciones las cerró Qed
  o Alt-Ergo; **Z3 no cerró ninguna**. No se presenta Z3 como contribuyente.

---

## 6. Limitaciones conocidas (estado real, no final)

Este proyecto **no está terminado**. Pendientes reales:

1. **Hallazgo bloqueante abierto (Fase 4).** Un revisor identificó que el `ensures` de P-1 en
   `cred_transition` es más fuerte que el código (contraejemplo con `caller_id` fuera de rango que
   devuelve `ERR_BAD_ID` en vez de `ERR_PERM`). Hay que condicionar el antecedente al rango de IDs y
   reclasificar la obligación en el doc. Ver `.agents/tasks/educhain-fase4-acsl/review.json`.
2. **Obligaciones WP pendientes (19 Failures + 29 timeouts).** Dos causas:
   - *Limitación de automatización del probador*: WP no encadena el frame a través de la llamada de
     auditoría sobre `educhain_state_t` (~7 KB). Afecta la atomicidad de contadores. Las propiedades
     son ciertas (respaldadas por tests + Eva) pero no cerradas por WP. No son contraejemplos
     (verificado con smoke-tests y reintentos con Z3).
   - *Especificación mejorable*: la máquina de estados (P-7) y la no-autovalidación (P-2) aún no están
     completamente formalizadas como `ensures` en algunos contratos.
3. **Falta `state_inv` global.** Sin un invariante de estado, propiedades como unicidad global de IDs,
   "una credencial por owner" y monotonía global del log no son expresables de forma que WP las cierre
   (hoy se garantizan en el código y por tests, no por contrato).
4. **Auditoría solo de éxitos (S-12).** Los intentos **rechazados** no se registran; el campo
   `event_t.result` siempre vale `EDU_OK`. Mejora pendiente.
5. **R-6 sin enforcement interno (S-6).** El núcleo expone `credential_is_valid` pero no "consume"
   credenciales; aplicar la regla depende de la capa externa.
6. **Validación de fechas por rango, no de calendario.** `20240230` se acepta (decisión documentada).
7. **Entregables finales** (los 12 del enunciado académico) aún sin consolidar.

Lo que el núcleo **NO** garantiza: veracidad real de los datos, persistencia, cifrado, concurrencia,
ni seguridad de la capa de aplicación.

---

## 7. Documentación

| Documento | Contenido |
|-----------|-----------|
| `docs/phase1_model.md` | Actores, matriz de permisos, estructuras, máquinas de estado, reglas BR-*, propiedades P-*, modelo de amenazas, supuestos y límites. |
| `docs/architecture.md` | Arquitectura modular, firmas de funciones, contratos en lenguaje natural, trazabilidad reglas→funciones. |
| `docs/formal_verification.md` | Resultados WP/Eva reales, **línea base** e historial de cada ejecución, clasificación de pendientes. |

---

## 8. Licencia y naturaleza

Proyecto académico, ficticio, sin datos reales. Uso educativo.
