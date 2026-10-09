# 00.37: VI, coordenadas de textura y cobertura alpha

La comparacion CPU/Vulkan del frame 2839 mostraba PAL estirado, contador de
monedas distinto y sprites cuadrados. Se guardaron vertices, materiales,
atlas y estado VI para investigar, ademas de capturas sin HUD del frontend.

## Cambios

- El puente RSP conserva s/t en dominio S10.5 duplicado. La captura dividia
  por 32 en lugar de 64: sprites llegaban a UV 1.9375 en lugar de 0.96875.
  Se corrigen triangulos RSP; rectangulos RDP conservan sus coordenadas.
- VI_ORIGIN puede seleccionar un buffer anterior. En el castillo, VI 0x38fa80
  frente a destino 0x3da800; la VI empieza una fila despues de 0x38f800.
  Se conservan tres imagenes GPU por direccion de framebuffer y se selecciona
  la pedida por VI. Se invalidan al cambiar modo, cargar estado o cerrar ROM.
  Si el buffer solicitado no esta disponible, se usa CPU.
- El escalado usa inicio, area activa y pasos VI, incluidos margenes PAL:
  640x288, inicio vertical 25, alto activo 237, pasos 512/1024. Mapas por fila
  y columna evitan divisiones por pixel. No implementa filtro VI completo.
- Se publica cvg_times_alpha: alpha cero descarta fragmentos sin escribir Z;
  alpha parcial se aproxima mediante mezcla Vulkan. Elimina cuadrados opacos
  de monedas/particulas aunque alpha_compare este desactivado. No completa
  las reglas de cobertura y blender N64.

## Evidencia local y limites

Frame 2839: error RGB medio de 38.50 a 14.85/255; pixeles con algun canal que
supera diferencia 16 pasan de 66.87% a 42.94%. Parte del error original era
el buffer VI incorrecto y el encuadre, no solo las texturas. El contador de
monedas coincide y los sprites dejan ver el fondo.

Validacion: ROM real con captura diferencial, audio, guardado/carga, reapertura
CPU/Vulkan; coordenadas RSP y rectangulos; cobertura sin alpha test y Z;
muestreo manual, atlas y glifos. Resultado final en build/hle-37/report.json.

Siguen diferencias: hueco negro en el borde superior izquierdo del castillo,
algunos materiales, resolucion GPU 256x256 y filtro VI/AA ausente. Angrylion
sigue rasterizando para referencia/fallback. No se promete mejora de FPS.
Lavapipe no valida comportamiento Liverpool; la PS4 debe confirmar el cambio.

Dumps *.bin: geometria recien dibujada, no necesariamente el buffer historico
seleccionado por VI. Metadata separa render_target y vi_origin. Las imagenes
CPU/Vulkan comparan la presentacion de esa VI.
