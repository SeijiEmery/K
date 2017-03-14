
scene.name    = "scene01"
scene.require_version("1.0")

assets.addPath("../assets")
assets.addPath("${PROJECT_SRC}/components/")

# assets.load({
#     dragon_mesh: "dragon.kmesh",
#     dragon_mtl:  "dragon.kmaterial",
#     camera_controller: "camera_controller.kmodule",
# })

create_entity("dragon")
    .transform(pos=(0,0,0),rot_degrees=(180,0,45))
    .mesh(asset="dragon.kmesh",origin:(0,0,0),scale:1.0)
    .material(asset="dragon.kmaterial")

create_entity("mainCamera")
    .camera(type="perspective",fov=60.0,near=0.0,far=100.0)
    .transform(pos=(0,-10,0),look_at=entities['dragon'],parent=entities['dragon'])
    .module(asset="camera_controller.kmodule")
