# Auditoría de arte — qué está sincronizado y qué llega al juego

Fecha: 2026-09-08. Pregunta que responde: de todo el arte generado, **cuánto está bien
enganchado y cuánto llega de verdad al juego**.

## Método

Dos medidas independientes, porque cada una miente por su lado:

1. **Cocinado contra disco.** `Saved/Cooked/Windows/Astraeon/Content` frente a `Content/`.
   El cocinado sigue referencias duras: lo que no se cocina **no lo usa el juego**. Es la
   medida dura de "usado"; buscar rutas `/Game/...` en el código no sirve, porque muchas se
   arman por concatenación.
2. **Slots de material.** `Scripts/Editor/AuditArtUsage.py` recorre cada malla estática y
   esquelética de `Content/Astraeon` y reporta los slots en nulo o con el material por
   defecto del motor. Es la medida de "sincronizado": un asset puede cocinarse y aun así
   verse mal.

Ambas se lanzan con `Scripts/RunCharacterChecks.ps1 -Check ArtUsage`, y la reparación con
`-Check ArtMaterials`. Reportes en `ContentPipeline/reports/art_usage_audit.json` y
`art_material_repair.json`.

---

## Defecto encontrado y corregido: materiales sin enganchar

**11 slots de material estaban en nulo**, en cinco mallas. Un slot en nulo no falla ni
avisa: el motor dibuja su material por defecto, así que el asset existe, se cocina, se ve
—y se ve mal—.

| Malla | Slots en nulo |
|---|---|
| `SK_Creature_UmbraGrazer_Blockout` | `M_Creature_Hide`, `M_Creature_Plate` |
| `SK_Creature_UmbraGrazer_Plated_Blockout` | `M_Creature_Hide_Plated`, `M_Creature_Plate_Plated` |
| `SK_Human_Body_Blockout` | `M_Human_fabric`, `M_Human_shell`, `M_Human_visor`, `M_Human_accent` |
| `SK_Human_HandsFP_Blockout` | `M_Human_fabric`, `M_Human_shell` |
| `SK_Astraeon_Player` (import crudo) | `MAT_Astraeon_Player_Atlas` |

El nombre del slot viaja en el FBX desde el material de Blender, y los scripts de
preparación crean un material con **ese mismo nombre en la misma carpeta**. Es decir: el
material correcto existía al lado del slot que lo necesitaba. `RepairArtMaterials.py`
reengancha por nombre, sólo donde el slot está vacío, y guarda: **10 reparados, 0 sin
resolver**.

El undécimo queda fuera a propósito: pertenece al import crudo del personaje, superado por
la versión optimizada, y apunta a un atlas que ya no existe.

Consecuencia visible: la criatura y las manos de primera persona se dibujaban con el
material por defecto en vez de con su paleta autorizada.

---

## Arte que existe y el juego no usa

De **209 assets** en `Content/`, el cocinado incluía **106**. Los 103 restantes, por familia:

| Familia | Qué es | Decisión |
|---|---|---|
| `Characters/Player/*` (47) | Import crudo del protagonista, previo a `Optimized/` | **Duplicado**: superado por `Optimized/`. Conservado como fuente de import, no entra al juego |
| `Characters/Player/Optimized/AN_*` (40 de 45) | Clips del cuerpo que ningún sistema reproduce | El runtime usa **5**: `Idle`, `Walk_F`, `Run_F`, `Jump_Loop`, `Jump_Land`. Crouch, Run/Walk laterales y traseros, `Scan`, `Interact`, `Pickup`, gestos de agarre y las poses One/TwoHand están **sin enganchar** |
| `AN_Human_{Idle,Walk,Run,Jump,Land}_Blockout` (5) | Clips originales de manos | Superados por `AN_HandsFP_*`; se conservan como fuente de la pose |
| `SM_Ref_Human_180_Blockout`, `M_Blockout_Human` | Prop de referencia de escala | Intencional: es una regla, no contenido |
| `Maps/TL_Art_MVP` | Mapa de pruebas de arte | Intencional: no se empaqueta |

El dato que importa para planificar: **el personaje tiene 45 clips y el juego reproduce 5**.
No es un defecto de sincronía —los clips están bien importados y a escala— sino trabajo de
integración pendiente, que se suma al pulido ya registrado en `KNOWN_ISSUES.md`.

---

## Estado tras la reparación

- Slots de material en nulo: **0** en todo lo que el juego usa.
- Materiales por defecto del motor: **0**.
- Referencias rotas de código a assets inexistentes: **0**.

Para repetir la comprobación:

```powershell
.\Scripts\RunCharacterChecks.ps1 -Check ArtUsage     # audita
.\Scripts\RunCharacterChecks.ps1 -Check ArtMaterials # repara por nombre de slot
```
