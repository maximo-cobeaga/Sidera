import bpy

print("=== ASTRA BLENDER TEST ===")

# Limpiar escena
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)

# Crear un cubo
bpy.ops.mesh.primitive_cube_add(location=(0, 0, 0))

obj = bpy.context.object
obj.name = "Astra_Test"

# Guardar el archivo .blend
bpy.ops.wm.save_as_mainfile(
    filepath=r"C:\Users\MAXIMO\Desktop\Astraeon\graphics\astra_test.blend"
)

print("=== SUCCESS ===")