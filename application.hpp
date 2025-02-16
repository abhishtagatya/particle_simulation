#pragma once

#include "camera_ubo.hpp"
#include "light_ubo.hpp"
#include "phong_material_ubo.hpp"
#include "pv227_application.hpp"
#include "ubo_impl.hpp"

struct Particle {
	glm::vec4 position; // The position of particles (on CPU).
	glm::vec3 velocity; // The velocity of particles (on CPU).
	float lifetime; // The lifetime of the particle
	glm::vec3 color; // The colors of all particles (on GPU).
	float remaining; // The remaining time of the particle
};

struct Mesh {
	std::vector<glm::vec4> positions;
	std::vector<int> indices;
};

class Application : public PV227Application {
	// Variables (Geometry)
protected:
	// Variables (Textures)
protected:
	GLuint star_tex;
	// Variables (Light)
protected:
	// Variables (Camera)
protected:
	CameraUBO camera_ubo;
	// Variables (Shaders)
protected:
	ShaderProgram pulsating_particle_program;
	ShaderProgram attracting_particle_program;
	ShaderProgram multi_attracting_particle_program;
	ShaderProgram nbody_particle_program;
	ShaderProgram nbody_update_program;
	ShaderProgram particle_surface_estimator_program;

	// Variables (Frame Buffers)
protected:
	// Variables (GUI)
	bool show_ui = true;
protected:
	// Variables (Particles)

	// The desired number of particles.
	int desired_particle_count = 4096;

	// The current number of particles.
	int current_particle_count = desired_particle_count;

	// The maximum number of particles.
	int max_particle_count = 4194304;

	// The particle size.
	float particle_size = 0.5f;

	// The particle data.
	std::vector<Particle> particles;

	// The particle buffer.
	GLuint particle_buffer;

	// -- Attracting Particles --
	const int max_attractors = 10;
	int attractor_used = 3;
	std::vector<glm::vec3> attraction_points;
	float attraction_force = 9.8f;

	// -- N-Body Particles --
	std::vector<glm::vec4> particle_positions[2];
	std::vector<glm::vec4> particle_velocities;
	GLuint particle_positions_buffer[2];
	GLuint particle_velocities_buffer;
	GLuint particle_colors_buffer;
	GLuint particle_vao[2];

	int current_read = 0;
	int current_write = 1;

	float acceleration_factor = 0.2f;
	float distance_threshold = 0.01f;

	// This must be the same as 'layout (local_size_x = 256) in;' in nbody_(shared).comp
	const int local_size_x = 256;

	float compute_fps_gpu;

	// -- Particle Surface Estimator --
	Mesh mesh;
	GLuint mesh_position_buffer;
	GLuint mesh_index_buffer;

	const std::string GOLEM_MODEL = "models/golem.obj";
	const std::string CUBE_MODEL = "models/cube.obj";
	const std::string TORUS_MODEL = "models/torus.obj";
	const std::string MONKEY_MODEL = "models/monkey.obj";
	const std::string STATUE_MODEL = "models/statue.obj";
	const std::string TOWER_MODEL = "models/tower.obj";

	const int SELECT_GOLEM_MODEL = 0;
	const int SELECT_CUBE_MODEL = 1;
	const int SELECT_TORUS_MODEL = 2;
	const int SELECT_MONKEY_MODEL = 3;
	const int SELECT_STATUE_MODEL = 4;
	const int SELECT_TOWER_MODEL = 5;

	const char* MODEL_PATHS[6] = { GOLEM_MODEL.c_str(), CUBE_MODEL.c_str(), TORUS_MODEL.c_str(), MONKEY_MODEL.c_str(), STATUE_MODEL.c_str(), TOWER_MODEL.c_str() };
	const char* MODEL_NAMES[6] = { "Golem", "Cube", "Torus", "Monkey", "Statue", "Tower"};

	int current_model = SELECT_GOLEM_MODEL;

	int model_vertex_count = 0;
	int model_index_count = 0;

protected:
	/** The constants identifying what can be displayed on the screen. */
	const int DISPLAY_PULSATING_SCENE = 0;
	const int DISPLAY_SINGLE_ATTRACTOR_SCENE = 1;
	const int DISPLAY_MULTI_ATTRACTOR_SCENE = 2;
	const int DISPLAY_NBODY_SCENE = 3;
	const int DISPLAY_PARTICLE_SURFACE_ESTIMATOR_SCENE = 4;
	
	const char* DISPLAY_NAMES[5] = { "Sphere Pulsating", "Single Attractor", "Multi Attractor", "N-Body", "Particle-Surface Est."};

	int display_mode = DISPLAY_PULSATING_SCENE;

	/** Global Time Delta */
	float t_delta = 0;

public:
	Application(int initial_width, int initial_height, std::vector<std::string> arguments = {});

	/** Destroys the {@link Application} and releases the allocated resources. */
	virtual ~Application();

	// Shaders
	void compile_shaders() override;
public:
	/** Prepares the required cameras. */
	void prepare_cameras();

	/** Prepares the required materials. */
	void prepare_materials();

	/** Prepares the required textures. */
	void prepare_textures();

	/** Prepares the lights. */
	void prepare_lights();

	/** Prepares the scene objects. */
	void prepare_scene();

	/** Prepares the frame buffer objects. */
	void prepare_framebuffers();

	/** Resizes the full screen textures match the window. */
	void resize_fullscreen_textures();

	/** Resets the particles */
	void reset_particles();

	/** Updates the particles on CPU */
	void update_particles_buffer();

	// Update
	void update(float delta) override;

	/** Updates the particles on GPU */
	void update_particles_gpu();

	/** Updates the selected model */
	void update_model();

	// Render Modes
public:
	/** Render Sphere Pulsating Simulation (DISPLAY_PULSATING_SCENE) */
	void render_pulsating_simulation();

	/** Render Attracting Particles Simulation (DISPLAY_SINGLE_ATTRACTOR_SCENE) */
	void render_attracting_simulation();

	/** Render Attracting Particles Simulation (DISPLAY_MULTI_ATTRACTOR_SCENE) */
	void render_multi_attracting_simulation();

	/** Render N-Body Simulation (DISPLAY_NBODY_SCENE) */
	void render_nbody_simulation();

	/** Render Particle Surface Estimator (DISPLAY_PARTICLE_SURFACE_ESTIMATOR_SCENE) */
	void render_surface_estimator();

public:
	// Render
	void render() override;

public:
	// Render UI
	void render_ui() override;

public:
	// On Resize Callbak
	void on_resize(int width, int height) override;

public:
	// On Key Pressed Callback
	void on_key_pressed(int key, int scancode, int action, int mods) override;

public:
	// Helper Functions
	glm::vec3 random_inside_sphere(float radius);
	void from_file(std::filesystem::path path, bool center_vertices);
};