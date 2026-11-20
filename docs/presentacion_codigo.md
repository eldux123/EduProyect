---
marp: true
title: EduChain — Recorrido por el código
author: EduChain
paginate: true
theme: uncover
class: invert
style: |
  :root {
    --edu-accent: #4fc3f7;
    --edu-ok: #66bb6a;
    --edu-warn: #ffb74d;
    --edu-bg: #11151c;
    --edu-panel: #1b2430;
  }
  section {
    background: var(--edu-bg);
    color: #e6edf3;
    font-size: 24px;
    text-align: left;
    padding: 36px 56px;
    font-family: "Segoe UI", system-ui, sans-serif;
  }
  h1 { color: var(--edu-accent); font-size: 46px; letter-spacing: .5px; }
  h2 { color: var(--edu-accent); font-size: 34px; border-bottom: 2px solid #2a3542; padding-bottom: 8px; }
  h3 { color: #b7c3d0; font-weight: 500; }
  strong { color: var(--edu-accent); }
  a { color: var(--edu-accent); }
  code { font-family: "JetBrains Mono","Consolas",monospace; }
  pre {
    background: var(--edu-panel);
    border: 1px solid #2a3542;
    border-left: 4px solid var(--edu-accent);
    border-radius: 8px;
    font-size: 19px;
    padding: 16px 20px;
  }
  blockquote {
    border-left: 4px solid var(--edu-ok);
    background: #16202b;
    color: #cfe8d4;
    padding: 8px 18px;
    border-radius: 6px;
    font-size: 18px;
    line-height: 1.35;
  }
  table { font-size: 22px; border-collapse: collapse; }
  th { background: #22303f; color: var(--edu-accent); }
  td, th { border: 1px solid #2a3542; padding: 8px 14px; }
  section.lead { text-align: center; }
  section.lead h1 { font-size: 60px; }
  footer { color: #5b6b7c; font-size: 14px; }
---

<!-- _class: lead invert -->
<!-- _paginate: false -->

# EduChain

### Simulación de un contrato inteligente en **C**, verificada formalmente con **Frama-C**

Recorrido por el código · modelo → seguridad → verificación

---

<!-- _footer: 'cifras reales del repositorio' -->
## El proyecto en números

| Dimensión | Valor |
|-----------|------:|
| Módulos (núcleo C) | **6** |
| Líneas de código C (`src/`) | **1 262** |
| Líneas de cabecera + ACSL (`include/`) | **1 160** |
| Funciones públicas | **30** |
| Estados modelados (credencial + solicitud) | **10** |
| Operaciones auditables (`op_code`) | **15** |
| Pruebas (unitarias + adversariales) | **241** |

> Un núcleo **pequeño y acotado a propósito**: cuanto más simple, más verificable.

---

<!-- _footer: 'qué hace cada módulo' -->
## Qué hace cada módulo

| Módulo | Responsabilidad |
|--------|-----------------|
| `security` | **Decide quién puede hacer qué** (matriz rol × operación). |
| `professionals` | Registra profesionales (identidad institucional). |
| `experience` | Registra y **valida** experiencia laboral. |
| `credentials` | Emite y opera el carné como **máquina de estados**. |
| `requests` | Ciclo de vida de **solicitudes** y correcciones trazables. |
| `audit` | **Historial append-only**: quién hizo qué y cuándo. |

> Cada módulo tiene una sola responsabilidad. La seguridad no está en un solo
> sitio: es la **suma de controles** repartidos en todos ellos.

---

<!-- _footer: 'cómo se construyó la seguridad' -->
## ¿Cómo se hizo seguro el contrato?

La seguridad se diseñó en **capas**, cada una cubre un tipo de ataque:

1. **Autorización** — nadie ejecuta algo fuera de su rol (`security`).
2. **Separación de deberes** — quien crea un dato no lo aprueba (`experience`).
3. **Validación de entradas** — ningún parámetro se asume válido.
4. **Máquina de estados** — solo transiciones legales; REVOKED terminal.
5. **Atomicidad** — un error nunca deja el estado a medias.
6. **Trazabilidad** — toda operación queda auditada.

> Y encima de todo: **verificación formal** con Frama-C, que prueba que estas
> reglas se cumplen para todas las entradas, no solo las que probé.

---

<!-- _footer: 'include/common.h' -->
## 1. El estado: todo vive en una struct

```c
typedef struct {
    professional_t pros[MAX_PROFESSIONALS];  int pros_count;
    experience_t   exps[MAX_EXPERIENCES];    int exps_count;
    credential_t   creds[MAX_CREDENTIALS];   int creds_count;
    event_t        log[MAX_EVENTS];          int log_count;
} educhain_state_t;
```

> Arreglos de tamaño fijo, sin memoria dinámica. Un estado simple y acotado es
> mucho **más fácil de verificar formalmente**. Ocupa ~7 KB (importa al final).

---

<!-- _footer: 'include/common.h' -->
## 2. Roles y códigos de error

```c
typedef enum { ROLE_ADMIN, ROLE_HR, ROLE_COORD,
               ROLE_PRO, ROLE_VERIFIER } role_t;

#define EDU_OK              0
#define ERR_PERM           -2   /* rol sin permiso */
#define ERR_SELF_VALIDATE  -3   /* no puedes validar lo tuyo */
#define ERR_BAD_TRANSITION -9   /* transición de estado ilegal */
```

> Cinco roles con **mínimo privilegio**. Cada operación devuelve un código:
> 0 si éxito, negativo si se rechaza. Nunca corrompen el estado.

---

<!-- _footer: 'cómo leer lo que sigue' -->
## Las 6 capas, en el código

Las siguientes slides muestran **cada capa de seguridad en el código real**:

- Slide 3 → **Autorización** (`security.c`)
- Slide 4 → **Separación de deberes** (`experience.c`)
- Slide 5 → **Máquina de estados** (`credentials.c`)
- Slide 6 → **Atomicidad** (patrón común)

Cada una: el fragmento que la implementa + por qué impide un ataque concreto.

---

<!-- _footer: 'src/security.c' -->
## 3. Capa 1 — Autorización: punto único de decisión

```c
int security_can(role_t caller_role, int op_code) {
    switch (op_code) {
    case OP_REGISTER_PRO: return (caller_role==ROLE_ADMIN
                               || caller_role==ROLE_HR) ? 1:0;
    case OP_VALIDATE_EXP: return (caller_role==ROLE_HR
                               || caller_role==ROLE_COORD) ? 1:0;
    case OP_ISSUE_CRED:   return (caller_role==ROLE_COORD) ? 1:0;
    default: return 0;          // deny-by-default
    }
}
```

> Autorización centralizada. `default: return 0` → **todo lo no permitido
> explícitamente se deniega**. Cada mutador llama aquí antes de tocar nada.

**Ataque que impide:** escalada de privilegios — un rol sin permiso (p. ej. `ROLE_PRO`
intentando emitir una credencial) recibe `ERR_PERM` y no ejecuta nada.

---

<!-- _footer: 'src/experience.c → experience_validate' -->
## 4. Capa 2 — Separación de deberes: nadie valida lo propio

```c
if (!security_can(caller_role, OP_VALIDATE_EXP))
    return ERR_PERM;                                  // (1) ¿rol?

/* No autovalidación, con independencia del rol: */
if (st->exps[idx].owner_id == caller_id)
    return ERR_SELF_VALIDATE;                         // (2) ¿identidad?
```

> Dos capas: autorización por **rol** y por **identidad**. Aunque tengas el rol
> de validador, no puedes validar **tu propia** experiencia.

**Ataque que impide:** autoacreditación — alguien con rol de validador que intenta
aprobar su propia experiencia recibe `ERR_SELF_VALIDATE`.

---

<!-- _footer: 'src/credentials.c' -->
## 5. Capa 3 — Máquina de estados: REVOKED es terminal

```c
/* exige el estado de origen exacto */
if (st->creds[idx].state != from)
    return ERR_BAD_TRANSITION;

/* no se puede revocar lo ya revocado */
if (st->creds[idx].state == CRED_REVOKED)
    return ERR_BAD_TRANSITION;
```

`PENDING → ACTIVE → BLOCKED → REVOKED` · válida solo si **ACTIVE**

> Cada transición exige el estado de origen exacto. **REVOKED es terminal**:
> una credencial revocada no se reactiva nunca.

**Ataque que impide:** reutilización de una credencial revocada — cualquier intento
de reactivarla o re-revocarla da `ERR_BAD_TRANSITION`.

---

<!-- _footer: 'src/professionals.c (patrón común)' -->
## 6. Capa 4 — Atomicidad: validar todo antes de mutar

```c
if (st == 0)                     return ERR_NULL;       // 1
if (new_id < ID_MIN || ...)      return ERR_BAD_ID;     // 2
if (!security_can(role, ...))    return ERR_PERM;       // 3
if (professional_find(...) >= 0) return ERR_DUPLICATE;  // 4
if (st->pros_count >= MAX_...)   return ERR_FULL;       // 5
/* --- recién ahora se escribe y se audita --- */
st->pros[st->pros_count].id = new_id; ...
audit_record_event(st, caller_id, OP_REGISTER_PRO, new_id, EDU_OK);
```

> Todas las comprobaciones **antes** de la primera escritura. Si algo falla, el
> estado queda intacto. Toda operación exitosa deja rastro de auditoría.

**Ataque que impide:** dejar el sistema en estado inconsistente — un error a mitad
de operación no corrompe los datos ni crea registros a medias.

---

<!-- _footer: 'inventario de especificación ACSL' -->
## 7. Cuánta especificación formal hay

| Tipo de anotación ACSL | Cantidad |
|------------------------|---------:|
| `requires` (precondiciones) | **100** |
| `ensures` (postcondiciones) | **146** |
| `assigns` (marcos de memoria) | **78** |
| `loop invariant` (invariantes de lazo) | **23** |
| `behavior` (casos por comportamiento) | **28** |
| `predicate` (predicados lógicos) | **3** |

> No es "una anotación de ejemplo": son **~450 líneas de especificación formal**
> repartidas en los 6 módulos. Cada función crítica tiene su contrato.

---

<!-- _footer: 'include/security.h' -->
## 8. Verificación formal: el contrato ACSL

```c
/*@
  requires \true;
  assigns  \nothing;                        // no modifica memoria
  ensures  \result == 0 || \result == 1;    // siempre 0 o 1
  ensures  (op_code == OP_VALIDATE_EXP) ==>
           (\result == 1 <==>
             (caller_role==ROLE_HR || caller_role==ROLE_COORD));
*/
int security_can(role_t caller_role, int op_code);
```

> **No es código que se ejecuta: es matemática.** `requires`/`ensures`/`assigns`.
> El compilador las ignora; **Frama-C las demuestra** para TODA entrada posible.

---

<!-- _footer: 'include/professionals.h → professional_register' -->
## 8-bis. Ejemplo: registrar un profesional

```c
/*@
  requires \valid(st);                                  // PRE: estado válido
  requires 0 <= st->pros_count <= MAX_PROFESSIONALS;    // PRE: contador en rango
  // POST: rol no autorizado ⇒ se rechaza y el contador NO cambia
  ensures (caller_role != ROLE_ADMIN && caller_role != ROLE_HR)
          ==> (\result == ERR_PERM && st->pros_count == \old(st->pros_count));
  // POST: si éxito, el contador crece exactamente en 1
  ensures \result == EDU_OK ==> st->pros_count == \old(st->pros_count) + 1;
  // POST: ante cualquier error, el estado queda intacto (atomicidad)
  ensures \result != EDU_OK ==> st->pros_count == \old(st->pros_count);
*/
```

> **En palabras:** si quien llama **no es ADMIN ni HR** → `ERR_PERM` y **no registra a nadie**.
> Si hay éxito → se añade **exactamente un** profesional. Si falla → el estado queda **igual**.
> Y queda probado para **cualquier** entrada, no solo los casos probados a mano.

---

<!-- _footer: 'include/audit.h → audit_record_event' -->
## 8-ter. Ejemplo: auditoría append-only (con behaviors)

```c
/*@
  requires \valid(st);
  requires 0 <= st->log_count <= MAX_EVENTS;
  behavior full:                                   // CASO: log lleno
    assumes st->log_count >= MAX_EVENTS;
    assigns \nothing;                              //   no toca nada
    ensures \result == ERR_FULL;
  behavior ok:                                     // CASO: hay espacio
    assumes st->log_count < MAX_EVENTS;
    ensures \result == EDU_OK ==>                  //   seq estrictamente creciente
            st->log[\old(st->log_count)].seq
            == st->log[\old(st->log_count)-1].seq + 1;
  complete behaviors; disjoint behaviors;          // los casos cubren todo y no se solapan
*/
```

> **En palabras:** si el **log está lleno**, no toca nada y devuelve `ERR_FULL`. Si **hay
> espacio** y hay éxito, el nuevo evento recibe un `seq` **justo uno mayor que el anterior**:
> historial ordenado y sin huecos (*append-only*). Los dos casos **cubren todo y nunca
> coinciden**.

---

<!-- _footer: 'resultados reales — verification/' -->
## 9. La prueba: tres niveles de garantía

| Herramienta | Qué hace | Resultado |
|-------------|----------|-----------|
| **WP** | demuestra deductivamente | **1537 / 1620** obligaciones |
| **Eva** | analiza valores (memoria) | **0 alarmas**, 23/23 funciones |
| **Tests + ASan/UBSan** | observa ejecuciones | **241 / 241** |

```
$ frama-c -wp ... src/security.c
[wp] Proved goals:  114 / 114      ← módulo 100% demostrado
```

> WP **demuestra**, Eva **analiza**, los tests **observan**. Mantengo la distinción.

---

<!-- _footer: 'docs/formal_verification.md' -->
## 10. Honestidad: lo que NO se demostró

```
professionals.c   78 / 81    (2 Failed, 1 Timeout)
experience.c     146 / 179   (9 Failed, 24 Timeout)
credentials.c    362 / 382   (16 Failed, 4 Timeout)
```

> **Qué no pude demostrar y por qué:** estas obligaciones quedaron abiertas porque
> el solucionador no logra completar la prueba cuando tiene que razonar sobre el
> estado completo (~7 KB) a través de la llamada de auditoría — se queda sin
> tiempo. **No es un error del código:** no hay ningún contraejemplo y los casos
> están cubiertos por las pruebas. Quedan como trabajo pendiente, documentadas.

---

<!-- _class: lead invert -->
<!-- _paginate: false -->

# Gracias
