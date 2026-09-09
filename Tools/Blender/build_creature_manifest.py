"""Fail-closed source/package manifest for the original quadruped art."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def digest(path):
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


def main():
    source = json.loads((ROOT / 'ContentPipeline/reports/creature_blockout_validation.json').read_text())
    packages = json.loads((ROOT / 'ContentPipeline/reports/creature_content_packages.json').read_text())
    assert source['passed'] and packages['passed'], 'Incomplete evidence'
    for key, path in [('config_sha256', 'Tools/Blender/configs/creature_blockout.json'),
                      ('generator_sha256', 'Tools/Blender/generators/creature_blockout.py'),
                      ('validator_sha256', 'Tools/Blender/validators/creature.py')]:
        assert source[key] == digest(path), 'Stale source evidence: ' + path
    saved = {entry['id']: entry for entry in packages['meshes'] + packages['animations']}
    assert len(saved) == len(source['files']), 'Every export must reach a package'
    assets = []
    for item in source['files']:
        assert item['sha256'] == digest(item['path']) == saved[item['id']]['sha256'], 'Hash drift: ' + item['id']
        assets.append({'id': item['id'], 'path': item['path'], 'sha256': item['sha256'],
                       'unreal_asset': saved[item['id']]['asset'], 'quality': 'Q1',
                       'saved_unreal_package': True,
                       'gameplay_integrated': packages['gameplay_integrated']})
    manifest = {'schema_version': 1, 'generator_version': source['generator_version'],
                'species_id': source['species_id'],
                'provenance': 'Original procedural geometry and authored poses; no external assets',
                'skeleton': packages['skeleton'], 'assets': assets}
    (ROOT / 'ContentPipeline/creature_asset_manifest.json').write_text(
        json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    print('ASTRAEON_CREATURE_MANIFEST: PASS %d exports' % len(assets))


if __name__ == '__main__':
    main()
