"""Fail-closed source/import manifest for original handheld art."""
import hashlib
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]


def digest(path):return hashlib.sha256((ROOT/path).read_bytes()).hexdigest()


def main():
    source=json.loads((ROOT/'ContentPipeline/reports/tools_blockout_validation.json').read_text())
    ue=json.loads((ROOT/'ContentPipeline/reports/tools_unreal_import.json').read_text())
    assert source['passed'] and ue['passed']
    for key,path in [('config_sha256','Tools/Blender/configs/tools_blockout.json'),
                     ('generator_sha256','Tools/Blender/generators/tools_blockout.py'),
                     ('validator_sha256','Tools/Blender/validators/tools.py')]:
        assert source[key]==digest(path),'Stale source evidence'
    assert source['humanoid_source_sha256']==digest('ContentPipeline/Generated/HumanoidBlockout/Humanoid_Blockout_Source.blend')
    imports={a['id']:a for a in ue['assets']+ue['animations']}
    assert len(imports)==15
    assets=[]
    for item in source['assets']+source['animation_files']:
        assert item['sha256']==digest(item['path'])==imports[item['id']]['sha256']
        assert imports[item['id']]['passed']
        assets.append({'id':item['id'],'path':item['path'],'sha256':item['sha256'],
            'item_id':item.get('item_id'),'quality':'Q1','unreal_transient_import':True,
            'saved_unreal_package':False,'gameplay_integrated':False})
    manifest={'schema_version':1,'generator_version':source['generator_version'],
        'provenance':'Original procedural geometry and authored human poses; no external assets',
        'attachment':source['attachment'],'assets':assets}
    (ROOT/'ContentPipeline/tools_asset_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    print('ASTRAEON_TOOLS_MANIFEST: PASS 15 exports')


if __name__=='__main__':main()
