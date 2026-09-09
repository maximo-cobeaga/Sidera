"""Validated supplemental catalog; preserves the existing Itaca/future family catalog."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def main():
    config_path = ROOT / 'Tools/Blender/configs/region_blockout.json'
    config = json.loads(config_path.read_text(encoding='utf-8'))
    source = json.loads((ROOT / 'ContentPipeline/reports/region_blockout_validation.json').read_text(encoding='utf-8'))
    imported = json.loads((ROOT / 'ContentPipeline/reports/region_unreal_import.json').read_text(encoding='utf-8'))
    if not source['passed'] or not imported['passed']:
        raise RuntimeError('Source and Unreal import must pass')
    if source['config_sha256'] != hashlib.sha256(config_path.read_bytes()).hexdigest():
        raise RuntimeError('Stale source evidence')
    if source['generator_sha256'] != hashlib.sha256((ROOT / 'Tools/Blender/generators/region_blockout.py').read_bytes()).hexdigest():
        raise RuntimeError('Stale generator evidence')
    evidence = {a['id']: a for a in source['assets']}
    ue = {a['id']: a for a in imported['assets']}
    assets = []
    for spec in config['assets']:
        item = evidence[spec['id']]
        digest = hashlib.sha256((ROOT / item['fbx']).read_bytes()).hexdigest()
        if not item['passed'] or not item['fbx_roundtrip_passed'] or not ue[spec['id']]['passed'] or digest != item['fbx_sha256'] or digest != ue[spec['id']]['fbx_sha256']:
            raise RuntimeError('Stale/incomplete asset evidence: ' + spec['id'])
        assets.append({'id': spec['id'], 'family': spec['family'], 'quality': 'Q1_Blockout',
                       'status': 'imported_runtime_binding_present', 'dimensions_m': spec['dimensions_m'],
                       'source': config_path.relative_to(ROOT).as_posix(),
                       'generator': 'Tools/Blender/generators/region_blockout.py', 'generator_version': 1,
                       'fbx': item['fbx'], 'fbx_sha256': digest, 'geometry_sha256': item['geometry_sha256'],
                       'unreal_asset': ue[spec['id']]['asset'], 'triangles': item['triangles'],
                       'material_slots': 1, 'collision': 'existing_C++_marker_proxy; art_NoCollision',
                       'license_provenance': 'Original repository-authored procedural geometry; no external assets',
                       'acceptance': 'See region_Automation/Visual/PackagedVisual reports; Q2 requires human visual review'})
    manifest = {'schema_version': 1, 'scope': 'Seven region blockouts, not complete character/animation delivery',
                'reports': ['ContentPipeline/reports/region_blockout_validation.json',
                            'ContentPipeline/reports/region_unreal_import.json'], 'assets': assets}
    (ROOT / 'ContentPipeline/region_asset_manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    print('REGION_MANIFEST: PASS seven verified assets')


if __name__ == '__main__':
    main()
