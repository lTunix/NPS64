# ParaLLEl 00.59: comparar actualizaciones CPU y GPU

## Resultado real de 00.58

El caso 5 completó las esperas GPU para 0, 1, 31, 32, 33, 63, 65, 255 y
256 primitivas, con hasta ocho grupos concurrentes. El primer patrón de
scissor (pantalla completa) pasó. La primera prueba con una primitiva
y scissor de cuadrante falló en la comparación de datos, **sin bloqueo**:

```text
repeat=10 word=802816 actual=00000001 expected=00000000
```

La palabra corresponde al buffer de máscaras, tile x=0, y=16, grupo 0.
Ese tile queda fuera del scissor superior de 120 píxeles. El resultado
es compatible con datos obsoletos o un problema de cálculo/visibilidad;
sin captura de buffers no permite distinguir estas causas. No confirma
que los atómicos sean incorrectos: los ocho grupos anteriores pasaron.

## Nuevo caso 6

Se mantiene la prueba anterior para comparación. El caso 6 utiliza el
mismo SPIR-V, entradas, conteos y referencia. Cambia la publicación de
las entradas: después de copiar a memoria CPU, también las sube con
`vkCmdUpdateBuffer` y una barrera TRANSFER_WRITE -> SHADER_READ antes
del despacho. Los buffers declaran TRANSFER_DST. Los datos de comandos
se copian al grabarlos, antes de cualquier modificación posterior.

No se sustituyen las máscaras esperadas ni se relaja su comprobación.
Si la comparación falla, se guardan las cinco asignaciones completas en
`/data/NPS64-Parallel/tile-fail-caseN-bufferB.bin` (5 MiB en total), después
de completar el fence. Los logs informan cuántos bytes se escribieron.

El propósito es aislar la actualización/visibilidad de entradas en PS4.
Todavía no se declara corregido el driver ni se modifica la ejecución
de ROMs. La referencia local ya pasó 384 envíos (seis casos), con todos
los conteos y patrones para ambas formas de subir datos, sin errores
de validación Vulkan. RADV/ACO BONAIRE/GFX7 compiló los seis casos.

Instalar 00.59 y ejecutar **caso 6**, seleccionado al inicio. Conservar
`boot.log`, `tile-probe.log` y, si falla, los cinco archivos `tile-fail*`.
