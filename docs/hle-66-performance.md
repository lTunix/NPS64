# HLE 00.66: preparación de comandos y medición

Se omiten binds de pipeline, constantes y scissor que no cambian entre dibujos
consecutivos. El orden, los materiales, las coordenadas y la resolución siguen
siendo los mismos. La CPU de referencia continúa rasterizando; este cambio no
es una implementación de rasterizado exclusivamente GPU.

Pruebas locales: 1200 cuadros del renderer persistente y 2600 cuadros de Mario
con comparaciones, audio, pausa, estados y alternancia CPU/Vulkan. El PKG físico
y el adaptado a shadPS4 pasan 28 comprobaciones de contenido y extracción.

## Medición en shadPS4

`scripts/test_hle_shad.ps1` ejecuta el eboot extraído del PKG adaptado, abre Mario,
activa Vulkan, deja terminar el calentamiento y mide 60 segundos. No salta
cuadros ni cambia la velocidad de emulación. Archiva log y JSON; detiene solo
el proceso que lanzó. Requiere el runtime aislado ya preparado en `build`.

Se compiló shadPS4 sin las capturas y esperas de diagnóstico de ParaLLEl. Se
conserva la descarga del framebuffer GPU real hacia la memoria visible por el
guest, necesaria para presentar el resultado. El script de compilación restaura
el archivo fuente original al terminar y genera un ejecutable separado.

Cabeza de Mario, 60 muestras: **49,842 FPS promedio**, mínimo **44,7**, 58 muestras
de 60 con al menos 48 FPS. GPU promedio **3,373 ms**. Con el ejecutable que
capturaba diagnósticos: promedio 49,274 y GPU 4,919 ms; estas cifras no aíslan
el efecto del cambio en el renderer, porque también cambia el emulador.

Resultados: `build/hle-66-clean-performance.json` y `build/hle-66-clean-core.log`.
`-EnterGame` añade Start y confirmaciones para probar selección e introducción.
Estos números corresponden al PC bajo shadPS4 con intérprete cached, no a la
PS4 física ni a todos los niveles. No se alcanzó una garantía de 48 FPS mínimos.

Selección e introducción (`hle-66-game`): 59 muestras, **49,949 FPS promedio**,
mínimo **47,87**, 58 muestras con al menos 48 FPS y **3,136 ms GPU promedio**.
La sesión completó más de 2280 cuadros GPU y 16 fallbacks. Terminó por orden
del runner, sin cierre inesperado. El recorrido no certifica un nivel completo.

## Pendientes gráficos y de CPU

- Resolver deslizamiento de texturas con movimiento de cámara: auditar UV,
  perspectiva y estado de tiles contra el RDP CPU.
- Recuperar detalles faltantes y sustituir el framebuffer de prueba 256×256 por
  dimensiones correctas del render target/VI, con pruebas de bordes y recorte.
- Quitar rasterizado CPU duplicado solo después de asegurar lecturas de RDRAM,
  efectos de framebuffer y recuperación del fallback. Saltar primitivas sin
  resolver esas dependencias rompería las escenas y los cambios a CPU.
- Medir un recorrido jugable del castillo y repetir en consola física.
