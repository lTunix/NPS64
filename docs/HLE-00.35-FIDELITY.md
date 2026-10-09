# 00.35: materiales de HUD, atlas comprobado y fallback completo

El usuario informa 00.34 en PS4: mosaicos/textos rotos y 20-40 FPS, CPU y
Vulkan similares. No se considera confirmado el arreglo visual de 00.34.
Angrylion sigue rasterizando todo: esta entrega no retira ese trabajo ni
promete una mejora de FPS.

Cambios:

- Ocho ecuaciones adicionales de material especializadas para modos vistos
  en Mario 64: SHADE/ENV alpha, mezcla por alpha TEXEL0, TEXEL0 con ENV/SHADE
  alpha, textura directa, modulacion con alpha de textura y color PRIM.
  Se comprueban con colores y alpha conocidos, no solo contador PASS.
- En PS4 se usa subida completa cuando cambia el atlas, hasta comprobar
  transferencias parciales en hardware. No se afirma que estas fueran la
  causa de los mosaicos. Host permite comparar con NPS64_FULL_ATLAS_UPLOAD.
- Lectura y comparacion de todos los bytes del atlas tras los primeros
  frames y cada 120 frames. Un mismatch detiene el modo y vuelve a CPU.
  El log informa live atlas verify, mismatch, primer byte y tipo de copia.
- Frame sin material/texel o con dibujos no capturados usa la imagen CPU
  completa. No se presenta geometria parcial con colores de diagnostico.
- Diagnostico distingue mux/ciclo en lugar de agotar sus muestras con el
  mismo mux y diferentes colores/scissor; incluye rectangulos.

Se conservan los controles: pausa L3+R3, Triangulo cambia CPU/Vulkan.
Audio, estados y fallback permanecen. Confirmacion visual de 00.35 en PS4
pendiente; Lavapipe no reproduce Liverpool.

Para retirar el rasterizado CPU (siguiente bloque pendiente):

1. Separar estado/comandos/TMEM del trabajo de spans de Angrylion.
2. Registrar destinos de framebuffer/Z y lecturas de RDRAM que dependen
   de dibujo, incluida carga de textura desde framebuffer.
3. Resolver esas dependencias con escritura GPU a memoria N64 o replay
   CPU antes de la lectura; conservar un fallback del frame completo.
4. Omitir spans solo en el camino respaldado por esas dependencias,
   conservando interrupciones DP y comandos de estado/carga.
5. Validar imagen y memoria contra CPU en intro/castillo, cambios de escena,
   estados y reapertura. Medir core_ms, GPU/presentacion y VI FPS en consola.

Desactivar handlers de dibujo sin lo anterior deja memoria obsoleta y
rompe efectos/transiciones. El trabajo no queda marcado terminado por el
hecho de mostrar una imagen GPU. El presupuesto tambien incluye Ari64,
readback y espera de fence; liberar rasterizado no garantiza 50 FPS.

Pruebas locales: ecuaciones nuevas y alpha, atlas completo/incremental,
1200 frames con cambios por todas las celdas, atlas ancho y vacios;
geometria completa/incompleta y orden HUD; Mario 64 PAL continuo exterior
del castillo con audio, toggles, pausa, estados y dos juegos/un contexto.

El log separa gpu_ms (captura/dibujo, fence y readback) dentro de draw_ms,
para contrastarlo con core_ms y rdp_ms. No son tiempos exclusivos de GPU.
