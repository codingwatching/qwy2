
#include "window.hpp"
#include "shaders/shader.hpp"
#include "shaders/blocks/blocks.hpp"
#include "camera.hpp"
#include "chunk.hpp"
#include "nature.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <cstdlib>
#include <iostream>
#include <chrono>
#include <cmath>
#include <vector>
#include <cstdint>

int main()
{
	using namespace qwy2;

	if (init_window_graphics() == ErrorCode::ERROR)
	{
		return EXIT_FAILURE;
	}

	glEnable(GL_DEPTH_TEST);


	Nature nature;
	nature.world_generator.primary_block_type =
		nature.nature_generator.generate_block_type(nature);

	UniformValues uniform_values;
	uniform_values.atlas_opengltextureid = nature.atlas.opengltextureid;

	ShaderProgramBlocks shader_program_blocks;
	if (shader_program_blocks.init() == ErrorCode::ERROR)
	{
		std::cerr << "Error occured during shader compilation" << std::endl;
		return EXIT_FAILURE;
	}


	ChunkGrid chunk_grid(9);
	const ChunkRect generated_chunk_rect =
		ChunkRect(ChunkCoords(-5, -5, -5), ChunkCoords(5, 5, 5));
	ChunkCoords walker = generated_chunk_rect.walker_start();
	do
	{
		chunk_grid.generate_chunk(nature, walker);
	}
	while (generated_chunk_rect.walker_iterate(walker));


	Camera camera;

	glm::vec3 player_position{0.0f, 0.0f, 0.0f};
	float player_horizontal_angle = 0.0f;
	float player_vertical_angle = 0.0f;

	SDL_Event event;

	bool moving_forward = false;
	bool moving_backward = false;
	bool moving_leftward = false;
	bool moving_rightward = false;
	const float moving_factor = 0.1f;
	const float flying_moving_factor = 0.15f;

	glm::vec3 player_motion{0.0f, 0.0f, 0.0f};
	bool flying = false;
	bool flying_initial = false;
	const float flying_factor = 0.003f;
	const float flying_initial_value = 0.1f;
	const float falling_factor = 0.012f;

	SDL_SetRelativeMouseMode(SDL_TRUE);
	const float moving_angle_factor = 0.005f;


	using clock = std::chrono::high_resolution_clock;
	const auto clock_time_beginning = clock::now();
	float time = 0.0f; /* Time in seconds. */

	bool running = true;
	while (running)
	{
		time = std::chrono::duration<float>(clock::now() - clock_time_beginning).count();

		float horizontal_angle_motion = 0.0f;
		float vertical_angle_motion = 0.0f;
		flying_initial = false;

		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_QUIT:
					running = false;
				break;

				case SDL_KEYDOWN:
				case SDL_KEYUP:
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
							if (event.type == SDL_KEYDOWN)
							{
								running = false;
							}
						break;

						case SDLK_z:
							moving_forward = event.type == SDL_KEYDOWN;
						break;
						case SDLK_s:
							moving_backward = event.type == SDL_KEYDOWN;
						break;
						case SDLK_q:
							moving_leftward = event.type == SDL_KEYDOWN;
						break;
						case SDLK_d:
							moving_rightward = event.type == SDL_KEYDOWN;
						break;

						case SDLK_p:
							for (auto const& [chunk_coords, chunk] : chunk_grid.table)
							{
								chunk->recompute_mesh(nature);
							}
						break;
					}
				break;

				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					if (event.button.button == SDL_BUTTON_RIGHT)
					{
						flying = event.type == SDL_MOUSEBUTTONDOWN;
						flying_initial = event.type == SDL_MOUSEBUTTONDOWN;
					}
				break;

				case SDL_MOUSEMOTION:
					horizontal_angle_motion += -event.motion.xrel * moving_angle_factor;
					vertical_angle_motion += -event.motion.yrel * moving_angle_factor;
				break;
			}
		}

		player_horizontal_angle += horizontal_angle_motion;
		player_vertical_angle += vertical_angle_motion;
		if (player_vertical_angle < -TAU / 4.0f + 0.0001f)
		{
			player_vertical_angle = -TAU / 4.0f + 0.0001f;
		}
		else if (player_vertical_angle > TAU / 4.0f - 0.0001f)
		{
			player_vertical_angle = TAU / 4.0f - 0.0001f;
		}

		glm::vec3 player_horizontal_direction{
			std::cos(player_horizontal_angle),
			std::sin(player_horizontal_angle),
			0.0f};
		glm::vec3 player_horizontal_right{
			std::cos(player_horizontal_angle - TAU / 4.0f),
			std::sin(player_horizontal_angle - TAU / 4.0f),
			0.0f};
		float current_moving_factor =
			flying ? flying_moving_factor : moving_factor;
		float forward_motion = current_moving_factor *
			((moving_forward ? 1.0f : 0.0f) - (moving_backward ? 1.0f : 0.0f));
		float rightward_motion = current_moving_factor *
			((moving_rightward ? 1.0f : 0.0f) - (moving_leftward ? 1.0f : 0.0f));

		Chunk* player_chunk = chunk_grid.containing_chunk(player_position);
		if (player_chunk == nullptr)
		{
			std::cerr << "Exiting the world" << std::endl;
			return 0;
		}
		BlockCoords player_coords_int{
			static_cast<int>(std::round(player_position.x)),
			static_cast<int>(std::round(player_position.y)),
			static_cast<int>(std::round(player_position.z))};
		bool is_in_block = not player_chunk->block(player_coords_int).is_air;

		if (flying)
		{
			if (flying_initial)
			{
				player_motion.z = flying_initial_value;
			}
			else
			{
				player_motion.z += flying_factor;
			}
		}
		else if (not is_in_block)
		{
			player_motion.z -= falling_factor;
		}
		else
		{
			player_motion.z = 0.0f;
			player_position.z = std::round(player_position.z) + 0.5f;
		}
		player_position +=
			player_horizontal_direction * forward_motion +
			player_horizontal_right * rightward_motion +
			player_motion;

		glm::vec3 player_direction = glm::rotate(player_horizontal_direction,
			player_vertical_angle, player_horizontal_right);

		camera.set_position(player_position + glm::vec3(0.0f, 0.0f, 2.0f));
		camera.set_direction(player_direction);
		uniform_values.camera_matrix = camera.matrix;
		shader_program_blocks.update_uniforms(uniform_values);
		
		const float sky_variation = (std::cos(time / 8.0f) + 1.0f) / 2.0f;
		glClearColor(
			0.0f + sky_variation * 0.2f,
			0.7f - sky_variation * 0.2f,
			0.9f - sky_variation * 0.4f,
			1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		for (auto const& [chunk_coords, chunk] : chunk_grid.table)
		{
			shader_program_blocks.draw(chunk->mesh.openglid, chunk->mesh.vertex_data.size());
		}

		SDL_GL_SwapWindow(g_window);
	}

	cleanup_window_graphics();

	return 0;
}
