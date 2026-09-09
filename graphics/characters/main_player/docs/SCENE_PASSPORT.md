# Scene Passport — ASTRAEON Main Player

Estado: preproducción técnica; proxies explícitos, no personaje final. 2026-09-07.

refs_read: blender-scene, blender-scene-spec, blender-modeling, blender-lookdev,
blender-lighting-camera, blender-animation, blender-generation, blender-volatile,
blender-audit-finalize. Recursos del servidor `higgsfield_bridge`, `skill://blender/`.

## SCENE
- intent: protagonista explorador/científico humano, masculino, 30 años, cuerpo atlético moderado.
- deliverable: fuente modular animada y exports Unreal 5.7.4; el proxy sólo fija proporciones.
- units: metres; axes: right-handed Z-up; frente -Y, izquierda anatómica +X.
- render: BLENDER_EEVEE, 900×1100, 30 FPS, frame range inicial 1–61.
- dynamic: yes; acciones esqueléticas separadas; no turntable sustitutivo de locomoción.

## HIERARCHY
- collection/object naming: ASTRAEON_CHARACTER; GEO/BODY, SUIT, HELMET, GEAR; RIG,
  ANIMATIONS, ATTACHMENTS, COLLISION_HELPERS, EXPORT, TEST; PROXY_* sólo para andamiaje.
- parent/child relationships: futuro armature único; casco sobre head, equipo sobre sockets.
- protected existing objects: escena Scene y sus Cube, Light, Camera se preservan intactos.
  La escena nueva CHR_Astraeon_Player_Work contiene exclusivamente trabajo de este lote.

## ASSETS
- A01 Player | [GEN] | detailed | altura corporal 1.83 m, hombros ~0.48 m,
  profundidad torso ~0.26 m, envolvente en A-pose ~1.25 m | location [0,0,0]
  | orientation [0,0,0] | origen suelo/root | futuro armature | protagonista.
- A02 Helmet | [BLOCK] construcción nativa a refinar | detailed | envolvente objetivo
  [0.28,0.31,0.30] m | centro [0,-0.025,1.735] | [0,0,0] | collar/head | casco modular.
- A03 Backpack | [BLOCK] construcción nativa a refinar | detailed | [0.29,0.12,0.39] m
  | [0,0.20,1.31] | [0,0,0] | back_attach | soporte vital compacto.
- A04 WristComputer | [BLOCK] construcción nativa a refinar | detailed | [0.07,0.055,0.09] m
  | ajustado al antebrazo izquierdo después de rig | tool/forearm | escáner/mapa.
- A05 PROXY_* | [BLOCK] | blockout | mismas envolventes A01–04 | suelo/root
  | ninguno | validación de escala, silueta y espacios; excluido del export final.
- generation estimate/submission state A01: pendiente; bl_estimate_generation ausente.
  No job enviado, no créditos gastados. Solicitud concreta en GENERATION_REQUEST.json.

## SHOT
- active camera: CAM_Player_Front; adicional CAM_Player_Side.
- framing/lens/target: ortográfico 2.15 m, objetivo [0,0,0.96]; frente y perfil para QA.
- foreground, subject, background depth: suelo Z=0, personaje centrado, world gris neutro.

## LOOK
- material roles and palette: blanco técnico, navy/grafito, bronce mínimo, cian funcional;
  piel y cabello castaño. Color plano de proxy, sin atribuirle PBR final.
- material route: NONE durante proxy; materiales nativos y texturas generadas inspeccionadas
  al importar la base. Sets finales Character/Suit/Gear/Helmet.
- texture scale: detalle de tejido milimétrico, UV productivas pendientes.
- relief polarity: normal/bump fino; pliegues mayores y placas en geometría.
- dielectric/metallic: tejido/piel/polímero dieléctricos; sólo bronce metálico.
- world/background: estudio neutral, sin HDRI generado.

## LIGHTING
- focal subject: proporciones corporales; secondary: separación brazos/torso;
  darkest region: cara interior de extremidades, sin ocultar articulaciones.
- reference mood: iluminación técnica de estudio.
- environment route: NONE; true HDR-EXR required: no.
- world-only and zero-world baseline: pendientes de etapa lookdev, proxy usa Workbench.
- key: futura área amplia frontal/lateral para leer anatomía y tejido.
- fill target: conservar navy legible, sin borrar relieve.
- motivation: estudio de inspección, sin luminarias ambientales ficcionales.
- reflection strategy: bandas amplias sobre visor y pequeñas sobre placas/metal.
- gobos/flags/atmosphere: ninguno en QA de geometría.
- color management: AgX, exposure 0 para lookdev; Standard para Workbench de proxy.

## MOTION
- fps/frame range: 30; rangos por acción al construirlas, idle objetivo 1–61.
- beats: locomoción esquelética con contactos; interacción anticipación/acción/reposo.
- rest poses: A-pose con dedos separados; root al suelo. Ciclos in-place, raíz preparada
  para variantes root motion posteriores; sin escalar el objeto durante clips.

## ACCEPTANCE
- structural: 1.83 m sin casco, escala 1, nombres estables, piezas modulares, rig limpio,
  dedos completos, pesos normalizados, UV y fuentes presentes, export/import medidos.
- motion: acciones solicitadas por separado, deformaciones y contactos vistos por clip,
  loops sin salto, pruebas con props, IK manos/pies; no PASS por simple presencia de keys.
- visual: rostro humano creíble; explorador antes que soldado; anatomía natural;
  traje articulable sin interpenetraciones graves. Los proxies nunca pasan esta puerta final.
- lighting: estudio legible, sin clipping ni texturas ausentes; vidrio revisado en Unreal.

## Gates / tareas
- [x] Leer módulos indicados.
- [x] A: inspección de host/escena, unidades y contrato.
- [x] B: checkpoint previo; escena anterior protegida.
- [ ] C: silueta → proporción → profundidad → contacto → camera read.
- [ ] D: generar/importar/normalizar base; modelar módulos; rig/skin/animación/lookdev.
- [ ] E: auditoría estructural, visual, motion y export Unreal.

No cambiar gameplay, cápsula, paquetes Q1 ni toolchain durante este lote offline.
