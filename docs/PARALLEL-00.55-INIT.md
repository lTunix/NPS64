# 00.55: preparacion de la prueba aislada

El log PS4 de 00.54 termina durante la preparacion de recursos/pipelines,
sin READY, BEGIN ni SUBMIT y sin ningun fence enviado por la prueba.
El usuario observa CE-34878-0. No se puede atribuir este cierre al bloqueo
de GPU de 00.53 ni precisar su causa con el log existente.

00.54 creaba los dos pipelines al abrir el contexto, antes de ejecutar el
caso seleccionado. 00.55 crea solamente el pipeline elegido al ejecutar
ese caso. Registra el inicio/final de compilacion, creacion de descriptor
layout, command pool y fence. La inicializacion y ejecucion se realizan
en un worker con pila explicita de 8 MiB; los errores de atributos,
creacion y join se registran y no continuan con un hilo sin esa pila.
La pila amplia es una mitigacion a comprobar, no una causa confirmada.

El shader, sus entradas y las referencias de mascaras no cambian.
En host pasan 128 fences, las comprobaciones de los 5 MiB, guardas y
validacion Vulkan/sincronizacion con creacion diferida de pipelines.
Las cabeceras y SPIR-V preservados se verifican por contenido y hashes
cuando el directorio logs ya no contiene las capturas originales.

Instalar `out/NPS64-00.55-parallel-tile-probe.pkg`, TITLE_ID NPSP00055.
Ejecutar primero el caso 2 seleccionado (64 lanes). Sigue siendo una
prueba de tiles, no un paquete para ejecutar ROMs. Recoger
`/data/NPS64-Parallel/tile-probe.log`; si vuelve a cerrarse, recoger
tambien `/data/NPS64/boot.log`. La estabilidad PS4 esta pendiente.

Reconstruccion con los dumps externos originales:

```bash
bash scripts/build_parallel_tile_probe.sh /work/capturas-originales
```

Si ya se generaron los datos y el manifiesto en build/parallel-tile-probe:

```bash
NPS64_TILE_USE_PREPARED=1 bash scripts/build_parallel_tile_probe.sh
```

Dentro del contenedor GPU, proyecto montado en /work. Los datos capturados
permanecen fuera de Git. CPU51 y los paquetes 00.53/00.54 se conservan.
