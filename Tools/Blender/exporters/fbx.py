from pathlib import Path
import sys

import bpy

# El preset vive en un solo sitio. Antes estos ajustes estaban escritos aquí y repetidos en
# `main_character_export.py`: coincidían, pero nada lo garantizaba. Ver `presets/UE57_AST_V1.py`.
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from presets.UE57_AST_V1 import static_mesh_kwargs


def export_asset(obj, directory):
    directory = Path(directory)
    directory.mkdir(parents=True, exist_ok=True)
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    path = directory / (obj.name + '.fbx')
    result = bpy.ops.export_scene.fbx(filepath=str(path), **static_mesh_kwargs())
    if 'FINISHED' not in result or not path.is_file() or path.stat().st_size == 0:
        raise RuntimeError('FBX export failed: ' + str(path))
    return path
