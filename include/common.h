/* EduChain - common.h
 * Tipos, constantes y codigos de error compartidos por todos los modulos.
 * Proyecto academico ficticio: simula la logica de un contrato inteligente en C.
 * NO es una blockchain real. Sin datos personales reales.
 */
#ifndef EDUCHAIN_COMMON_H
#define EDUCHAIN_COMMON_H

#include <stddef.h>

/* ---- Limites (decisiones de diseno, Fase 1 §4.1) ---- */
#define MAX_PROFESSIONALS 64
#define MAX_EXPERIENCES   256
#define MAX_CREDENTIALS   64
#define MAX_EVENTS        1024

#define ID_MIN            1
#define ID_MAX            2000000000   /* dentro de int de 32 bits */

#define DATE_MIN          19000101
#define DATE_MAX          21001231

/* ---- Roles (Fase 1 §2) ---- */
typedef enum {
    ROLE_ADMIN    = 0,
    ROLE_HR       = 1,
    ROLE_COORD    = 2,
    ROLE_PRO      = 3,
    ROLE_VERIFIER = 4
} role_t;

/* ---- Codigos de operacion para auditoria ---- */
typedef enum {
    OP_REGISTER_PRO    = 1,
    OP_REGISTER_EXP    = 2,
    OP_VALIDATE_EXP    = 3,
    OP_ISSUE_CRED      = 4,
    OP_ACTIVATE_CRED   = 5,
    OP_BLOCK_CRED      = 6,
    OP_REACTIVATE_CRED = 7,
    OP_REVOKE_CRED     = 8
} op_code_t;

/* ---- Codigos de retorno (Fase 1 §6) ---- */
#define EDU_OK               0
#define ERR_NULL            -1
#define ERR_PERM            -2
#define ERR_SELF_VALIDATE   -3
#define ERR_DUPLICATE       -4
#define ERR_NOT_FOUND       -5
#define ERR_BAD_DATE        -6
#define ERR_BAD_ID          -7
#define ERR_FULL            -8
#define ERR_BAD_TRANSITION  -9
#define ERR_IMMUTABLE      -10
#define ERR_ALREADY_VALID  -11

/* ---- Estados ---- */
typedef enum { EXP_PENDING = 0, EXP_VALIDATED = 1 } exp_state_t;

typedef enum {
    CRED_PENDING = 0,
    CRED_ACTIVE  = 1,
    CRED_BLOCKED = 2,
    CRED_REVOKED = 3
} cred_state_t;

/* ---- Estructuras de datos (Fase 1 §4.2) ---- */
typedef struct {
    int id;        /* ID_MIN..ID_MAX; 0 = slot libre */
    int active;    /* 0/1 */
} professional_t;

typedef struct {
    int         id;
    int         owner_id;
    int         start_date;   /* YYYYMMDD */
    int         end_date;     /* YYYYMMDD, >= start_date */
    exp_state_t state;
    int         validator_id; /* 0 si no validada; != owner_id si validada */
} experience_t;

typedef struct {
    int          id;
    int          owner_id;
    cred_state_t state;
} credential_t;

typedef struct {
    int seq;        /* secuencial, estrictamente creciente */
    int actor_id;
    int op_code;
    int target_id;
    int result;
} event_t;

typedef struct {
    professional_t pros[MAX_PROFESSIONALS];  int pros_count;
    experience_t   exps[MAX_EXPERIENCES];    int exps_count;
    credential_t   creds[MAX_CREDENTIALS];   int creds_count;
    event_t        log[MAX_EVENTS];          int log_count;
} educhain_state_t;

#endif /* EDUCHAIN_COMMON_H */
