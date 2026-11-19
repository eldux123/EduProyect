# Explicación técnica de la propiedad seleccionada

## Propiedad

> Un rol sin permiso no puede ejecutar una operación protegida.

Instancia concreta usada en el experimento: **`ROLE_VERIFIER` (rol de solo lectura) NO está
autorizado para la operación de escritura `OP_VALIDATE_EXP`** (validar una experiencia).
Es la propiedad de autorización más simple y clara de EduChain, y está documentada en
`docs/explicacionv1.md §4.7`: «`ROLE_VERIFIER` no tiene permiso de escritura en ninguna operación».

## Dónde vive

`security_can` es el **punto único de decisión de autorización** de EduChain: una función pura,
sin estado ni bucles, que codifica la matriz de permisos (rol × operación). Todas las funciones
que mutan estado consultan aquí la autorización por rol.

- Implementación: `src/security.c`, función `int security_can(role_t caller_role, int op_code)`.
- Contrato ACSL: `include/security.h` (cláusulas `ensures` por operación).
- Predicado lógico `role_can`: `verification/specifications.h` (incluido solo bajo `__FRAMAC__`).

## Implementación (caso relevante)

```c
case OP_VALIDATE_EXP:
    return (caller_role == ROLE_HR || caller_role == ROLE_COORD) ? 1 : 0;
```

Solo `ROLE_HR` y `ROLE_COORD` quedan autorizados; cualquier otro rol (incluido `ROLE_VERIFIER`)
obtiene `0`.

## Contrato ACSL (cláusulas que fijan la propiedad)

Numeradas por su orden en el contrato de `security.h`. La mutación M1 las contradice sin
tocarlas:

```
// ensures #4 — bicondicional por operacion para OP_VALIDATE_EXP:
ensures (op_code == OP_VALIDATE_EXP) ==>
        (\result == 1 <==> (caller_role == ROLE_HR || caller_role == ROLE_COORD));

// ensures #8 — un VERIFIER nunca puede escribir nada:
ensures (caller_role == ROLE_VERIFIER) ==> (\result == 0);

// ensures #9 — equivalencia con el predicado logico role_can (misma matriz):
ensures \result == 1 <==> role_can(caller_role, op_code);
```

El predicado `role_can` (en `specifications.h`) refleja la misma matriz; para `OP_VALIDATE_EXP`
exige `r == ROLE_HR || r == ROLE_COORD`, y además `r == ROLE_VERIFIER ==> \false`.

## Obligación WP que se demuestra hoy

Con el código original, WP demuestra las **114/114** obligaciones de `security_can`, incluidas
las derivadas de `ensures #4`, `#8` y `#9`. Evidencia real (idéntica al artefacto versionado
`verification/wp_results_fase4_security.txt`):

```
[wp] Proved goals:  114 / 114
  Qed:           103
  Alt-Ergo :       9
```

(Reproducida en FASE A, ver `salidas_wp/faseA_wp.txt`.)

## Por qué la mutación la viola

La mutación M1 añade `|| caller_role == ROLE_VERIFIER` al caso `OP_VALIDATE_EXP`. Entonces, con
`caller_role == ROLE_VERIFIER` y `op_code == OP_VALIDATE_EXP`, la implementación devuelve `1`,
mientras que el contrato (sin cambios) exige `0`. Ese único punto del espacio de entradas es el
**contraejemplo** que:

- hace **falsa** la cláusula `ensures #8` (`VERIFIER ==> \result == 0`),
- rompe el sentido "⇐" del bicondicional de `ensures #4`, y
- contradice `role_can` en `ensures #9`.

Por eso WP (con Z3) devuelve `[Failure]` sobre `ensures_4`, `ensures_8` y `ensures_9`: ha
encontrado el modelo que viola la propiedad. La detección es lógica y exacta, no estadística.

## Relevancia de seguridad

`OP_VALIDATE_EXP` valida experiencias profesionales, un paso que habilita después la emisión de
credenciales. Un `VERIFIER` es, por diseño, un observador de solo lectura. Que un `VERIFIER`
pudiera validar experiencias sería una **escalada de privilegios**: exactamente el tipo de fallo
de autorización que la verificación formal debe atrapar. El experimento confirma que WP lo atrapa.
