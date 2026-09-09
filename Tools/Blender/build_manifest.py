"""Rebuild the art catalog using current evidence; standard Python, no bpy required."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CATALOG = [
    ('Character_Player_A','Characters','MVP_SUPPORTING',[], 'First-person scale, modular suit/body; rig not produced'),
    ('Equipment_Scanner_Tools','Equipment','MVP_SUPPORTING',['Character_Player_A'], 'Scanner, harvesting tool, FP hands and attachment sockets'),
    ('Equipment_Suit_Resonator','Equipment','MVP_REQUIRED',[], 'Protection and signal_resonator presentation'),
    ('Ship_Itaca_Interior','Vehicles','MVP_REQUIRED',['SM_Ref_Human_180_Blockout'], 'Room, ceiling, panels, hatch leaf; kit sample exported, room not assembled'),
    ('Itaca_Story_Props','Story','MVP_REQUIRED',['Ship_Itaca_Interior'], 'Titles, notes, original photographs and repairs'),
    ('Ship_Itaca_Exterior','Vehicles','POST_MVP_FOUNDATION',['Ship_Itaca_Interior'], 'Hull, cockpit, engines, landing gear, antenna and cargo'),
    ('Region_Ground','Environment','MVP_REQUIRED',[], 'Bounded runtime region; readable ground material'),
    ('Region_Rocky_Props','Environment','MVP_SUPPORTING',['Region_Ground'], 'Regional rocks and navigation landmarks'),
    ('Resource_silicate_fiber','Resources','MVP_REQUIRED',[], 'Harvestable silicate_fiber node'),
    ('Resource_ferrite_nodule','Resources','MVP_REQUIRED',[], 'Harvestable ferrite_nodule node'),
    ('Resource_SeedSignature','Resources','MVP_REQUIRED',[], 'Seed-selected resource; preserve the current C++ catalog IDs'),
    ('Signal_Source_Barrier','Story','MVP_REQUIRED',['Equipment_Suit_Resonator'], 'Signal source, progression barrier, small anomaly'),
    ('Creature_Quadruped_A','Creatures','MVP_REQUIRED',[], 'Umbra Grazer shared anatomy/rig and one compatible visual variant'),
    ('UI_Visor_Logbook_Map','UI','MVP_REQUIRED',[], 'Instrument readouts, certainty, logbook and revealed map'),
    ('VFX_Scan_Alert','VFX','MVP_SUPPORTING',['UI_Visor_Logbook_Map'], 'Scan/hologram geometry and threat/damage feedback'),
    ('Planet_References','Planets','POST_MVP_FOUNDATION',['SM_Ref_Human_180_Blockout'], 'Small 150m, Medium 400m, Large 900m, Giant 2000m; 33x33 samples per face'),
    *[(f'Biome_{biome}','Planets','PHASE_2',['Planet_References'],
       'Two 2K ground layers, 1K RGB DetailMask, rock/detail/unique prop catalog')
      for biome in ('Desert','Rocky','Forest','Ice','Volcanic','Saline')],
    ('Props_Destructible','Environment','PHASE_2',['Biome_Rocky'], 'Closed source meshes; Chaos/runtime implementation still required'),
    ('Creatures_OtherFamilies','Creatures','PHASE_2',['Creature_Quadruped_A'], 'Bipeds, flyers, crawlers, aquatic/exotic families as ecology requires'),
    ('NPC_Humanoids','Characters','LATER',['Character_Player_A'], 'Citizen, scientist, engineer, trader, story and hostile roles only when justified'),
    ('Vehicles_Other','Vehicles','LATER',['Ship_Itaca_Exterior'], 'NPC ships, drones/probes, escape craft and wrecks'),
    ('Structures_Modular','Environment','LATER',['Ship_Itaca_Interior'], 'Corridors, platforms, stations, settlements, doors and lights'),
    ('Ruins_AlienTechnology','Environment','LATER',['Signal_Source_Barrier'], 'Expanded ruins, caves and alien machinery; no civilization produced now'),
    ('Machinery_Storage_Extraction','Environment','LATER',['Structures_Modular'], 'Containers, extractors, lab, greenhouse and colony machinery'),
    ('Equipment_Weapons','Equipment','LATER',['Character_Player_A'], 'Future weapon/tool families and sockets; not mandatory combat in MVP'),
    ('VFX_FutureSystems','VFX','LATER',['Vehicles_Other'], 'Engine/shield/destruction support when gameplay exists; portals unrequested')
]


def main():
    config_path = ROOT / 'Tools/Blender/configs/itaca_blockout.json'
    config = json.loads(config_path.read_text(encoding='utf-8'))
    validation_path = ROOT / 'ContentPipeline/reports/itaca_blockout_validation.json'
    report = json.loads(validation_path.read_text(encoding='utf-8'))
    if not report['passed']:
        raise RuntimeError('Cannot promote assets from a failed generation run')
    if report['config_sha256'] != hashlib.sha256(config_path.read_bytes()).hexdigest():
        raise RuntimeError('Stale source validation; rerun Blender')
    evidence = {r['id']: r for r in report['assets']}
    import_path = ROOT / 'ContentPipeline/reports/itaca_unreal_import.json'
    imported = json.loads(import_path.read_text(encoding='utf-8')) if import_path.exists() else {}
    import_evidence = {a['id']: a for a in imported.get('assets', [])} if imported.get('passed') else {}
    assets = []
    for spec in config['assets']:
        item = evidence[spec['id']]
        output = ROOT / item['fbx']
        if not item['passed'] or not item['fbx_roundtrip_passed'] or not output.is_file():
            raise RuntimeError('Incomplete output: ' + spec['id'])
        fbx_hash = hashlib.sha256(output.read_bytes()).hexdigest()
        if fbx_hash != item['fbx_sha256']:
            raise RuntimeError('FBX changed after validation: ' + spec['id'])
        ue = import_evidence.get(spec['id'], {})
        import_passed = bool(ue.get('passed') and ue.get('fbx_sha256') == fbx_hash)
        is_human = spec['id'].startswith('SM_Ref')
        assets.append({'id':spec['id'], 'entry_type':'asset',
            'category':'Reference' if is_human else 'Environment',
            'family':'Character_Player_A' if is_human else 'Ship_Itaca_Interior',
            'source':'Tools/Blender/configs/itaca_blockout.json',
            'generator':'Tools/Blender/generators/itaca_blockout.py',
            'generator_version':1, 'output':item['fbx'], 'status':'blockout_export_validated',
            'priority':'MVP_SUPPORTING' if is_human else 'MVP_REQUIRED',
            'mvp_relevance':'Scale reference' if is_human else 'AC-03 Itaca presentation',
            'dependencies':[] if is_human else ['SM_Ref_Human_180_Blockout'],
            'validation_state':{'source':'passed','fbx_roundtrip':'passed',
                'unreal_dimensions':'passed_transient_import' if import_passed else 'pending',
                'report':validation_path.relative_to(ROOT).as_posix()},
            'unreal_destination':'/Game/Astraeon/Art/Blockouts/Itaca/' + spec['id'],
            'unreal_integrated':False, 'license_provenance':'Original repository-authored procedural blockout; no third-party assets',
            'geometry_sha256':item['geometry_sha256'], 'fbx_sha256':fbx_hash})
    for identifier, category, priority, deps, description in CATALOG:
        assets.append({'id':identifier, 'entry_type':'family', 'category':category,
            'family':identifier, 'source':'Docs/ART_ASSET_MASTER_PLAN.md',
            'generator':None, 'output':None, 'status':'planned', 'priority':priority,
            'mvp_relevance':description, 'dependencies':deps,
            'validation_state':{'source':'not_produced'},
            'unreal_destination':f'/Game/Astraeon/Art/{category}/{identifier}',
            'unreal_integrated':False})
    ids = {a['id'] for a in assets}
    if len(ids) != len(assets) or any(d not in ids for a in assets for d in a['dependencies']):
        raise RuntimeError('Duplicate IDs or unresolved dependencies')
    graph = {a['id']: a['dependencies'] for a in assets}
    def visit(node, active):
        if node in active:
            raise RuntimeError('Cyclic dependency: ' + node)
        for dependency in graph[node]:
            visit(dependency, active | {node})
    for node in graph:
        visit(node, set())
    manifest = {'schema_version':1, 'scope':'MVP-first art catalog; family rows are plans, not produced assets',
                'supplemental_manifests':['ContentPipeline/region_asset_manifest.json'],
                'assets':assets}
    (ROOT / 'ContentPipeline/asset_manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    print(f'ART_MANIFEST: PASS {len(assets)} entries; {len(config["assets"])} generated blockouts')


if __name__ == '__main__':
    main()
