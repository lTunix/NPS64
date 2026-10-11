# ParaLLEl 00.62: integrar el binning aprobado

El usuario confirmó el caso 8 en PS4: 64 envíos completados, con conteos
0..256 y patrones de scissor correctos. La versión 00.62 incorpora ese
shader al renderizador de juegos. Conserva grupos de 32 hilos, lecturas
volátiles y atómicos de máscaras gruesas; predica el rechazo vertical y
elimina las escrituras exclusivas del diagnóstico. El rasterizador,
texturas, color y VI siguen perteneciendo a ParaLLEl-RDP.

Los cambios se aplican a copias de compilación, sin modificar el núcleo
original ni la rama main. El título NPSP00062 se instala separado de CPU.

Validación local: 64 dispatches del shader de producción, máscaras,
entradas y guardas correctas, sin errores de validación Vulkan. PKG y
payload verificados con 28 comprobaciones; paquete CPU 00.51 intacto.

Mario PAL completó 650 cuadros RDP locales. Las capturas VI 150, 300,
480 y 600 se compararon con Angrylion reproduciendo los mismos comandos:
640x288, estado VI coincidente y error máximo de 1/255 por canal en las
cuatro imágenes. Se revisó visualmente la comparación del cuadro 600.
Artefactos: `build/parallel-comparison/mario-62-stream/index.html` y
`frame-600-comparison.png` (CPU, Vulkan, diferencia amplificada x8).

La ejecución local usa Lavapipe. No reproduce la GPU de PS4 ni permite
predecir sus FPS. La aprobación del caso 8 no demuestra todavía que todas
las etapas de renderizado de juegos funcionen en consola.

- [x] Integrar el shader aprobado y retirar la instrumentación de prueba.
- [x] Validar el shader de producción con la referencia de máscaras.
- [x] Compilar y verificar el PKG separado.
- [x] Comparar imágenes reales con CPU usando los mismos comandos RDP.
- [ ] Validar ROM, cambios de escena y audio en PS4.
- [ ] Medir rendimiento en PS4 antes de reducir la sincronización conservadora.

Paquete: `out/NPS64-00.62-parallel-rdp-test.pkg`.
Log de juego: `/data/NPS64-Parallel/nps64.log`.
