# Guía rápida — GUI de Frama-C (Ivette) para la sustentación

## Lanzar (terminal WSL Ubuntu)
```bash
cd /mnt/c/Users/erick/Downloads/Blockchain/EduProyect
bash verification/gui.sh credentials     # un módulo
bash verification/gui.sh requests        # el módulo grande (tarda más: 766 obligaciones)
bash verification/gui.sh                  # los 6 módulos juntos
bash verification/gui.sh --eva            # análisis de valores (Eva)
```
Espera a que termine: abajo a la izquierda la barra **WP** verde deja de girar y en **Console**
aparece `Proved goals: N / M` y `[server] Server running.`

> **IMPORTANTE — solo UNA ventana a la vez.** Ivette no permite dos instancias simultáneas; la
> segunda falla con `Cannot load window URL / ERR_FAILED`. Para ver otro módulo, **cierra primero**
> la ventana anterior (botón X) o ejecuta:
> ```bash
> bash verification/gui_close.sh     # cierra toda instancia de la GUI
> bash verification/gui_status.sh    # muestra si hay procesos de la GUI vivos
> ```
> Entonces ya puedes lanzar el siguiente módulo.

> Si abajo dice solo **"UN"** y los paneles están vacíos → la GUI arrancó SIN análisis.
> Ciérrala y relanza con `bash verification/gui.sh credentials` (con el argumento).

---

## Orden de paneles a mostrar (lista "Views", a la izquierda)

### 1. Console — el titular
Señala la línea `Proved goals: 362 / 382` (credentials) · `739 / 766` (requests).
**Decir:** "Frama-C demostró estas obligaciones de prueba. Los números coinciden con lo documentado."

### 2. WP View — el corazón visual
Clic en **WP View**. Lista cada obligación con color:
- **verde = demostrada** · **rojo = Failed** · **naranja = Timeout**.
**Decir:** "Cada propiedad de cada función es una obligación. La mayoría en verde: demostradas
matemáticamente para todas las entradas, no solo para casos de prueba."

### 3. Source Code — contratos ACSL sobre el código
Clic en **Source Code** y luego en una función, p. ej. `credential_revoke`.
Verás sus `requires` / `ensures` / `assigns` con ✓ o ✗ al lado.
**Decir:** "Esto es la especificación formal: precondiciones, postcondiciones y qué puede modificar.
WP prueba que el código las cumple."

### 4. Properties — tabla de propiedades
Clic en **Properties**. Lista todas las propiedades y su estado de prueba. Útil para filtrar.

### 5. (Opcional) Eva — lanzar con `--eva`
Paneles **Eva Summary** / **Eva Values**: muestran los rangos de valores calculados y
`0 alarms`. **Decir:** "Eva confirma ausencia de errores de memoria/overflow en las trazas."

---

## Cómo explicar un ROJO (Failed/Timeout) si preguntan

Abre **WP View**, selecciona un rojo de `cred_transition` o `credential_issue`:

> "Esta obligación no la cierra el solver porque la prueba debe atravesar una llamada de auditoría
> sobre una estructura de ~7 KB, y el probador no encadena el marco automáticamente. La propiedad
> **es cierta** —lo verifiqué con smoke-tests, no hay contraejemplo— y está respaldada por las
> pruebas dinámicas. No la declaro demostrada porque formalmente no lo está."

Esta respuesta demuestra rigor. Es más fuerte que un falso "todo verde".

---

## Avisos del panel Messages (si los ven)

`annot:missing-spec` en `security.h`, `audit.h`, `professionals.h`:
**no son errores.** Al analizar un módulo aislado, WP usa los *contratos* de las funciones externas
(no su código). Es el funcionamiento normal de la **verificación modular**.

---

## Frases de cierre

- "WP **demuestra**; Eva **analiza**; los tests **observan**. Distingo siempre los tres."
- "1537 de 1620 obligaciones demostradas; lo pendiente está documentado con su causa."
- "El valor no es un número perfecto: es poder defender cada afirmación."
