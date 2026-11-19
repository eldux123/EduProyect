# Guía de sustentación — EduChain

Documento único para tener al lado durante la defensa. Dos partes:
**A) manejo de la GUI de Frama-C (Ivette)** · **B) mecanismos de seguridad con código real.**

---

# PARTE A — GUI de Frama-C (Ivette)

## Lanzar (terminal WSL Ubuntu)
```bash
cd /mnt/c/Users/erick/Downloads/Blockchain/EduProyect
bash verification/gui.sh credentials     # un módulo
bash verification/gui.sh requests        # el módulo grande (766 obligaciones, tarda más)
bash verification/gui.sh                  # los 6 módulos juntos
bash verification/gui.sh --eva            # análisis de valores (Eva)
```
Listo cuando: la barra **WP** verde (abajo izq.) deja de girar y en **Console** sale
`Proved goals: N / M` y `[server] Server running.`

> **Solo UNA ventana a la vez.** Ivette no permite dos instancias; la segunda falla con
> `Cannot load window URL / ERR_FAILED`. Para cambiar de módulo, cierra la anterior (botón X) o:
> ```bash
> bash verification/gui_close.sh     # cierra toda instancia de la GUI
> bash verification/gui_status.sh    # ¿hay GUI viva?
> ```
> Lánzala desde TU terminal (si la lanza un proceso que termina, la ventana se cierra con él).

> Si abajo dice **"UN"** y está vacío → arrancó sin análisis. Relanza con el argumento del módulo.

## Orden de paneles a mostrar (lista "Views", izquierda)
1. **Console** — señala `Proved goals: 362/382` (credentials) · `739/766` (requests).
   *"Frama-C demostró estas obligaciones; coinciden con lo documentado."*
2. **WP View** — cada obligación con color: **verde=demostrada**, rojo=Failed, naranja=Timeout.
   *"Cada propiedad de cada función es una obligación; la mayoría demostradas para TODA entrada."*
3. **Source Code** — clic en una función (p. ej. `credential_revoke`): ves sus `requires`/`ensures`/
   `assigns` con ✓ o ✗. *"Esta es la especificación formal; WP prueba que el código la cumple."*
4. **Properties** — tabla de propiedades y su estado.
5. **Eva** (si lanzas `--eva`): **Eva Summary/Values** → `0 alarms`.
   *"Eva confirma ausencia de errores de memoria/overflow en las trazas."*

## Cómo explicar un ROJO (Failed/Timeout)
> "No la cierra el solver porque la prueba atraviesa una llamada de auditoría sobre una estructura
> de ~7 KB y el probador no encadena el marco. La propiedad **es cierta** (verificado con
> smoke-tests, sin contraejemplo) y está respaldada por las pruebas dinámicas. No la declaro
> demostrada porque formalmente no lo está."

## Avisos `annot:missing-spec` (panel Messages)
No son errores: al analizar un módulo aislado, WP usa los *contratos* de las funciones externas, no
su código. Es el funcionamiento normal de la **verificación modular**.

---

# PARTE B — Mecanismos de seguridad (con código real)

## 1. Autorización por rol — deny-by-default
**`src/security.c` → `security_can`**
```c
switch (op_code) {
  case OP_REGISTER_PRO: return (role==ROLE_ADMIN || role==ROLE_HR) ? 1:0;
  case OP_VALIDATE_EXP: return (role==ROLE_HR || role==ROLE_COORD) ? 1:0;
  ...
  default: return 0;   // todo lo no permitido se deniega
}
```
*"Autorización centralizada; `default: return 0` = deny-by-default. Cada mutador llama aquí antes de
tocar nada."* **Prueba:** `security.c` 114/114 en WP. (`requests` tiene su análogo `request_role_can`.)

## 2. Separación de deberes — nadie valida lo propio
**`src/experience.c` → `experience_validate`**
```c
if (!security_can(caller_role, OP_VALIDATE_EXP)) return ERR_PERM;   // rol
if (st->exps[idx].owner_id == caller_id) return ERR_SELF_VALIDATE;  // identidad
```
*"Dos capas: autorización por rol y por identidad. Aunque seas validador, no validas tu propia
experiencia."*

## 3. Integridad / inmutabilidad — lo validado no se altera
**`src/experience.c`**
```c
if (st->exps[idx].state == EXP_VALIDATED) return ERR_ALREADY_VALID;
st->exps[idx].state = EXP_VALIDATED;  st->exps[idx].validator_id = caller_id; // fechas/owner NO cambian
```
*"Un registro aprobado no se modifica con operaciones ordinarias; el `assigns` ACSL lista exactamente
los campos escribibles."*

## 4. Máquina de estados + REVOKED terminal — credenciales
**`src/credentials.c` → `cred_transition` / `credential_revoke`**
```c
if (st->creds[idx].state != from) return ERR_BAD_TRANSITION;   // solo transición legal
if (st->creds[idx].state == CRED_REVOKED) return ERR_BAD_TRANSITION; // revoke: terminal
```
*"Cada operación exige el estado origen exacto; REVOKED es terminal; válida solo si ACTIVE."*

## 5. Validación de entradas — defensa en profundidad
**Inicio de cada mutador (patrón uniforme)**
```c
if (st == 0) return ERR_NULL;
if (caller_id < ID_MIN || caller_id > ID_MAX) return ERR_BAD_ID;
if (!security_can(...)) return ERR_PERM;
if (st->X_count >= MAX_X) return ERR_FULL;
```
*"Orden: nulo → rango de ids → permiso → reglas → capacidad. Nada se asume válido."*

## 6. Atomicidad — validar todo antes de mutar nada
*"Todas las comprobaciones antes de la primera escritura; un error deja el estado idéntico. Reservo
el espacio del log ANTES de mutar, para que no haya cambio sin su evento."* **Prueba:** test A10.

## 7. Auditoría append-only — trazabilidad
**`src/audit.c` → `audit_record_event`**
*"Cada operación exitosa deja evento con `actor_id = caller_id` real, operación, registro y resultado.
El log solo crece; `seq` estrictamente creciente."* **Prueba:** `audit.c` 98/98 en WP.
**Límite honesto:** hoy solo se auditan éxitos; los intentos rechazados no (mejora S-12).

## Resumen de una frase
> "Cinco capas: autorización por rol (deny-by-default), por identidad (separación de deberes),
> validación de entradas, máquina de estados con transiciones legales, y atomicidad en error — todo
> con trazabilidad append-only. Y no solo lo programé: lo **especifiqué en ACSL y lo verifiqué con
> Frama-C** (1537/1620 obligaciones demostradas)."

---

# ¿Dónde está la verificación formal?
- **Contratos ACSL** (`/*@ requires/ensures/assigns */`) en `include/*.h` e invariantes de lazo en
  `src/*.c` — ~466 líneas de especificación. Son propiedades matemáticas; `gcc` las ignora, Frama-C
  las demuestra.
- **Predicados lógicos** (`role_can`, ...) en `verification/specifications.h` (bajo `#ifdef __FRAMAC__`).
- **Resultados** en `verification/wp_results.txt` y `eva_results.txt` (reproducibles).

**Distinción clave:** el `.c` *implementa*; el contrato ACSL dice *qué debe cumplir*; WP *demuestra*
que lo cumple para toda entrada; los tests lo *comprueban* en casos concretos.

## Frases de cierre
- "WP **demuestra**; Eva **analiza**; los tests **observan**. Distingo siempre los tres."
- "1537 de 1620 obligaciones demostradas; lo pendiente está documentado con su causa."
- "El valor no es un número perfecto: es poder defender cada afirmación."
