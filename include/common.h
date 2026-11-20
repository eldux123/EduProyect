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

/* ---- Limites del flujo de solicitudes (Rev 5 §4.3/§10-D6, aditivo) ---- */
#define MAX_REQUESTS      128   /* capacidad de reqs[] (no definitivo para produccion) */
#define MAX_AUTHZ         64    /* capacidad de auth[]  (no definitivo para produccion) */

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
    ROLE_VERIFIER = 4,
    ROLE_INSTITUTION = 5    /* Rev 5 §3.1: actor nuevo (append-only, no renumera) */
} role_t;

/* Rev 5 §3.1: ROLE_PRO actua como PROFESSOR en el flujo nuevo. Alias aditivo,
 * mismo valor; no introduce un rol nuevo, solo nombra el existente. */
#define ROLE_PROFESSOR ROLE_PRO

/* ---- Codigos de operacion para auditoria ---- */
typedef enum {
    OP_REGISTER_PRO    = 1,
    OP_REGISTER_EXP    = 2,
    OP_VALIDATE_EXP    = 3,
    OP_ISSUE_CRED      = 4,
    OP_ACTIVATE_CRED   = 5,
    OP_BLOCK_CRED      = 6,
    OP_REACTIVATE_CRED = 7,
    OP_REVOKE_CRED     = 8,
    /* ---- Operaciones del flujo de solicitudes (Rev 5 §3.3, append-only) ---- */
    OP_SUBMIT_REQUEST        = 9,
    OP_ENDORSE_REQUEST       = 10,
    OP_VERIFY_REQUEST        = 11,
    OP_REJECT_REQUEST        = 12,
    OP_RECORD_REQUEST        = 13,
    OP_GRANT_REPRESENTATION  = 14,
    OP_REVOKE_REPRESENTATION = 15
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

/* =====================================================================
 * Flujo de solicitudes (Rev 5) -- tipos ADITIVOS.
 * El estado nuevo vive en requests_state_t SEPARADO (NO en educhain_state_t,
 * Rev 5 §4.3/§4.5), para no degradar el framing de WP de los modulos ya
 * verificados. Ningun valor existente arriba se modifica.
 * ===================================================================== */

/* ---- Estados de una solicitud (Rev 5 §4.1) ---- */
typedef enum {
    REQ_PENDING    = 0,   /* recien creada, a la espera de verificacion         */
    REQ_ENDORSED   = 1,   /* respaldada por su institucion (categorias §5)      */
    REQ_VERIFIED   = 2,   /* aprobada por VERIFIER, lista para incorporar       */
    REQ_REJECTED   = 3,   /* rechazada por VERIFIER (terminal)                  */
    REQ_RECORDED   = 4,   /* incorporada al historial oficial vigente por ADMIN */
    REQ_SUPERSEDED = 5    /* era RECORDED y fue sustituida (terminal, Rev 3)    */
} request_state_t;

/* ---- Las 14 categorias (Rev 5 §4.1) ---- */
typedef enum {
    CAT_INSTITUTION_WORKED    = 0,
    CAT_LINK_PERIOD           = 1,
    CAT_LINK_TYPE_DEDICATION  = 2,
    CAT_COURSES_TAUGHT        = 3,
    CAT_ACADEMIC_PERIOD       = 4,
    CAT_TEACHING_HOURS        = 5,
    CAT_ACADEMIC_ACTIVITY     = 6,
    CAT_RESEARCH_PROJECT      = 7,
    CAT_PUBLICATION           = 8,
    CAT_THESIS_SUPERVISION    = 9,
    CAT_OUTREACH              = 10,
    CAT_COMMITTEES            = 11,
    CAT_TRAINING              = 12,
    CAT_CERTIFICATION         = 13
} request_category_t;   /* 14 valores */

/* ---- Solicitud: struct generico con metadatos por categoria (Rev 5 §4.2) ---- */
typedef struct {
    int                id;             /* id unico (ID_MIN..ID_MAX); 0 = slot libre */
    int                owner_id;       /* profesor titular (a quien acredita)       */
    int                institution_id; /* institucion representada (0 si no aplica)  */
    int                submitter_id;   /* presentador real (= caller_id que la creo) */
    int                verifier_id;    /* 0 si no verificada; != submitter si verif. */
    int                endorser_id;    /* institucion que respaldo (0 si sin respaldo)*/
    request_category_t category;       /* una de las 14 categorias                   */
    request_state_t    state;          /* PENDING/ENDORSED/VERIFIED/REJECTED/...      */
    int                start_date;     /* YYYYMMDD, DATE_MIN..DATE_MAX                */
    int                end_date;       /* YYYYMMDD, >= start_date                      */
    /* evidence_ref es una referencia EXTERNA OPACA (Rev 5 §4.4):
     * NO es un hash criptografico, NO calcula SHA-256, NO garantiza integridad
     * documental. La veracidad depende de la autoridad del VERIFIER, no de este
     * campo. El contrato NO afirma ninguna propiedad criptografica sobre el. */
    int                evidence_ref;
    int                prev_id;        /* id del registro que corrige; 0 = original  */
    /* Metadatos genericos reinterpretados por `category` (Rev 5 §4.2). */
    int                num_a;
    int                num_b;
    int                num_c;
    int                num_d;
} request_t;

/* ---- Vinculo de representacion usuario -> institucion (Rev 5 §3.2) ---- */
typedef struct {
    int user_id;         /* caller_id autorizado a representar...        */
    int institution_id;  /* ...a esta institucion                        */
    int active;          /* 0/1 (revocar un vinculo sin borrar)          */
} inst_authz_t;

/* ---- Estado SEPARADO del flujo de solicitudes (Rev 5 §4.5) ---- */
typedef struct {
    request_t    reqs[MAX_REQUESTS];   int reqs_count;
    inst_authz_t auth[MAX_AUTHZ];      int auth_count;
} requests_state_t;

#endif /* EDUCHAIN_COMMON_H */
