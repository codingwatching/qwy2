
/* This file may be overwritten or removed by the buildsystem.
 * Do not modify, see "src/embedded.hpp" or "buildsystem/embed.py" instead. */


/* Content of "src/shaders/classic/classic.vert". */
extern char const g_shader_source_classic_vert[] = "\n#version 430 core\n\nlayout(location = 0) in vec3 in_coords;\nlayout(location = 1) in vec3 in_normal;\nlayout(location = 2) in vec2 in_atlas_coords;\nlayout(location = 3) in vec2 in_atlas_coords_min;\nlayout(location = 4) in vec2 in_atlas_coords_max;\nlayout(location = 5) in float in_ambient_occlusion;\n\nlayout(location = 0) uniform mat4 u_user_camera;\nlayout(location = 5) uniform vec3 u_user_camera_direction;\nlayout(location = 2) uniform mat4 u_sun_camera;\n\nout vec2 v_atlas_coords;\nout vec2 v_atlas_coords_min;\nout vec2 v_atlas_coords_max;\nout vec3 v_normal;\nout vec3 v_sun_camera_space_coords;\nout vec3 v_coords;\nout float v_ambient_occlusion;\n\nvoid main()\n{\n\tgl_Position = u_user_camera * vec4(in_coords, 1.0);\n\n\tv_atlas_coords = in_atlas_coords;\n\tv_atlas_coords_min = in_atlas_coords_min;\n\tv_atlas_coords_max = in_atlas_coords_max;\n\n\tv_normal = in_normal;\n\t\n\t/* Coords of the vertex in the sun camera space,\n\t * and then in the shadow depth buffer space (0.0 ~ 1.0 instead of -1.0 ~ +1.0),\n\t * to compare the fragment depths to their shadow depth buffer analog. */\n\tvec4 sun_coords = u_sun_camera * vec4(in_coords, 1.0);\n\tv_sun_camera_space_coords = sun_coords.xyz / sun_coords.w; //uhu..\n\tv_sun_camera_space_coords.xyz = (v_sun_camera_space_coords.xyz + 1.0) / 2.0;\n\n\tv_coords = in_coords;\n\n\tv_ambient_occlusion = in_ambient_occlusion;\n}\n";

/* Content of "src/shaders/classic/classic.frag". */
extern char const g_shader_source_classic_frag[] = "\n#version 430 core\n\nin vec2 v_atlas_coords;\nin vec2 v_atlas_coords_min;\nin vec2 v_atlas_coords_max;\nin vec3 v_normal;\nin vec3 v_sun_camera_space_coords;\nin vec3 v_coords;\nin float v_ambient_occlusion;\n\nlayout(location =  1) uniform sampler2D u_atlas;\nlayout(location =  6) uniform float u_atlas_side;\nlayout(location =  3) uniform sampler2D u_shadow_depth;\nlayout(location =  4) uniform vec3 u_sun_camera_direction;\nlayout(location =  7) uniform vec3 u_user_coords;\nlayout(location =  8) uniform vec3 u_fog_color;\nlayout(location =  9) uniform float u_fog_distance_inf;\nlayout(location = 10) uniform float u_fog_distance_sup;\n\nout vec4 out_color;\n\nvoid main()\n{\n\t/* Clamp atlas coords in the assigned texture to stop bleeding. */\n\t/* TODO: Do it in the mesh construction! Is it possible tho ? */\n\tconst float texel_side = (1.0 / u_atlas_side) / 2.0;\n\tconst vec2 atlas_coords = clamp(v_atlas_coords,\n\t\tv_atlas_coords_min + vec2(1.0, 1.0) * texel_side,\n\t\tv_atlas_coords_max - vec2(1.0, 1.0) * texel_side);\n\n\tout_color = texture(u_atlas, atlas_coords);\n\tif (out_color.a < 0.001)\n\t{\n\t\tdiscard;\n\t}\n\n\t/* Shadow calculation and effect. */\n\t/* TODO: Make `shadow_ratio` a parameter. */\n\t/* TODO: Make `ao_ratio_max` a parameter. */\n\tfloat light = -dot(v_normal, normalize(u_sun_camera_direction));\n\tconst float shadow_depth = texture(u_shadow_depth, v_sun_camera_space_coords.xy).r;\n\tconst bool is_in_shadow = v_sun_camera_space_coords.z > shadow_depth;\n\tif (is_in_shadow || light < 0.0)\n\t{\n\t\tlight *= 0.0;\n\t}\n\tconst float shadow_ratio = 0.7; /* How dark is it in the shadows. */\n\tout_color.rgb *= light * shadow_ratio + (1.0 - shadow_ratio);\n\tconst float ao_ratio_max = 0.7; /* How dark is it in corners (ambiant occlusion). */\n\tconst float ao_ratio = ao_ratio_max / (light + 1.0); \n\tout_color.rgb *= v_ambient_occlusion * ao_ratio + (1.0 - ao_ratio);\n\n\t/* Sun gold-ish color. */\n\t/* TODO: Make `sun_light_color` a parameter. */\n\tconst vec3 sun_light_color = vec3(0.5, 0.35, 0.0);\n\tout_color.rgb = mix(out_color.rgb,\n\t\tout_color.rgb * (vec3(1.0, 1.0, 1.0) + sun_light_color),\n\t\tlight);\n\n\t/* Fog effect. */\n\tconst float distance_to_user = distance(v_coords, u_user_coords);\n\t//const float fog_ratio =\n\t//\t(clamp(distance_to_user, u_fog_distance_inf, u_fog_distance_sup) - u_fog_distance_inf)\n\t//\t/ (u_fog_distance_sup - u_fog_distance_inf);\n\tconst float fog_ratio = smoothstep(u_fog_distance_inf, u_fog_distance_sup, distance_to_user);\n\tout_color.rgb = mix(out_color.rgb, u_fog_color, fog_ratio);\n}\n";

/* Content of "src/shaders/line/line.vert". */
extern char const g_shader_source_line_vert[] = "\n#version 430 core\n\nlayout(location = 0) in vec3 in_coords;\nlayout(location = 1) in vec3 in_color;\n\nlayout(location = 0) uniform mat4 user_camera;\n\nout vec3 v_color;\n\nvoid main()\n{\n\tgl_Position = user_camera * vec4(in_coords, 1.0);\n\n\tv_color = in_color;\n}\n";

/* Content of "src/shaders/line/line.frag". */
extern char const g_shader_source_line_frag[] = "\n#version 430 core\n\nin vec3 v_color;\n\nout vec4 out_color;\n\nvoid main()\n{\n\tout_color = vec4(v_color, 1.0);\n}\n";

/* Content of "src/shaders/shadow/shadow.vert". */
extern char const g_shader_source_shadow_vert[] = "\n#version 430 core\n\nlayout(location = 0) in vec3 in_coords;\nlayout(location = 1) in vec2 in_atlas_coords;\n\nlayout(location = 0) uniform mat4 sun_camera;\n\nout vec2 v_atlas_coords;\n\nvoid main()\n{\n\tgl_Position = sun_camera * vec4(in_coords, 1.0);\n\tv_atlas_coords = in_atlas_coords;\n}\n";

/* Content of "src/shaders/shadow/shadow.frag". */
extern char const g_shader_source_shadow_frag[] = "\n#version 430 core\n\nin vec2 v_atlas_coords;\n\nlayout(location = 1) uniform sampler2D u_atlas;\n\nvoid main()\n{\n\t/* Here there is no need to carefully avoid atlas bleeding it seems. */\n\n\tvec4 out_color = texture(u_atlas, v_atlas_coords);\n\tif (out_color.a < 0.001)\n\t{\n\t\tdiscard;\n\t}\n\n\t/* We only checked for transparent `out_color`, but we then do nothing with it\n\t * as the only thing we care about is the Z-buffer. */\n}\n";

/* Content of "src/shaders/simple/simple.vert". */
extern char const g_shader_source_simple_vert[] = "\n#version 430 core\n\nlayout(location = 0) in vec3 in_coords;\nlayout(location = 1) in vec3 in_normal;\nlayout(location = 2) in vec3 in_color;\n\nlayout(location = 0) uniform mat4 u_user_camera;\nlayout(location = 5) uniform vec3 u_user_camera_direction;\nlayout(location = 2) uniform mat4 u_sun_camera;\n\nout vec3 v_normal;\nout vec3 v_sun_camera_space_coords;\nout vec3 v_coords;\nout vec3 v_color;\n\nvoid main()\n{\n\tgl_Position = u_user_camera * vec4(in_coords, 1.0);\n\n\tv_color = in_color;\n\n\tv_normal = in_normal;\n\t\n\t/* Coords of the vertex in the sun camera space,\n\t * and then in the shadow depth buffer space (0.0 ~ 1.0 instead of -1.0 ~ +1.0),\n\t * to compare the fragment depths to their shadow depth buffer analog. */\n\tvec4 sun_coords = u_sun_camera * vec4(in_coords, 1.0);\n\tv_sun_camera_space_coords = sun_coords.xyz / sun_coords.w; //uhu..\n\tv_sun_camera_space_coords.xyz = (v_sun_camera_space_coords.xyz + 1.0) / 2.0;\n\n\tv_coords = in_coords;\n}\n";

/* Content of "src/shaders/simple/simple.frag". */
extern char const g_shader_source_simple_frag[] = "\n#version 430 core\n\nin vec3 v_normal;\nin vec3 v_sun_camera_space_coords;\nin vec3 v_coords;\nin vec3 v_color;\n\nlayout(location =  3) uniform sampler2D u_shadow_depth;\nlayout(location =  4) uniform vec3 u_sun_camera_direction;\nlayout(location =  7) uniform vec3 u_user_coords;\nlayout(location =  8) uniform vec3 u_fog_color;\nlayout(location =  9) uniform float u_fog_distance_inf;\nlayout(location = 10) uniform float u_fog_distance_sup;\n\nout vec4 out_color;\n\nvoid main()\n{\n\tout_color = vec4(v_color, 1.0);\n\n\t/* Shadow calculation and effect. */\n\t/* TODO: Make `shadow_ratio` a parameter. */\n\tfloat light = -dot(v_normal, normalize(u_sun_camera_direction));\n\tconst float shadow_depth = texture(u_shadow_depth, v_sun_camera_space_coords.xy).r;\n\tconst bool is_in_shadow = v_sun_camera_space_coords.z > shadow_depth;\n\tif (is_in_shadow || light < 0.0)\n\t{\n\t\tlight *= 0.0;\n\t}\n\tconst float shadow_ratio = 0.7; /* How dark is it in the shadows. */\n\tout_color.rgb *= light * shadow_ratio + (1.0 - shadow_ratio);\n\n\t/* Sun gold-ish color. */\n\t/* TODO: Make `sun_light_color` a parameter. */\n\tconst vec3 sun_light_color = vec3(0.5, 0.35, 0.0);\n\tout_color.rgb = mix(out_color.rgb,\n\t\tout_color.rgb * (vec3(1.0, 1.0, 1.0) + sun_light_color),\n\t\tlight);\n\n\t/* Fog effect. */\n\tconst float distance_to_user = distance(v_coords, u_user_coords);\n\t//const float fog_ratio =\n\t//\t(clamp(distance_to_user, u_fog_distance_inf, u_fog_distance_sup) - u_fog_distance_inf)\n\t//\t/ (u_fog_distance_sup - u_fog_distance_inf);\n\tconst float fog_ratio = smoothstep(u_fog_distance_inf, u_fog_distance_sup, distance_to_user);\n\tout_color.rgb = mix(out_color.rgb, u_fog_color, fog_ratio);\n}\n";

/* Content of "src/shaders/simple_shadow/simple_shadow.vert". */
extern char const g_shader_source_simple_shadow_vert[] = "\n#version 430 core\n\nlayout(location = 0) in vec3 in_coords;\n\nlayout(location = 0) uniform mat4 sun_camera;\n\nvoid main()\n{\n\tgl_Position = sun_camera * vec4(in_coords, 1.0);\n}\n";

/* Content of "src/shaders/simple_shadow/simple_shadow.frag". */
extern char const g_shader_source_simple_shadow_frag[] = "\n#version 430 core\n\nvoid main()\n{\n\t/* We only care about the Z-buffer. */\n}\n";

/* Content of "src/shaders/line_ui/line_ui.vert". */
extern char const g_shader_source_line_ui_vert[] = "\n#version 430 core\n\nlayout(location = 0) in vec2 in_coords;\nlayout(location = 1) in vec3 in_color;\n\nout vec3 v_color;\n\nvoid main()\n{\n\tgl_Position = vec4(in_coords, 0.0, 1.0);\n\n\tv_color = in_color;\n}\n";

/* Content of "src/shaders/line_ui/line_ui.frag". */
extern char const g_shader_source_line_ui_frag[] = "\n#version 430 core\n\nin vec3 v_color;\n\nout vec4 out_color;\n\nvoid main()\n{\n\tout_color = vec4(v_color, 1.0);\n}\n";

/* Content of "src/default_commands.qwy2". */
extern char const g_default_commands[] = "\nlog \"Commands start.\"\n\n# ZQSD walking controls (for AZERTY keyboards).\n# TODO: Make it so that the default controls can adapt to keyboard layout or something.\nbind_control KD:z [player_move_forward]\nbind_control KU:z [player_move_backward]\nbind_control KD:s [player_move_backward]\nbind_control KU:s [player_move_forward]\nbind_control KD:d [player_move_rightward]\nbind_control KU:d [player_move_leftward]\nbind_control KD:q [player_move_leftward]\nbind_control KU:q [player_move_rightward]\n\n# Controls (inspired from the default Minecraft controls).\nbind_control KD:space [player_jump]\nbind_control MD:left  [player_break_block]\nbind_control MD:right [player_place_block]\nbind_control KD:lctrl [toggle_fast_and_infinite_jumps]\nbind_control KD:f5    [toggle_see_from_behind]\n\n# Some other useful controls.\n# Note that according to https://minecraft.fandom.com/wiki/Controls?file=Kbd-minecraft.svg#Semi-configurable_controls\n# the F8, F9, F10 and F12 keys are not default-bound to controls in Minecraft,\n# so there is no risk of conflict when default-binding stuff to these keys here.\n# Same for V, B and N (which are at the bottom of both AZERTY and QWERTY keyboards).\nbind_control KD:escape [quit_game]\nbind_control KD:f8     [toggle_see_from_sun]\nbind_control KD:f9     [toggle_capture_cursor]\nbind_control KD:f10    [toggle_see_chunk_borders]\nbind_control KD:n      [teleport_relative_player 0 0 30]\nbind_control KD:x      [spawn_entity_on_player]\n\n# Note: Here are some of MY cringe controls (which I can't play without, but\n# also which you probably can't play with, which is why controls were made\n# configurable so early in development).\n#bind_control KD:a [player_place_block]\n#bind_control MD:left [player_break_block]\n#bind_control MD:right [player_jump]\n# These are not particularly cringe.\n#bind_control KD:l [toggle_capture_cursor]\n#bind_control KD:m [toggle_see_from_sun]\n#bind_control KD:j [teleport_player 0 0 0]\n#bind_control KD:u [teleport_relative_player 0 0 30]\n#bind_control KD:f [toggle_fast_and_infinite_jumps]\n\nlog \"Commands end.\"\n";
