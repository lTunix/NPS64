# ParaLLEl 00.52: aislar compute en PS4

Primer bloque de la rama experimental. No es todavia un renderizador de
juegos ni una mejora de FPS: verifica las operaciones GPU antes de volver
a integrar ParaLLEl-RDP original. La version CPU51 permanece intacta.

## Instalar y probar

Paquete: `out/NPS64-00.52-parallel-compute-probe.pkg`.
TITLE_ID: `NPSP00052`; se instala como una app distinta.

1. Abrir la app; inicialmente no envia trabajo Vulkan.
2. Pulsar X en el caso 1 y esperar PASS.
3. Elegir el siguiente caso con arriba/abajo y pulsar X; repetir hasta el 7.
4. Recoger `/data/NPS64/parallel-probe.log` y `/data/NPS64/boot.log`.
   Ante un fallo, el ultimo SUBMIT identifica caso y repeticion.
5. Circulo sale de la app.

Cada caso ejecuta 64 envios. Cada envio contiene dos despachos y espera
un fence antes de leer resultados. Se verifican todas las palabras,
guardas y argumentos indirectos, con valores distintos por repeticion.

| Caso | Operacion |
| --- | --- |
| 1 | Buffer 1D, grupo local de 64 |
| 2 | Buffer 2D, grupos de 8 x 8 |
| 3 | Buffer 3D, grupos de 4 x 4 x 4 |
| 4 | LDS, atomicOr, barreras, grupo de 32 |
| 5 | LDS, atomicOr, barreras, grupo de 64 |
| 6 | Cambio de pipeline y lectura de una escritura anterior |
| 7 | Despacho indirecto 2D y dependencia lectura/escritura |

COMPLETE se registra solo despues del fence y la comprobacion de memoria.
WRITE_DATA por si solo no demuestra que un shader termine. El timeout es
cinco segundos; un bloqueo fisico del controlador puede seguir congelando
la consola. Tras un fallo no se envian mas comandos. Se retiene el dispositivo
hasta salir para evitar la destruccion que fallo en las pruebas anteriores.

## Reconstruccion y evidencia local

En el entorno Docker preparado del proyecto:

```powershell
./scripts/build_parallel_probe.ps1
```

Requiere las dependencias GPU y Mesa PS4 ya construidas. El shader se
compila y valida como SPIR-V Vulkan 1.1. Los encabezados generados y los
artefactos quedan en `build/parallel-probe/`, excluido de Git.

Lavapipe paso siete casos: 448 envios, 896 despachos, verificacion exacta
con validacion Vulkan y de sincronizacion, sin errores reportados.
El PKG paso 28 controles y su payload coincide con el stage independiente.
Tambien se verifico el paquete CPU51 existente: hash y payload conservados.

Docker no reproduce la GPU Liverpool ni el controlador de PS4. Estos
resultados no prueban estabilidad ni velocidad en consola. Falta validar
compute sobre texturas, cargas reales RDP, fidelidad y sesiones de juegos.

SHA256 del PKG:
`ff1937b45ce018914c56dfcbabc20e469f91c468e51615ee21fc09abf4356de5`.
