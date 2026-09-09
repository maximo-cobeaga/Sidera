"""Build catalog only when source, FBX and transient UE evidence agree."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def digest(path):
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


def main():
    source = json.loads((ROOT / 'ContentPipeline/reports/humanoid_blockout_validation.json').read_text())
    unreal = json.loads((ROOT / 'ContentPipeline/reports/humanoid_unreal_import.json').read_text())
    assert source['passed'] and unreal['passed']
    for key, path in [('config_sha256', 'Tools/Blender/configs/humanoid_blockout.json'),
                      ('generator_sha256', 'Tools/Blender/generators/humanoid_blockout.py'),
                      ('validator_sha256', 'Tools/Blender/validators/humanoid.py')]:
        assert source[key] == digest(path), 'Stale evidence: ' + path
    imports = {item['id']: item for item in unreal['assets']}
    assert len(source['files']) == len(imports) == 9
    assets = []
    for item in source['files']:
        assert digest(item['path']) == item['sha256'] == imports[item['id']]['sha256']
        assert imports[item['id']]['passed']
        assets.append({**item, 'quality': 'Q1', 'source_validated': True,
                       'unreal_transient_import_validated': True, 'saved_unreal_package': False,
                       'gameplay_integrated': False, 'skeleton': 'SKEL_Humanoid_A'})
    manifest = {'schema_version': 1, 'generator_version': source['generator_version'],
                'provenance': 'Original geometry and animation authored locally; no external assets',
                'source_blend': 'ContentPipeline/Generated/HumanoidBlockout/Humanoid_Blockout_Source.blend',
                'assets': assets}
    (ROOT / 'ContentPipeline/humanoid_asset_manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    print('ASTRAEON_HUMANOID_MANIFEST: PASS 9 assets')


if __name__ == '__main__':
    main()
