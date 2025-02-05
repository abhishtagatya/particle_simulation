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

	// Variables (Frame Buffers)
protected:
	// Variables (GUI)
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
	glm::vec3 attraction_point = glm::vec3(0.0f, 0.0f, 0.0f);
	float attraction_force = 9.8f;

protected:
	/** The constants identifying what can be displayed on the screen. */
	const int DISPLAY_PULSATING_SCENE = 0;
	const int DISPLAY_ATTRACTING_SCENE = 1;
	
	const char* DISPLAY_NAMES[2] = { "Sphere Pulsating Simulation", "Attracting Simulation"};

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

	// Render Modes
public:
	/** Render Sphere Pulsating Simulation (DISPLAY_PULSATING_SCENE) */
	void render_pulsating_simulation();

	/** Render Attracting Particles Simulation (DISPLAY_ATTRACTING_SCENE) */
	void render_attracting_simulation();

public:
	// Render
	void render() override;

public:
	// Render UI
	void render_ui() override;

public:
	// On Resize Callbak
	void on_resize(int width, int height) override;
};