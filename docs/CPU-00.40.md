# CPU 00.40: perfiles y seis trabajadores

El ultimo tramo 00.39 de nps64.log contiene 101 ventanas de juego, 31 por debajo
de 45 VI/s. En esas ventanas: 39.90 VI/s, RDP 13.80 ms, core 17.29 ms,
VI 0.76 ms, draw 1.29 ms y flip 4.77 ms. RDP esta incluido en core_ms.

## Cambios

PGO LLVM 14 se aplica solo a n64video.o de Angrylion. Se mantienen O2, SSE2,
las reglas de rasterizado y el filtro VI. El perfil combina Mario 64 PAL
(4000 iteraciones, un trabajador para evitar carreras en los contadores)
y casos de sombreado, profundidad, textura y renderizado a textura.
assets/cpu-rdp-sm64.profdata contiene contadores, sin ROM ni imagenes.
scripts/train_cpu_profile.sh permite reconstruirlo con una ROM local.
El build rechaza perfiles de control de flujo desactualizados.

La configuracion predeterminada usa seis trabajadores del pool existente.
diagnostics.cfg conserva prioridad sobre ese valor. El escalador SSE2 de
00.39 sigue activo. Vulkan no se enlaza.

VideoOut copia mediante stores SSE2 streaming y sfence antes de presentar.
Conserva los dos buffers y la espera de flip previa a reutilizarlos.
Cada 60 muestras registra [scanout] wait_ms y copy_ms por separado.
Esta copia apunta a memoria WC de PS4: en memoria WB del PC fue mas lenta
que memcpy del SDK. Su beneficio real en consola aun necesita medicion.

## Validacion local

- 3952 hashes de imagen completa y 133 estados CPU/VI identicos a CPU39,
 tanto con cuatro como con seis trabajadores, en Mario 64 PAL.
- 48 casos RDP: framebuffer y profundidad identicos al renderer original.
- Copia de scanout: tamanos/alineaciones, canarios y ASan/UBSan aprobados.
- Backend PS4 simulado: contenido completo del scanout y protecciones de
 reutilizacion aprobados en cuatro variantes. No simula la GPU PS4.
- OpenOrbis: ELF/OELF validos; PKG de 6619136 bytes, 28 controles aprobados,
 payload extraido identico al stage.

## Rendimiento en Docker (no FPS de PS4)

Dos pares con orden invertido, PGO frente a O2, ambos con cuatro trabajadores:

| Medida media | Original | PGO | Original repeticion | PGO repeticion |
| --- | ---: | ---: | ---: | ---: |
| Tiempo total por iteracion (ms) | 5.671 | 5.505 | 5.550 | 5.369 |
| RDP (ms) | 3.055 | 2.970 | 3.090 | 2.911 |

Con el mismo core PGO, cuatro frente a seis trabajadores: RDP 2.672 -> 2.429 ms
(9.1% menos); total 4.805 -> 4.732 ms (1.5% menos); percentil 95 total
10.343 -> 9.359 ms. Casos sinteticos de relleno medio/grande mejoraron cerca
del 20% con seis trabajadores. Estos resultados no garantizan 50 FPS en PS4.
Las variantes O3, SSE4.1 y accesos manuales de 16 bits se descartaron por
rendimiento inconsistente; no estan en el paquete.

## Prueba en consola

Instalar out/NPS64-00.40-cpu.pkg. Inicio esperado:
NPS64 00.40 CPU ONLY core 890323d Ari64 / Angrylion PGO.
Revisar workers=6 requested_workers=6 en el log. Si una configuracion antigua
fuerza cuatro, /data/NPS64/diagnostics.cfg puede ajustarse a:

   NPS64DIAG5 6 1 1 1 0

Para una comparacion controlada con cuatro:

   NPS64DIAG5 4 1 1 1 0

Comparar la misma escena pesada del castillo, conservando el filtro VI.
Revisar speed_pct, rdp_ms, core_ms y [scanout] wait_ms/copy_ms. No sumar
rdp_ms a core_ms. Las versiones 00.13 y 00.39 se preservan para comparar.
