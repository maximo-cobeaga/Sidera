# Protagonist completion — 2026-09-08

SCENE
- intent: complete the existing game protagonist, keeping its visual identity.
- deliverable: canonical editable Blender source, FBX/textures, isolated validated Unreal assets.
- units: metres; axes: right-handed Z-up; player faces -Y.
- render: existing EEVEE, existing cameras/exposure; QA at 900x1100; 30 fps, clip-specific frame ranges.
- dynamic: yes; existing 45 actions, with equipment following the rig.

HIERARCHY
- collection/object naming: existing ASTRAEON_* / SK_Astraeon_* convention.
- parent/child relationships: body/helmet skin to SKEL_Astraeon_Player; backpack at socket_backpack; wrist computer at lowerarm_l.
- protected existing objects: SRC_Player_MESH_01, PROXY_*, unrelated collections; preserve original source GLB.
- mutation scope: body topology/UV/weights, LOD1-3, helmet, authored facial keys, additive twist bones, two new equipment props; existing animation names preserved.

ASSETS
- A01 SK_Astraeon_Player | [EXISTING] | detailed | [1.8372,0.3815,1.83] m | origin [0,0,0] | rig parent | hero.
- A02 SK_Astraeon_Helmet | [EXISTING] | detailed | fitted to existing head; dimensions measured before refining | socket_helmet | removable protection.
- A03 SK_Astraeon_Backpack | [BLOCK] local lofted geometry, refined to detailed | target [0.30,0.16,0.42] m | back spine anchor, measured in rest pose | life support.
- A04 SK_Astraeon_WristComputer | [BLOCK] local lofted geometry, refined to detailed | target [0.11,0.075,0.045] m | fit to lowerarm_l in bone frame | instrument.
- generation estimate/submission state: none; no generation needed.

SHOT
- active camera: CAM_Player_Front; existing Side/Face/Hand/Helmet QA cameras.
- framing/lens/target: preserve measured existing orthographic full body and closeups; supplementary equipment views only if needed.
- depth: neutral studio background, uncluttered silhouette.

LOOK
- material roles and palette: preserve existing suit/skin texture, graphite equipment, bronze accents, dark cyan display.
- material route: EXISTING images plus locally authored nodes; separate Character/Suit/Gear/Helmet texture sets through baking.
- texture scale: UV maps, body 2048 px; equipment sized to pixel needs.
- relief: tangent normal from source; no invented displacement.
- response: skin/cloth dielectric, graphite rough, bronze metallic, restrained display emission.
- world/background: preserve existing neutral studio.

LIGHTING
- focal subject: body/face, secondary equipment, darkest recesses between armor.
- reference mood: neutral technical studio QA.
- environment route: EXISTING; no HDR requirement.
- baseline/key/fill: preserve existing World, LGT_Player_Key and LGT_Player_Fill and their exposure.
- motivation: studio inspection; no new cinematic lighting.
- reflections: broad existing areas; no fog/cards.
- color management: preserve existing view transform/exposure.

MOTION
- fps/frame range: 30; animation_contract.json.
- beats: existing authored actions; inspect walk/run/tool/crouch and grip poses.
- rest/loops: restore frame 1 and rest pose before export; verify loop seams.

ACCEPTANCE
- structural: 1.83 m, unit scale, root preserved; base 71 bones (version contract if additive twist bones); body target 60–80k tris, UV and normalized weights, 4 LODs, separate props/textures, meaningful facial morphs.
- topology: decimation alone does not count as articulation retopology; retain explicit limitation until verified edge flow exists.
- motion: 45 evaluated imported clips at 30 fps, no static multi-frame clips or discontinuous loops; deformations and equipment attachments checked visually.
- visual: inspect face, helmet opening, arm/hip bends, equipment contact, surface/bake continuity in Blender and Unreal.
- lighting: retain existing technical studio, check readable materials and no missing textures.
- validation: actual Unreal import and saved-package re-open plus rendered walk/run; no completion claim while required checks fail.

refs_read: blender-scene, blender-scene-spec, blender-modeling, blender-lookdev, blender-animation, blender-lighting-camera, blender-audit-finalize, blender-volatile.

Phase gates (updated 2026-09-08): A inspection/passport complete; B recovery checkpoints in
Saved/BlenderRecovery (4); C blockout accepted; **D complete** — quad retopology at 65 284 tris,
weights transferred with 0 unweighted vertices, separate Character/Suit/Gear/Helmet texture sets,
LOD1-3 regenerated from the new LOD0, equipment UV-unwrapped, backpack refitted to the measured
back, three facial morphs authored; **E complete** — structural, motion and visual audits passed,
plus a real Unreal 5.7.4 import that saved 68 packages with passed: true.

Acceptance deviations, decided and recorded:
- facial morphs: three (jaw_open, brow_raise, brow_furrow) instead of the seven the import script
  originally asserted. The face carries 249 vertices and ~18 per eye; a blink needs eyelid loops
  this topology does not have. The threshold in MainCharacterAppearance.py was lowered to the three
  authorized morphs rather than the assertion being removed.
- jaw_open lowers the mandible but does not part the lips: the mouth is sealed and has no interior
  geometry. Amplitude fixed at 6 degrees.
- hair silhouette keeps a few dark notches: retopology fidelity loss against the 233k source.
- gameplay integration is NOT done. See Docs/PENDIENTE_PROTAGONISTA.md, P9.

Deferred by decision: dense head retopology (would enable blink and a real mouth opening) and
helmet shape redesign.
