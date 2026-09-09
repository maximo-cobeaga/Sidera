"""Construct the protagonist's portable PBR materials and import separate gear."""
import unreal


def texture(path, destination, normal=False, data=False):
    task=unreal.AssetImportTask()
    for k,v in dict(filename=str(path),destination_path=destination,
                    automated=True,replace_existing=False,save=False).items():
        task.set_editor_property(k,v)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    images=[o for o in task.get_objects() if isinstance(o,unreal.Texture2D)]
    assert len(images)==1, 'Texture import failed: '+str(path)
    image=images[0]
    if normal:
        image.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP)
        image.set_editor_property('flip_green_channel',True)
    if normal or data:image.set_editor_property('srgb',False)
    return image


def pbr(region,destination,source):
    material=unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        'M_Player_'+region,destination,unreal.Material,unreal.MaterialFactoryNew())
    textures=[]
    samples={}
    for index,kind in enumerate(('Color','NormalGL','ORM','Emission')):
        path=source/'textures'/f'T_Player_{region}_{kind}.png'
        if not path.exists():
            assert kind=='Emission', 'Missing required texture: '+str(path)
            continue
        image=texture(path,destination,normal=kind=='NormalGL',data=kind=='ORM')
        textures.append(image)
        sample=unreal.MaterialEditingLibrary.create_material_expression(
            material,unreal.MaterialExpressionTextureSample,-600,index*240)
        sample.set_editor_property('texture',image)
        if kind=='NormalGL':sample.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        elif kind=='ORM':sample.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
        samples[kind]=sample
    properties={'Color':unreal.MaterialProperty.MP_BASE_COLOR,
                'NormalGL':unreal.MaterialProperty.MP_NORMAL,
                'Emission':unreal.MaterialProperty.MP_EMISSIVE_COLOR}
    for kind,property in properties.items():
        if kind in samples:unreal.MaterialEditingLibrary.connect_material_property(samples[kind],'RGB',property)
    for channel,property in [('R',unreal.MaterialProperty.MP_AMBIENT_OCCLUSION),
                              ('G',unreal.MaterialProperty.MP_ROUGHNESS),('B',unreal.MaterialProperty.MP_METALLIC)]:
        unreal.MaterialEditingLibrary.connect_material_property(samples['ORM'],channel,property)
    unreal.MaterialEditingLibrary.recompile_material(material)
    for asset in textures+[material]:
        assert unreal.EditorAssetLibrary.save_loaded_asset(asset)
    return material


# La cara tiene 249 vertices y ~18 por ojo: a esa densidad son viables las
# deformaciones de gran escala y no lo es un parpadeo. El umbral original de 7
# morphs no era alcanzable sin rehacer la topologia de la cabeza; se fija en los
# tres autorizados el 2026-09-08. Ver Docs/PENDIENTE_PROTAGONISTA.md.
REQUIRED_MORPHS = ('jaw_open', 'brow_raise', 'brow_furrow')


def assign_materials(mesh,chooser):
    """Asigna materiales por slot en un SkeletalMesh.

    `SkeletalMesh.set_material` no existe en la API de Python de UE 5.7: los
    slots viven en la propiedad `materials`, como structs `SkeletalMaterial`.
    Hay que reescribir el array entero, porque los structs que devuelve la
    propiedad son copias.
    """
    slots=mesh.get_editor_property('materials')
    updated=[]
    for index,slot in enumerate(slots):
        material=chooser(index)
        if material is not None:
            slot.set_editor_property('material_interface',material)
        updated.append(slot)
    mesh.set_editor_property('materials',updated)
    return len(updated)


def finish_appearance(body,skeleton,destination,source):
    import ValidateMainCharacterImport as importer
    materials={region:pbr(region,destination,source) for region in ('Character','Suit','Gear','Helmet')}
    body_slots={0:materials['Character'],1:materials['Suit']}
    assign_materials(body,body_slots.get)
    props=[]
    for name,region in [('SK_Astraeon_Helmet','Helmet'),('SK_Astraeon_Backpack','Gear'),
                         ('SK_Astraeon_WristComputer','Gear')]:
        assets=importer.import_fbx(source/'exports'/f'{name}.fbx',name,False,skeleton)
        mesh=next(a for a in assets if isinstance(a,unreal.SkeletalMesh))
        # Cada pieza llega con los cinco slots del generador
        # (Ceramic/Graphite/Bronze/Visor/Display); el set horneado de la region
        # ya los resuelve, asi que se asignan todos y no solo el slot 0.
        count=assign_materials(mesh,lambda index:materials[region])
        assert mesh.get_editor_property('skeleton')==skeleton
        assert unreal.EditorAssetLibrary.save_loaded_asset(mesh)
        props.append(dict(asset=mesh.get_path_name(),material_slots=count))
    morphs=body.get_editor_property('morph_targets')
    names={m.get_name() for m in morphs}
    missing=[m for m in REQUIRED_MORPHS if m not in names]
    assert not missing, 'Missing facial shape keys: '+', '.join(missing)
    return dict(materials=[m.get_path_name() for m in materials.values()],
                equipment=props,morph_targets=sorted(names),
                blink='not authored: ~18 vertices per eye, see Docs/PENDIENTE_PROTAGONISTA.md',
                normal_green_inverted=True)
