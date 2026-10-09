# Cell64 y NPS64: investigación del 5 de octubre de 2026

## Alcance y límites

ZeBobbel confirma que Cell64 deriva de un antiguo emulador de N64 para PS3 y que sigue usando un intérprete: sus intentos de implementar JIT aún no funcionaban al anunciar 1.60. No se encontró un repositorio público de su versión actual en las búsquedas realizadas. No se inspeccionó el ejecutable de Cell64 ni se ejecutó en PS3. No está demostrado qué renderer concreto conserva o qué partes modificó.

Fuentes primarias:

- [Presentación de ZeBobbel](https://psx-place.com/threads/hello-humans-im-new-to-psx-place-not-new-to-the-ps3-tho-currently-working-on-voxel-engines-emulators.51048/).
- [Actualización 1.60 del autor](https://psx-place.com/threads/cell64-1-60-n64-emulator-progress.51220/).
- [Código público del antiguo Wii64 para PS3](https://github.com/emukidid/wii64-ps3).

Se descargó ese último código en `reference/wii64-ps3`, revisión `3e59958d54ed36fc2242be571e87c1cb05136e24`. Es una referencia plausible del proyecto antiguo mencionado por el autor, no una identificación confirmada del árbol exacto de Cell64.

## Qué demuestra el código antiguo

`Makefile.ps3` compila Mupen/Wii64, RSP HLE y glN64_GX para la PPU, enlazando libRSX y GCM de PSL1GHT. El README declara intérprete puro y gráficos acelerados preliminares. Aunque el Makefile contiene macros de dynarec heredadas, eso no prueba que el recompilador funcionara en PS3.

La rama PS3 de `glN64_GX/OpenGL.cpp` utiliza `rsxDrawVertexBegin`, atributos de color, coordenadas de textura y posiciones, y `rsxDrawVertexEnd`. Dibuja triángulos, líneas y rectángulos con el rasterizador gráfico de RSX. No rasteriza esos elementos mediante los despachos compute de paraLLEl-RDP.

`shaders/combined_shader.vcg` transforma posiciones con matrices. `combined_shader.fcg` ofrece modos de textura, color y modulación de ambos. `Combiner.cpp` selecciona TEXTURE_ENV en PS3 y deja comentado el combiner RSX más elaborado. Esto demuestra una aproximación gráfica sencilla en el port antiguo; no demuestra que Cell64 actual conserve exactamente esas limitaciones. No se observó una ruta SPU en los objetivos y subsistemas examinados; no puede afirmarse que Cell64 utilice SPU sin su código.

## Comparación con el código de NPS64

| Componente | Cell64 confirmado / port antiguo examinado | NPS64 PS4 |
|---|---|---|
| CPU N64 | Cell64: intérprete, según su autor | Ari64 recompila a x86-64 cuando la memoria ejecutable está disponible; alternativa con caché |
| Núcleo | Cell64: deriva de un emulador antiguo; Wii64-PS3 examinado: Mupen/Wii64 | paraLLEl-N64, revisión 890323d |
| RSP | Port antiguo: HLE; Cell64 actual no verificado | HLE en la configuración actual |
| Gráficos GPU | Port antiguo: glN64 adaptado a RSX, triángulos y shaders de vértices/fragmentos | paraLLEl-RDP, rasterización mediante compute Vulkan |
| Controlador | Port antiguo: PSL1GHT, libRSX/GCM | OpenOrbis y Mesa RADV/ACO adaptado a GNM |
| Presentación | Port antiguo: ruta gráfica RSX | Copia de scanout a buffer CPU, composición en canvas y envío por VideoOut |
| Alternativa CPU | El port antiguo incluye también un objetivo soft | 00.13: Ari64 + Angrylion, probado por el usuario |

NPS64 GPU procesa spans, binning por tiles, rasterización, mezcla/profundidad y scanout con una cadena de recursos y sincronización distinta. En `src/gpu_vulkan.c`, `nps64_gpu_readback` copia la imagen a un buffer y espera una fence antes de entregar los píxeles; `src/platform_ps4.c` copia el canvas al buffer de presentación y hace el flip. Hay trabajo CPU y espera por frame aun con GPU activa.

## Consecuencias para nuestro bloqueo

Que un renderer RSX pueda ejecutar juegos no valida nuestro controlador compute ni su compilador GFX7. La captura actual muestra que la submission 37 no finaliza: llega el marcador 8, pero no el 9 posterior al flush compute. La variante sin texturas redujo código y registros y volvió a fallar. Estos datos no identifican por sí solos el acceso o la instrucción culpable.

Una alternativa inspirada en el port antiguo sería mantener Ari64 y RSP HLE, añadir un renderer HLE que traduzca los comandos gráficos a triángulos, texturas y shaders de vértices/fragmentos, y presentar sin el readback actual. Puede evitar esa carga compute concreta, pero requiere un backend para PS4, gestión de memoria, compilación de shaders, sincronización y pruebas reales. No basta con copiar llamadas RSX ni cambiar el nombre del renderer. Si utiliza RADV, parte del controlador problemático seguirá compartida. No se puede prometer estabilidad ni mayor rendimiento antes de medirlo.

El costo de ese enfoque es implementar y verificar combinadores, modos de mezcla, profundidad, texturas y framebuffer. paraLLEl-RDP busca reproducir esas operaciones con más detalle; reemplazarlo por HLE exige evaluar compatibilidad juego por juego. Otra línea independiente es mejorar Angrylion, que ya funciona en esta PS4, mientras se aísla el fallo de compute con pruebas pequeñas que separen lecturas, escrituras y tipos de acceso.

La investigación no modifica los renderers ni genera un nuevo PKG. Es una comparación documentada, con el código antiguo disponible para futuras consultas.
