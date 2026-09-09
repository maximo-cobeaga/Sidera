You are the autonomous 3D art pipeline lead for:

ASTRAEON: Legado del Vacío.

You are working directly inside the ASTRAEON repository on Windows 11.

The local environment already has:

- Blender 5.2.1 LTS
- Blender executable:
  `C:\Program Files\Blender Foundation\Blender 5.2\blender.exe`
- Codex with access to the current repository
- Unreal Engine 5.7.4
- Git
- A working Blender Python (`bpy`) automation pipeline
- Blender command-line execution already validated successfully

Do not spend time revalidating basic Blender installation unless the existing pipeline fails.

# MISSION

Plan and begin implementing the complete visual asset pipeline for ASTRAEON.

The project must eventually support, at minimum:

- main playable character
- main player spacecraft
- procedural planets
- biome materials
- biome props
- destructible props
- creatures / mobs
- citizens / NPCs where required
- structures
- ruins
- alien technology
- interactable objects
- environmental storytelling assets
- resource objects
- mission-specific objects
- VFX-supporting geometry
- modular asset families

However:

THE MVP `La primera señal` remains the production priority.

Do not attempt to build the entire game's final art library before determining what the MVP actually requires.

The objective of this task is therefore:

1. understand the game and current development state;
2. define the complete visual asset taxonomy;
3. identify the minimum assets necessary for the MVP;
4. design reusable procedural/modular systems wherever possible;
5. establish the Blender → validation → export → Unreal pipeline;
6. begin production with the highest-value foundational assets;
7. leave the repository in a state where subsequent agents can continue asset production autonomously.

---

# PHASE 0 — READ THE PROJECT

Before making changes, completely read all relevant documentation.

At minimum:

- `AGENTS.md`
- `Docs/GAME_DESIGN_MASTER.md`
- `Docs/MVP_LA_PRIMERA_SENAL.md`
- `Docs/MODULAR_ASSET_GENERATION.md`
- `Docs/GUIA_ARTE_PLANETAS_BLENDER.md`
- `DEVELOPMENT_STATE.md`
- `BACKLOG.md`

Also inspect:

- existing Unreal Content structure
- existing Blender tooling
- existing scripts
- existing assets
- existing naming conventions
- existing character/gameplay systems
- existing spaceship systems
- existing planet systems
- existing creature systems
- current test levels

Do not assume that documentation and implementation are synchronized.

Determine the real repository state.

---

# PHASE 1 — ASSET AUDIT

Create an inventory of every asset family implied by the current game design.

Do not immediately model them.

Classify them.

Use categories similar to:

## Characters

- PlayerCharacter
- NPC_Citizen
- NPC_Scientist
- NPC_Engineer
- NPC_Trader
- hostile humanoids if required
- story characters

## Creatures

- Creature_Quadruped_A
- future quadruped families
- bipeds
- flyers
- crawlers
- aquatic / exotic life if supported by design

## Vehicles

- PlayerShip
- NPC ships
- drones
- escape craft
- wrecked ships

## Planets

- reference planet meshes
- biome materials
- biome props
- destructible props
- resources
- landmarks

The current planet system must preserve the design already established:

- procedural cube-sphere gameplay planet
- Blender reference meshes only
- Small radius: 150 m
- Medium radius: 400 m
- Large radius: 900 m
- Giant radius: 2000 m
- 33×33 vertices per cube face for Blender reference meshes
- triplanar materials
- biome blending driven by runtime vertex data
- no hand-painted splat maps per planet

Supported initial biomes:

- Desert
- Rocky
- Forest
- Ice
- Volcanic
- Saline

## Environment

- modular structures
- settlements
- stations
- interiors
- ruins
- alien ruins
- caves if necessary
- doors
- corridors
- platforms
- containers
- machinery
- consoles
- lights
- resource extractors

## Story / Mission

Determine explicitly which unique assets are needed by:

`MVP_LA_PRIMERA_SENAL`

These assets receive higher priority than generic Phase 2 content.

## Resources

Identify:

- harvestable resources
- minerals
- crystals
- biological resources
- technological salvage
- containers

## Equipment

Determine from game design:

- weapons
- scanner
- tools
- suit components
- backpack
- handheld devices
- mission equipment

## VFX support meshes

Identify geometry required for:

- engines
- shields
- holograms
- portals
- scanning
- damage
- destruction
- environmental effects

Do not fabricate gameplay systems that do not exist in the design documentation.

---

# PHASE 2 — DEFINE THE VISUAL SYSTEM

Before detailed asset production, create a coherent visual language.

Determine from existing documentation:

- visual tone
- technological era
- realism level
- human technology style
- alien technology style
- architecture language
- silhouette language
- material language
- color principles
- wear/damage philosophy
- scale language
- creature biological principles

If documentation does not sufficiently define one of these areas:

choose a conservative, reversible direction that fits the existing game.

Document the decision.

Do not block production for minor ambiguity.

---

# PHASE 3 — MAIN CHARACTER

Design the technical asset specification for the playable character.

Do NOT begin with final sculpting.

First define:

- canonical height
- proportions
- skeleton strategy
- Unreal compatibility
- rig structure
- locomotion requirements
- hand requirements
- equipment attachment points
- backpack attachment
- helmet attachment
- weapon/tool sockets
- modular clothing strategy
- facial requirements
- whether facial animation is required for MVP
- LOD requirements
- collision requirements
- material slots
- texture budgets
- topology budgets

Prefer modularity.

Example structure, if compatible with game design:

`Character_Player_A`

with modular:

- BaseBody
- Head
- Hair
- SuitTorso
- SuitLegs
- Boots
- Gloves
- Helmet
- Backpack
- Equipment

The player character must NOT become an isolated one-off asset if components can support future NPC generation.

Investigate whether the player skeleton can also become the canonical humanoid skeleton for future NPCs.

Document the tradeoff before committing.

---

# PHASE 4 — PLAYER SHIP

Create the technical design for the main spacecraft.

First determine gameplay requirements from the design documentation.

Identify:

- exterior dimensions
- player capacity
- walkable interior or non-walkable interior
- cockpit requirements
- entrances
- landing system
- cargo
- propulsion
- damage states
- customization
- module system
- interactable consoles
- weapons
- scanning
- storage
- engines
- landing gear
- doors
- animation requirements

Do NOT detail the final ship before determining what must actually function in the MVP.

Prefer a modular ship architecture where appropriate.

Possible logical decomposition:

- Ship_Core
- Cockpit
- Hull
- Engine_Left
- Engine_Right
- LandingGear
- Cargo
- Antenna
- Scanner
- WeaponHardpoint
- Interior modules

Only use this decomposition if compatible with the actual game design.

Create blockout geometry first.

Validate human scale using the canonical player height.

---

# PHASE 5 — CREATURE SYSTEM

Continue the existing procedural/modular philosophy.

The first canonical creature family is:

`Creature_Quadruped_A`

This must NOT be implemented as ten unrelated finished creatures.

Design a coherent anatomical family.

Candidate interchangeable systems:

- torso
- head
- jaw
- neck
- front limbs
- rear limbs
- feet
- tail
- horns
- armor plates
- dorsal structures
- sensory organs

Candidate procedural parameters:

- body length
- body mass
- leg length
- neck length
- head size
- jaw scale
- tail length
- shoulder height
- muscle mass
- armor density

Generation must be deterministic from a seed.

Example conceptual result:

Seed → module selection + morph values + material values → reproducible creature.

Do not sacrifice animation compatibility for visual variety.

Prioritize:

1. shared skeleton
2. stable attachment interfaces
3. deformation quality
4. modularity
5. procedural variety
6. detail

---

# PHASE 6 — PLANET PIPELINE

Treat `GUIA_ARTE_PLANETAS_BLENDER.md` as authoritative for the Blender-side planet art pipeline unless it conflicts with a newer explicit project decision.

Do not model final runtime planets in Blender.

Blender produces:

- reference cube-sphere meshes
- biome material source assets
- reusable detail masks
- biome prop catalogs
- destructible prop source meshes

Runtime terrain remains generated in Unreal/C++.

Create reference meshes:

- `SM_Planet_Ref_Small`
- `SM_Planet_Ref_Medium`
- `SM_Planet_Ref_Large`
- `SM_Planet_Ref_Giant`

Requirements:

- cube-sphere topology
- 6 faces
- uniform quads
- 32 segments / 33×33 vertices per face
- normalized sphere
- outward normals
- metric real-world scale
- centered origin

Do not add unnecessary mesh detail.

---

# PHASE 7 — BIOME SYSTEM

Initial biome codes:

- Desert
- Rocky
- Forest
- Ice
- Volcanic
- Saline

Every biome must eventually support at least:

## Ground

Two tileable terrain layers.

Textures:

- BaseColor 2048×2048
- Normal 2048×2048
- ORM 2048×2048
- optional Height 1024×1024
- DetailMask 1024×1024

Target world tile:

4×4 meters.

DetailMask channels:

- R = macro variation
- G = fine variation
- B = biome-edge noise

Generated masks should be procedural and tileable.

## Props

Minimum target catalog per biome:

- 3 small rock variants
- 2 medium rock variants
- 1–2 large rock/formations
- 2–3 primary vegetation/ground detail variants
- 1–2 secondary detail variants
- 1 unique biome-defining prop

Do not produce all six complete biome catalogs immediately unless MVP/test-level priorities justify it.

Implement one vertical slice first.

---

# PHASE 8 — BLENDER AUTOMATION ARCHITECTURE

Production assets should be reproducible whenever practical.

Use Blender Python (`bpy`) as the preferred generation/manipulation interface.

Do not depend on unreproducible manual Blender state.

Create or improve a structure similar to:

`Tools/Blender/`

containing:

- `common/`
- `generators/`
- `validators/`
- `exporters/`
- `tests/`
- `configs/`

Create common reusable utilities for:

- scene initialization
- unit setup
- naming
- transforms
- material creation
- mesh validation
- manifold checks
- UV validation
- pivot validation
- bounding-box measurement
- triangle counting
- armature validation
- FBX export
- JSON reports

Avoid duplicating logic between generators.

---

# PHASE 9 — VALIDATION

Automated validation is mandatory.

Every generated asset family should be validated where applicable.

Validate:

- expected object names
- duplicate names
- scale
- rotation
- applied transforms
- pivot position
- dimensions
- mesh type
- topology
- ngons
- manifold state
- normal direction
- triangle count
- material slots
- armature relationship
- missing textures
- unsupported modifiers
- export readiness

For destructible meshes additionally verify:

- watertight
- manifold
- no duplicate interior geometry
- outward normals

Produce machine-readable validation output.

Prefer JSON reports.

Example:

`reports/Creature_Quadruped_A_validation.json`

Validation failure should result in a non-zero exit code when run in CI-style mode.

---

# PHASE 10 — BLENDER → UNREAL RULES

Respect the existing project standard:

Blender:

- metric
- Unit Scale = 1.0
- 1 Blender unit = 1 meter

FBX:

- Forward = -Y
- Up = Z
- export scale = 1.0

Unreal:

- Import Uniform Scale = 1.0

Final objects must have rotation and scale applied.

Props:

- pivot at ground contact
- XY centered
- Z=0 at base

Planet references:

- pivot at exact sphere center

Final FBX policy:

ONE FINAL ASSET PER FBX.

Do not export entire working Blender scenes into a single production FBX.

---

# PHASE 11 — NAMING

Preserve existing naming conventions.

Examples:

Static meshes:

- `SM_`

Skeletal meshes:

- `SK_`

Textures:

- `T_`

Materials:

- `M_`

Material instances:

- `MI_`

Planet examples:

- `SM_Planet_Ref_Medium`
- `SM_Rock_Desert_Medium_02`
- `SM_Rock_Desert_Medium_02_Destructible`
- `SM_Veg_Forest_Tree_01`
- `T_Desert_Ground_A`
- `T_Desert_Ground_N`
- `T_Desert_Ground_ORM`
- `T_Desert_DetailMask`
- `M_Desert_Ground`

Create additional conventions for characters, ships, creatures and structures only after checking whether they already exist.

Document new conventions centrally.

---

# PHASE 12 — PRIORITIZATION

After completing the audit, create a dependency-aware asset roadmap.

Classify every asset as:

- MVP_BLOCKER
- MVP_REQUIRED
- MVP_SUPPORTING
- POST_MVP_FOUNDATION
- PHASE_2
- LATER

Also estimate for each asset family:

- gameplay dependency
- reuse value
- procedural potential
- technical risk
- artistic risk
- required before Unreal testing?
- placeholder acceptable?
- final art required?

Favor assets that unlock multiple systems.

For example, a canonical humanoid skeleton may unlock:

- player
- NPCs
- suits
- animations
- equipment attachments

A canonical planet reference mesh may unlock:

- curvature testing
- materials
- scatter testing
- size validation

A modular quadruped skeleton may unlock:

- multiple creature variants

Use this leverage when ordering work.

---

# PHASE 13 — TEST LEVEL STRATEGY

Preserve the existing planet test progression:

1. `TL_00_ReferenceScale`
2. `TL_01_MaterialBiomeSingle`
3. `TL_02_BiomeBlend`
4. `TL_03_PropScatterSingleBiome`
5. `TL_04_AllBiomesOnePlanet`
6. `TL_05_PropDestruction`
7. `TL_06_TerrainDestructionRegen`
8. `TL_07_SizeTierSweep`
9. `TL_08_PerfStress`

Do not skip directly to complex levels.

Placeholders are explicitly acceptable in early tests.

Also determine whether equivalent test maps are required for:

- PlayerCharacter
- PlayerShip
- Creature_Quadruped_A
- structures/interiors

If useful, propose them.

Do not create unnecessary maps.

---

# PHASE 14 — INITIAL VERTICAL SLICE

After planning, immediately begin one small vertical slice instead of stopping at documentation.

Choose the vertical slice based on actual MVP dependencies.

Strong candidates are:

A. canonical player scale + character blockout

B. player ship exterior/interior blockout

C. `Creature_Quadruped_A` anatomical blockout

D. `SM_Planet_Ref_Medium` + first biome material test

Select the one with the highest dependency value after reading the project.

You may implement multiple tiny foundational blockouts if they are tightly coupled, but do not begin detailed art production across every asset family simultaneously.

---

# PHASE 15 — DELIVERABLES FOR THIS RUN

Before finishing this task, produce:

## 1. Art asset master plan

Create:

`Docs/ART_ASSET_MASTER_PLAN.md`

Include:

- complete asset taxonomy
- MVP assets
- post-MVP assets
- dependencies
- priorities
- procedural/modular strategy
- naming
- quality tiers
- technical budgets
- production order

## 2. Visual direction document

If no equivalent document already exists, create:

`Docs/VISUAL_LANGUAGE.md`

Do not invent extensive lore.

Translate existing game design into actionable art rules.

## 3. Blender pipeline documentation

Create or update:

`Docs/BLENDER_PIPELINE.md`

Include:

- Blender version
- executable path
- repository structure
- generator usage
- validator usage
- exporter usage
- Blender→Unreal rules
- troubleshooting

## 4. Asset manifest

Create a machine-readable manifest such as:

`ContentPipeline/asset_manifest.json`

or another repository-appropriate location.

Each asset entry should be able to track:

- id
- category
- family
- source
- generator
- output
- status
- priority
- MVP relevance
- dependencies
- validation state
- Unreal destination

## 5. Initial Blender tooling

Create/refactor the minimal reusable pipeline necessary to support the first vertical slice.

## 6. First generated blockout(s)

Generate the highest-priority foundational asset(s).

Do not fake completion.

A blockout must be clearly labeled as blockout.

## 7. Validation report

Automatically validate generated output.

## 8. Development state

Update:

- `DEVELOPMENT_STATE.md`
- `BACKLOG.md`

Record:

- completed work
- decisions
- technical debt
- blocked work
- next recommended asset task

---

# AUTONOMY RULES

Work autonomously.

Do not ask questions for minor decisions.

When facing a non-blocking ambiguity:

1. select the simplest solution;
2. prefer reversible decisions;
3. prefer reusable systems;
4. document the assumption;
5. continue.

Ask for user intervention only when a decision would:

- fundamentally alter game design
- create substantial irreversible work
- require paid external services
- require credentials
- require licensing decisions
- conflict materially with existing documentation

Do not stop merely because an asset lacks final concept art.

Use blockout/procedural placeholders when appropriate.

---

# IMPORTANT ART RULE

Do not optimize for generating the maximum number of meshes.

Optimize for creating reusable asset systems.

BAD:

10 incompatible alien creatures.

GOOD:

1 strong quadruped anatomical family capable of generating many compatible variants.

BAD:

30 unrelated building meshes.

GOOD:

1 modular architectural kit capable of assembling many structures.

BAD:

1 monolithic player ship mesh if gameplay requires modular functionality.

GOOD:

a coherent ship system whose modules correspond to actual gameplay needs.

---

# SOURCE OF TRUTH

Prefer this hierarchy when resolving conflicts:

1. explicit current user instruction
2. `GAME_DESIGN_MASTER.md`
3. `MVP_LA_PRIMERA_SENAL.md`
4. more specific current subsystem documentation
5. `DEVELOPMENT_STATE.md`
6. implementation reality
7. backlog / older notes

If documentation conflicts with implementation, document the discrepancy rather than silently hiding it.

---

# EXECUTION LOOP

For every implementation step:

1. inspect
2. plan
3. implement
4. run Blender if required
5. validate
6. inspect output
7. correct failures
8. rerun
9. document
10. commit-ready state

Do not report success unless validation passes.

---

# FIRST ACTION

Begin now.

First:

1. read all project documents;
2. inspect the repository;
3. inventory all visual asset requirements;
4. identify MVP blockers;
5. determine dependency order;
6. create the master art plan;
7. select the first vertical slice;
8. implement its pipeline and blockout;
9. validate it;
10. update project state.

Do not begin by creating detailed final assets.

Build the production system first.