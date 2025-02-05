#include "application.hpp"
#include "model_ubo.hpp"
#include "utils.hpp"
#include <random>

Application::Application(int initial_width, int initial_height, std::vector<std::string> arguments)
	: PV227Application(initial_width, initial_height, arguments) {
	
	// Debug Memory Size
	GLint size;
	glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &size);
	std::cout << "GL_MAX_SHADER_STORAGE_BLOCK_SIZE is " << size << " bytes." << std::endl;
	std::cout << "Size of Particle: " << sizeof(Particle) << " bytes." << std::endl;
	std::cout << "Alignment of Particle: " << alignof(Particle) << " bytes." << std::endl;
	
	Application::compile_shaders();
	prepare_cameras();
	prepare_materials();
	prepare_textures();
	prepare_lights();
	prepare_scene();
	prepare_framebuffers();
}

Application::~Application() {}

// Shaders
void Application::compile_shaders() {
	default_unlit_program = ShaderProgram(lecture_shaders_path / "object.vert", lecture_shaders_path / "unlit.frag");
	default_lit_program = ShaderProgram(lecture_shaders_path / "object.vert", lecture_shaders_path / "lit.frag");

	pulsating_particle_program = ShaderProgram();
	pulsating_particle_program.add_vertex_shader(lecture_shaders_path / "pulsating_particle.vert");
	pulsating_particle_program.add_fragment_shader(lecture_shaders_path / "pulsating_particle.frag");
	pulsating_particle_program.add_geometry_shader(lecture_shaders_path / "pulsating_particle.geom");
	pulsating_particle_program.link();

	attracting_particle_program = ShaderProgram();
	attracting_particle_program.add_vertex_shader(lecture_shaders_path / "attracting_particle.vert");
	attracting_particle_program.add_fragment_shader(lecture_shaders_path / "attracting_particle.frag");
	attracting_particle_program.add_geometry_shader(lecture_shaders_path / "attracting_particle.geom");
	attracting_particle_program.link();

	std::cout << "Shaders are reloaded." << std::endl;
}

// Initialize Scene
void Application::prepare_cameras() {
	// Sets the default camera position.
	camera.set_eye_position(glm::radians(-45.f), glm::radians(20.f), 25.f);
	// Computes the projection matrix.
	camera_ubo.set_projection(
		glm::perspective(glm::radians(45.f), static_cast<float>(this->width) / static_cast<float>(this->height), 1.0f, 1000.0f));
	camera_ubo.update_opengl_data();
}

// Materials
void Application::prepare_materials() {}

// Textures
void Application::prepare_textures() {
	star_tex = TextureUtils::load_texture_2d(lecture_textures_path / "star.png");
	// Particles are really small, use mipmaps for them.
	TextureUtils::set_texture_2d_parameters(star_tex, GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
}

// Lights
void Application::prepare_lights() {}

// Scenes
void Application::prepare_scene() {
	// Initializes the particles.
	particles.resize(max_particle_count); // Resize the vector to the maximum number of particles.
	
	// Initializes the particle buffer.
	glGenBuffers(1, &particle_buffer);
	reset_particles();
}

// Framebuffers
void Application::prepare_framebuffers() {}

// On Resize
void Application::resize_fullscreen_textures() {}

// Reset Particles
void Application::reset_particles() {
	// Initialize random number generators
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> real_dist(0.0f, 5.0f);  // Random lifetime

	if (display_mode == DISPLAY_PULSATING_SCENE) {

		for (int i = 0; i < current_particle_count; i++) {
			// Initializes the particle position.
			particles[i].position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
			particles[i].lifetime = real_dist(gen);
			particles[i].remaining = particles[i].lifetime;
		}
	}
	else if (display_mode == DISPLAY_ATTRACTING_SCENE) {

		std::uniform_real_distribution<> pos_dist(-50.0f, 50.f);  // Random position value
		std::uniform_real_distribution<> vel_dist(-10.0f, 10.f);  // Random position value

		// Zeroes the particle data.
		for (int i = 0; i < current_particle_count; i++) {
			// Initializes the particle position.
			particles[i].position = glm::vec4(pos_dist(gen), pos_dist(gen), pos_dist(gen), 1.0f);
			particles[i].velocity = glm::vec3(vel_dist(gen), vel_dist(gen), vel_dist(gen));
			particles[i].lifetime = real_dist(gen);
			particles[i].remaining = particles[i].lifetime;
		}
	}

	// Updates the particle buffer.
	update_particles_buffer();
}

// Update Particles Buffer
void Application::update_particles_buffer() {
	current_particle_count = desired_particle_count;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, particle_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Particle) * current_particle_count, particles.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	std::cout << "Desired particle count: " << current_particle_count << std::endl;
	std::cout << "Particles buffer size: " << sizeof(Particle) * current_particle_count << " bytes." << std::endl;
	std::cout << "Particles buffer updated." << std::endl;
}

// Update
void Application::update(float delta) {
	PV227Application::update(delta);

	// Updates the main camera.
	const glm::vec3 eye_position = camera.get_eye_position();
	camera_ubo.set_view(lookAt(eye_position, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
	camera_ubo.update_opengl_data();

	// Updates the global time delta.
	t_delta = delta;
}

// Pulsating Simulation (DISPLAY_PULSATING_SCENE)
void Application::render_pulsating_simulation() {
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	
	pulsating_particle_program.use();
	pulsating_particle_program.uniform("t_time", (float)elapsed_time);
	pulsating_particle_program.uniform("t_delta", (float)t_delta * 0.0001f);
	pulsating_particle_program.uniform("particle_size_vs", particle_size);

	// Binds the particle texture.
	glBindTextureUnit(0, star_tex);

	// Binds the particle buffer.
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, particle_buffer);

	// Renders the particles.
	glBindVertexArray(empty_vao);
	glDrawArrays(GL_POINTS, 0, current_particle_count);

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}

// Attracting Simulation (DISPLAY_ATTRACTING_SCENE)
void Application::render_attracting_simulation() {

	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	attracting_particle_program.use();
	attracting_particle_program.uniform("t_time", (float)elapsed_time);
	attracting_particle_program.uniform("t_delta", (float)t_delta * 0.0001f);
	attracting_particle_program.uniform("particle_size_vs", particle_size);
	attracting_particle_program.uniform("attractor_point", attraction_point);
	attracting_particle_program.uniform("attractor_force", attraction_force);

	// Binds the particle texture.
	glBindTextureUnit(0, star_tex);

	// Binds the particle buffer.
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, particle_buffer);

	// Renders the particles.
	glBindVertexArray(empty_vao);
	glDrawArrays(GL_POINTS, 0, current_particle_count);

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}

// Render Scene
void Application::render() {
	// Starts measuring the elapsed time.
	glBeginQuery(GL_TIME_ELAPSED, render_time_query);

	// Binds the main window framebuffer.
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, width, height);

	// Clears the framebuffer color.
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	// Binds the necessary buffers.
	camera_ubo.bind_buffer_base(CameraUBO::DEFAULT_CAMERA_BINDING);

	if (display_mode == DISPLAY_PULSATING_SCENE) {
		render_pulsating_simulation();
	}
	else if (display_mode == DISPLAY_ATTRACTING_SCENE) {
		render_attracting_simulation();
	}

	// Resets the VAO and the program.
	glBindVertexArray(0);
	glUseProgram(0);

	// Stops measuring the elapsed time.
	glEndQuery(GL_TIME_ELAPSED);

	// Waits for OpenGL - don't forget OpenGL is asynchronous.
	glFinish();

	// Evaluates the query.
	GLuint64 render_time;
	glGetQueryObjectui64v(render_time_query, GL_QUERY_RESULT, &render_time);
	fps_gpu = 1000.f / (static_cast<float>(render_time) * 1e-6f);
}

// GUI
void Application::render_ui() {
	const float unit = ImGui::GetFontSize();

	ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	std::string fps_cpu_string = "FPS (CPU): ";
	ImGui::Text(fps_cpu_string.append(std::to_string(fps_cpu)).c_str());

	std::string fps_string = "FPS (GPU): ";
	ImGui::Text(fps_string.append(std::to_string(fps_gpu)).c_str());

	if (ImGui::Combo("Display", &display_mode, DISPLAY_NAMES, IM_ARRAYSIZE(DISPLAY_NAMES))) {
		reset_particles();
	}

	if (ImGui::CollapsingHeader("Particle Settings")) {
		const char* particle_labels[15] = {
		"256", "512", "1024", "2048", "4096",
		"8192", "16384", "32768", "65536", "131072",
		"262144", "524288", "1048576", "2097152", "4194304"
		};
		int exponent = static_cast<int>(log2(current_particle_count) - 8);
		if (ImGui::Combo("Particle Count", &exponent, particle_labels, IM_ARRAYSIZE(particle_labels))) {
			desired_particle_count = static_cast<int>(glm::pow(2, exponent + 8));
			update_particles_buffer();
		}

		ImGui::SliderFloat("Particle Size", &particle_size, 0.1f, 2.0f, "%.1f");

		if (ImGui::Button("Reset Particles", ImVec2(150.f, 0.f))) {
			reset_particles();
		}
	}

	if (ImGui::CollapsingHeader("Scene Controls")) {
		if (display_mode == DISPLAY_ATTRACTING_SCENE) {
			ImGui::InputFloat3("Center", glm::value_ptr(attraction_point));
			ImGui::SliderFloat("Force", &attraction_force, 0.1f, 25.0f, "%.1f");
		}
	}

	ImGui::End();
}

void Application::on_resize(int width, int height) {
	PV227Application::on_resize(width, height);
	resize_fullscreen_textures();
}