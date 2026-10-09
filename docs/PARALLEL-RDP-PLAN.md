# Rama experimental: ParaLLEl-RDP en PS4

Rama: `experimental/parallel-rdp-ps4`.
Base: `main`, CPU 00.51, commit `143dfd7`.

Objetivo: ejecutar ParaLLEl-RDP original con precision grafica y medir si
el trabajo trasladado a la GPU mejora el rendimiento real de PS4. La rama
comienza con el mismo codigo ejecutable de `main`; este documento define
las pruebas pendientes y no habilita Vulkan en el build CPU.

## Reglas de trabajo

- Mantener Angrylion como referencia y opcion estable.
- Separar builds y paquetes experimentales de los productos CPU.
- No reemplazar ParaLLEl-RDP por el renderizador HLE construido previamente.
- Mantener resolucion nativa y comparar contra CPU antes de optimizar.
- Cambiar una condicion del controlador o del shader por experimento.
- Docker valida compilacion y resultados locales; PS4 valida GPU y velocidad.
- No incorporar ROMs, capturas, logs, SDKs, dependencias ni PKGs a Git.

El antecedente es el bloqueo de compute en el envio 37 mientras los envios
graphics completaban correctamente. El marcador WRITE_DATA posterior a
un DISPATCH solo demuestra que el procesador de comandos lo alcanzo;
no demuestra que el shader haya terminado. Se deben usar fences y barreras
apropiados para atribuir el bloqueo a una operacion concreta.

## Primer entregable: 00.52

La app independiente implementa siete casos compute y pasa 448 envios en
Lavapipe con validacion Vulkan y sincronizacion. El PKG no carga juegos;
el log de PS4 confirma los siete casos, 448 envios y 896 despachos completos.
Ver [instrucciones](PARALLEL-00.52-COMPUTE.md).

00.53 integra juegos completos y compara siete imagenes de Mario, cabeza y
menu, sobre el mismo stream RDP: error maximo 1 por canal. No confirma aun
rendimiento ni estabilidad del juego completo en PS4. Ver
[entrega experimental](PARALLEL-00.53-GAMES.md).

## Bloque 1: pruebas minimas de compute

- [ ] Revisar y reutilizar `tests/gpu_compute_probe.*` y las herramientas de captura.
- [ ] Confirmar escritura y lectura GPU/CPU de buffers con fences completados.
- [ ] Validar descriptores, direcciones, alineacion y vida util de recursos.
- [ ] Probar dimensiones 1D, 2D y 3D, varios grupos y distintos tamanos locales.
- [ ] Probar texturas, LDS, barreras de grupo y atomicos por separado.
- [ ] Probar DISPATCH_INDIRECT despues de confirmar despachos directos.
- [ ] Repetir las cargas y registrar errores o bloqueos sin anunciar exito parcial.

Criterio: resultados esperados leidos tras completar la GPU en PS4 real.
Un triangulo graphics o una prueba exitosa en PC no cumple este criterio.

## Bloque 2: reproducir RDP fuera del juego

- [x] Capturar comandos RDP y estado de memoria de escenas pequenas de Mario 64.
- [x] Reproducir identica entrada con Angrylion y ParaLLEl-RDP.
- [ ] Reducir el primer bloqueo a una captura y un shader concretos.
- [ ] Registrar por etapa preparacion, binning, rasterizado, resolve y VI.
- [ ] Verificar el arreglo sobre capturas distintas y sesiones consecutivas.

Las capturas permanecen en `build/` o fuera del repositorio. Los scripts,
manifiestos sin contenido de ROM y pruebas sinteticas pueden versionarse.

## Bloque 3: fidelidad grafica

- [ ] Comparar color, profundidad, cobertura y filtrado VI a resolucion nativa.
- [ ] Validar TMEM, formatos de textura, paletas, combiner y transparencia.
- [ ] Incluir textos, particulas, render-to-texture y cambios de framebuffer.
- [ ] Explicar cada diferencia; no aceptar solo siluetas o geometria correcta.

Criterio: las capturas verificadas mantienen resultados fieles a la referencia;
cualquier diferencia conocida queda documentada con una reproduccion.

## Bloque 4: integracion continua

- [x] Integrar ParaLLEl-RDP y un RSP compatible en un build experimental.
- [ ] Validar RDRAM compartida, lecturas CPU de framebuffer y sincronizacion.
- [ ] Revisar presentacion sin lectura GPU->CPU obligatoria de cada cuadro.
- [ ] Validar inicio, cambios de escena, pausa, cierre y multiples sesiones.
- [ ] Mantener audio e input mientras el backend GPU procesa cuadros.

No se promete recuperar la aplicacion con un fallback CPU si el controlador
queda bloqueado: el fallback solo es viable mientras el dispositivo responde.

## Bloque 5: rendimiento en consola

- [ ] Recorridos comparables de Mario 64, Zelda, Tennis y GoldenEye.
- [ ] Comparar VI/s y velocidad emulada, tiempos CPU/GPU, esperas y copias.
- [ ] Comparar cargas iniciales, compilacion de shaders y rendimiento sostenido.
- [ ] Verificar continuidad del audio y revisar escenas graficamente exigentes.
- [ ] Habilitar optimizaciones opcionales una por una y repetir fidelidad.

Objetivos de referencia: 50 VI/s para las ROMs PAL y aproximadamente
60 VI/s para GoldenEye USA. Estos valores no representan necesariamente
la frecuencia interna con que el juego dibuja imagenes nuevas.

## Incorporacion futura

- [ ] Ofrecer seleccion explicita CPU/GPU y guardar la preferencia por juego.
- [ ] Documentar requisitos, limitaciones y resultados reales de PS4.
- [ ] Revisar los cambios antes de integrarlos en `main`.

ParaLLEl-RDP y requisitos del controlador:
https://github.com/Themaister/parallel-rdp

Investigacion previa del proyecto:
[GPU](GPU.md), [investigacion](GPU-INVESTIGATION.md) y
[validacion](VALIDATION.md).
