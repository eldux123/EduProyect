# Guion de la llamada — EduChain (≈10 min)

> Ten abierto ANTES de empezar: (1) el editor con el proyecto, (2) una terminal WSL Ubuntu ya en
> `cd /mnt/c/Users/erick/Downloads/Blockchain/EduProyect`, (3) este guion.
> Comparte pantalla completa (así puedes cambiar entre editor y terminal).

---

## 0. Antes de la llamada (ensayo, 5 min)
Prueba una vez que la GUI abre:
```bash
cd /mnt/c/Users/erick/Downloads/Blockchain/EduProyect
bash verification/gui.sh credentials
```
Si abre la ventana y al final la consola dice `Proved goals: 362 / 382`, estás listo. Ciérrala con
`bash verification/gui_close.sh`. Lo demás (demo, tests) nunca falla.

---

## 1. Qué es (2 min) — comparte el README.md
**Decir:**
> "EduChain es una simulación en C de la lógica de un contrato inteligente. Modela el registro de
> profesionales, su experiencia y las credenciales, con reglas de seguridad. Lo importante: no solo
> programé esas reglas, las **verifiqué formalmente** con Frama-C. Aclaro que **no es una blockchain
> real**: no hay red ni criptografía, es un modelo académico."

Muestra la tabla de módulos del README (6 módulos).

---

## 2. Demo en vivo (2 min) — en la terminal
```bash
bash run_demo.sh
```
**Decir, señalando los números:**
> "Cada número es un código de retorno. Aquí un profesional intenta registrarse a sí mismo: `-2`,
> permiso denegado. Aquí alguien intenta validar su propia experiencia: `-3`, rechazado. Y aquí se
> intenta reactivar una credencial revocada: `-9`, porque REVOKED es un estado terminal. El sistema
> rechaza los abusos."

---

## 3. Pruebas (1 min) — en la terminal
```bash
bash run_tests.sh
```
**Decir:**
> "241 pruebas, 0 fallos. Incluye pruebas adversariales que intentan romper cada regla, y corre bajo
> sanitizadores de memoria (ASan/UBSan) sin errores."

---

## 4. Verificación formal — la parte central (3 min)

Primero enseña un contrato en el editor: abre `include/security.h`, señala el bloque `/*@ ... */`.
**Decir:**
> "Esto son las anotaciones ACSL: precondiciones, postcondiciones y qué puede modificar cada función.
> Son propiedades matemáticas, no código. Frama-C las demuestra."

Luego en la terminal:
```bash
bash verification/gui.sh credentials
```
Espera a que la barra verde **WP** (abajo izq.) termine. Abre el panel **WP View** (lista de la
izquierda). **Decir:**
> "Cada línea es una obligación de prueba. En verde, demostradas matemáticamente para TODAS las
> entradas posibles, no solo para los casos que probé. Eso es lo que da la verificación formal frente
> a las pruebas."

*(Si tarda, mientras compila sigue hablando del contrato en el editor.)*

---

## 5. Honestidad (1 min) — comparte docs/formal_verification.md
**Decir:**
> "En total, 1537 de 1620 obligaciones demostradas. Las que no cierran están documentadas con su
> causa: son límite del probador sobre una estructura grande, no errores. Lo verifiqué: no hay
> contraejemplos. No declaro verificado nada que no esté demostrado."

---

## 6. Cierre (30 seg)
> "Resumiendo: un núcleo en C con reglas de seguridad, probado dinámicamente con 241 tests y
> **verificado formalmente** con Frama-C. El valor no es un número perfecto: es poder defender cada
> afirmación que hago sobre el sistema."

---

## Si preguntan... (respuestas rápidas)

- **"¿Por qué C y no Solidity?"** → "El objetivo era la verificación formal con Frama-C, que opera
  sobre C. No construir una blockchain, sino demostrar la lógica."
- **"¿Está todo verificado?"** → "No, y no lo afirmo: 1537 de 1620. El resto está documentado como
  pendiente con su causa. Prefiero eso a un falso 100%."
- **"¿Dónde está la verificación formal?"** → "En las anotaciones ACSL de los headers (`/*@ requires
  / ensures / assigns */`) y los invariantes de lazo. Frama-C las demuestra con el motor WP."
- **"¿Qué seguridad tiene el contrato?"** → "Cinco capas: autorización por rol, separación de deberes
  (nadie valida lo propio), validación de entradas, máquina de estados con transiciones legales, y
  atomicidad en error; todo con auditoría append-only." *(detalle en guia_sustentacion.md)*
- **Un rojo en la GUI** → "No lo cierra el solver por el tamaño de la estructura; la propiedad es
  cierta y está respaldada por los tests. No es un contraejemplo."

---

## Comandos a la mano (copiar/pegar)
```bash
cd /mnt/c/Users/erick/Downloads/Blockchain/EduProyect
bash run_demo.sh                       # demo
bash run_tests.sh                      # 241 pruebas
bash verification/gui.sh credentials   # GUI con WP
bash verification/gui_close.sh         # cerrar la GUI antes de abrir otro módulo
bash verification/gui.sh requests      # (opcional) módulo grande
```
