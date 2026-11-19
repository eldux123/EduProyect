# Chuleta — Mecanismos de seguridad del contrato (con código real)

Si te preguntan "¿qué método de seguridad usas y dónde está en el código?", abre el archivo
indicado y señala estas líneas. Cada mecanismo tiene: **qué es**, **dónde**, **qué decir**.

---

## 1. Autorización por rol — "punto único de decisión"

**Dónde:** `src/security.c` → función `security_can(caller_role, op_code)`.

```c
int security_can(role_t caller_role, int op_code) {
    switch (op_code) {
    case OP_REGISTER_PRO:  return (caller_role==ROLE_ADMIN || caller_role==ROLE_HR) ? 1:0;
    case OP_VALIDATE_EXP:  return (caller_role==ROLE_HR || caller_role==ROLE_COORD) ? 1:0;
    case OP_ISSUE_CRED:    return (caller_role==ROLE_COORD) ? 1:0;
    ...
    default: return 0;   // deny-by-default
    }
}
```

**Qué decir:** "La autorización está centralizada en un `switch` puro. Cada operación define qué
roles pueden ejecutarla. Lo importante: el `default: return 0` → **todo lo no permitido
explícitamente se deniega** (deny-by-default). Cada función que muta estado llama aquí ANTES de
tocar nada." *(El módulo `requests` tiene su propio decisor análogo, `request_role_can`.)*

**Prueba formal:** `security.c` está **114/114 en WP** — su autorización está completamente demostrada.

---

## 2. Separación de deberes — "nadie valida lo propio"

**Dónde:** `src/experience.c` → `experience_validate`, chequeo `owner_id == caller_id`.

```c
if (!security_can(caller_role, OP_VALIDATE_EXP)) return ERR_PERM;   // (1) rol
...
/* No autovalidacion (BR-2), con independencia del rol. */
if (st->exps[idx].owner_id == caller_id) return ERR_SELF_VALIDATE;  // (2) identidad
```

**Qué decir:** "Hay DOS capas: la autorización por rol (`security_can`) y la **autorización por
identidad** (quién es el titular). Aunque tengas rol de validador, no puedes validar TU propia
experiencia — se comprueba `owner_id == caller_id` y se rechaza con `ERR_SELF_VALIDATE`. Es
separación de responsabilidades: quien registra no es quien valida."

---

## 3. Integridad / inmutabilidad — "lo validado no se altera"

**Dónde:** `src/experience.c` → `experience_validate`.

```c
/* Inmutabilidad: si ya esta validada, no se toca (BR-5). */
if (st->exps[idx].state == EXP_VALIDATED) return ERR_ALREADY_VALID;
...
/* Solo cambian state y validator_id; fechas y owner NO cambian. */
st->exps[idx].state        = EXP_VALIDATED;
st->exps[idx].validator_id = caller_id;
```

**Qué decir:** "Un registro aprobado no se puede modificar con operaciones ordinarias: si ya está
`VALIDATED`, se rechaza. Y al validar solo se tocan `state` y `validator_id` — las fechas y el
titular nunca cambian. En ACSL esto se captura con el `assigns`, que lista exactamente los campos
escribibles."

---

## 4. Máquina de estados + terminalidad — credenciales

**Dónde:** `src/credentials.c` → `cred_transition` (exige estado origen exacto) y `credential_revoke`.

```c
if (st->creds[idx].state != from) return ERR_BAD_TRANSITION;  // solo transición legal
...
// revoke: terminal
if (st->creds[idx].state == CRED_REVOKED) return ERR_BAD_TRANSITION;
```

**Qué decir:** "Las transiciones de credencial están controladas: cada operación exige el estado
origen exacto (`from`), cualquier otra transición da `ERR_BAD_TRANSITION`. Y **REVOKED es terminal**:
ninguna operación saca una credencial de revocada. Una credencial solo es válida si está `ACTIVE`."

---

## 5. Validación de entradas — defensa en profundidad

**Dónde:** inicio de CADA función que muta estado (patrón uniforme).

```c
if (st == 0) return ERR_NULL;                              // puntero nulo
if (caller_id < ID_MIN || caller_id > ID_MAX) return ERR_BAD_ID;   // rango de id
if (!security_can(...)) return ERR_PERM;                   // permiso
...                                                        // reglas específicas
if (st->X_count >= MAX_X) return ERR_FULL;                 // capacidad
```

**Qué decir:** "Toda función valida en orden: puntero no nulo → rango de identificadores → permiso →
reglas de negocio → capacidad. Nada se asume válido. Los códigos de retorno (`common.h`) comunican
cada tipo de error."

---

## 6. Atomicidad — "validar todo antes de mutar nada"

**Dónde:** patrón en todas las operaciones de escritura.

```c
// ... TODOS los chequeos primero (null, id, permiso, duplicado, capacidad, espacio de log) ...
// ... solo DESPUÉS, la mutación: ...
st->X[n] = ...;  st->X_count = n + 1;
audit_record_event(...);   // y se audita
```

**Qué decir:** "Un error nunca deja el estado a medias: todas las comprobaciones ocurren antes de la
primera escritura. Si algo falla, se retorna un código negativo y el estado queda idéntico. Además
reservo el espacio del log ANTES de mutar, para que no haya un cambio sin su evento de auditoría."

**Prueba:** el test adversarial A10 verifica que un error no modifica los contadores.

---

## 7. Auditoría append-only — trazabilidad

**Dónde:** `src/audit.c` → `audit_record_event`.

**Qué decir:** "Cada operación exitosa deja un evento con **quién** lo hizo (`actor_id = caller_id`
real), qué operación, sobre qué registro y el resultado. El log solo crece (append-only) y el `seq`
es estrictamente creciente — no se sobrescriben eventos. `audit.c` está **98/98 en WP**."

**Límite honesto (si preguntan):** hoy solo se auditan las operaciones **exitosas**; los intentos
rechazados no se registran (documentado como mejora S-12).

---

## Resumen de una frase

> "Uso **cinco capas**: autorización por rol (deny-by-default), autorización por identidad
> (separación de deberes), validación de entradas, máquina de estados con transiciones legales, y
> atomicidad en error — todo con **trazabilidad** en un log append-only. Y lo relevante: no solo lo
> programé, lo **especifiqué en ACSL y lo verifiqué con Frama-C** (1537/1620 obligaciones demostradas)."

---

## Si piden ver la PRUEBA de un mecanismo (no solo el código)

- **En la GUI:** abre `security.c` → panel WP View → todas verdes (114/114). Ese es el mecanismo de
  autorización **demostrado**, no solo escrito.
- **En consola:** `frama-c -wp ... src/security.c` → `Proved goals: 114/114`.
- **Distinción clave:** el código *implementa* la regla; WP *demuestra* que la implementación la
  cumple para toda entrada; los tests la *comprueban* en casos concretos.
