"""Create a dedicated test map once; subsequent runs validate without replacing it."""
import json
from pathlib import Path
import unreal

MAP = '/Game/Maps/TL_Art_MVP'
ROOT = Path(unreal.Paths.project_dir()).resolve()
level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if unreal.EditorAssetLibrary.does_asset_exist(MAP):
    level.load_level(MAP)
else:
    if not level.new_level(MAP):
        raise RuntimeError('Could not create isolated art test map')

# Complete this generated fixture after an interrupted creation without deleting user actors.
existing = {a.get_actor_label():a for a in actors.get_all_level_actors()}
if 'Itaca_Q1_ModularInterior' not in existing:
    room_class = unreal.load_class(None, '/Script/Astraeon.AstraeonItacaInterior')
    room = actors.spawn_actor_from_class(room_class, unreal.Vector(0,0,0))
    room.set_actor_label('Itaca_Q1_ModularInterior')
if 'PlayerStart_ItacaArt' not in existing:
    player = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0,0,100))
    player.set_actor_label('PlayerStart_ItacaArt')
if 'Reference_Human_180cm_NoCollision' not in existing:
    human = unreal.EditorAssetLibrary.load_asset('/Game/Astraeon/Art/Blockouts/Itaca/SM_Ref_Human_180_Blockout')
    if human is None:
        raise RuntimeError('Import the reference mesh first')
    reference = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(-100,210,0))
    reference.get_component_by_class(unreal.StaticMeshComponent).set_static_mesh(human)
    reference.set_actor_label('Reference_Human_180cm_NoCollision')
    reference.get_component_by_class(unreal.StaticMeshComponent).set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
if not level.save_current_level():
    raise RuntimeError('Could not save art test map')
labels = [a.get_actor_label() for a in actors.get_all_level_actors()]
if 'Itaca_Q1_ModularInterior' not in labels or 'PlayerStart_ItacaArt' not in labels:
    raise RuntimeError('Existing test map does not match expected art fixture')
(ROOT/'ContentPipeline/reports/itaca_test_map.json').write_text(json.dumps({'passed':True,'map':MAP,'actors':labels}, indent=2)+'\n', encoding='utf-8')
unreal.log('ITACA_ART_MAP: PASS')
