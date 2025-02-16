#include "application.hpp"
#include "model_ubo.hpp"
#include "utils.hpp"
#include "tiny_obj_loader.h"
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

	multi_attracting_particle_program = ShaderProgram();
	multi_attracting_particle_program.add_vertex_shader(lecture_shaders_path / "multi_attracting_particle.vert");
	multi_attracting_particle_program.add_fragment_shader(lecture_shaders_path / "multi_attracting_particle.frag");
	multi_attracting_particle_program.add_geometry_shader(lecture_shaders_path / "multi_attracting_particle.geom");
	multi_attracting_particle_program.link();

	nbody_particle_program = ShaderProgram();
	nbody_particle_program.add_vertex_shader(lecture_shaders_path / "nbody_particle.vert");
	nbody_particle_program.add_fragment_shader(lecture_shaders_path / "nbody_particle.frag");
	nbody_particle_program.add_geometry_shader(lecture_shaders_path / "nbody_particle.geom");
	nbody_particle_program.link();

	nbody_update_program = ShaderProgram();
	nbody_update_program.add_compute_shader(lecture_shaders_path / "nbody.comp");
	nbody_update_program.link();

	particle_surface_estimator_program = ShaderProgram();
	particle_surface_estimator_program.add_vertex_shader(lecture_shaders_path / "surface_estimator.vert");
	particle_surface_estimator_program.add_fragment_shader(lecture_shaders_path / "surface_estimator.frag");
	particle_surface_estimator_program.add_geometry_shader(lecture_shaders_path / "surface_estimator.geom");
	particle_surface_estimator_program.link();

	std::cout << "Shaders are reloaded." << std::endl;
}

void Application::from_file(std::filesystem::path path, bool center_vertices) {
	const std::string extension = path.extension().generic_string();

	if (extension == ".obj") {
		tinyobj::ObjReader reader;

		if (!reader.ParseFromFile(path.generic_string())) {
			if (!reader.Error().empty()) {
				std::cerr << "TinyObjReader: " << reader.Error();
			}
		}

		if (!reader.Warning().empty()) {
			std::cerr << "TinyObjReader: " << reader.Warning();
		}

		auto& attrib = reader.GetAttrib();
		auto& shapes = reader.GetShapes();
		auto& materials = reader.GetMaterials();

		const tinyobj::shape_t& shape = shapes[0];

		glm::vec3 min{ std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
		glm::vec3 max{ std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min() };

		mesh.positions.clear();
		mesh.indices.clear();

		int vertex_count = 0;
		for (size_t i = 0; i < attrib.vertices.size() / 3; i++) {
			mesh.positions.insert(mesh.positions.end(), glm::vec4(attrib.vertices[3 * i + 0], attrib.vertices[3 * i + 1], attrib.vertices[3 * i + 2], 1.0f));
			vertex_count++;
		}
		model_vertex_count = vertex_count;

		int index_count = 0;
		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				mesh.indices.insert(mesh.indices.end(), index.vertex_index);
				index_count++;
			}
		}
		model_index_count = index_count;
	}
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
	attraction_points.resize(max_attractors); // Resize the vector to the maximum number of attractors.

	// For N-Body Simulation
	particle_positions[0].resize(max_particle_count);
	particle_positions[1].resize(max_particle_count);
	particle_velocities.resize(max_particle_count);
	
	std::vector<glm::vec3> particle_colors(max_particle_count);
	for (int i = 0; i < max_particle_count; i++) {
		particle_colors[i] = glm::rgbColor(glm::vec3(static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 360.0f, 1.0f, 1.0f));
	}

	// Initializes the particle buffers (N-Body Simulation).
	glCreateBuffers(2, particle_positions_buffer);
	glCreateBuffers(1, &particle_velocities_buffer);
	glCreateBuffers(1, &particle_colors_buffer);
	glNamedBufferStorage(particle_positions_buffer[0], sizeof(float) * 4 * max_particle_count, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glNamedBufferStorage(particle_positions_buffer[1], sizeof(float) * 4 * max_particle_count, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glNamedBufferStorage(particle_velocities_buffer, sizeof(float) * 4 * max_particle_count, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glNamedBufferStorage(particle_colors_buffer, sizeof(float) * 3 * max_particle_count, particle_colors.data(), 0); // We can already upload the colors as they will not be changed.

	// Setup VAOs for rendering particles. (N-Body Simulation)
	glCreateVertexArrays(2, particle_vao);

	glVertexArrayVertexBuffer(particle_vao[0], 0, particle_positions_buffer[0], 0, 4 * sizeof(float));
	glVertexArrayVertexBuffer(particle_vao[0], 1, particle_colors_buffer, 0, 4 * sizeof(float));

	glEnableVertexArrayAttrib(particle_vao[0], 0);
	glVertexArrayAttribFormat(particle_vao[0], 0, 4, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(particle_vao[0], 0, 0);

	glEnableVertexArrayAttrib(particle_vao[0], 1);
	glVertexArrayAttribFormat(particle_vao[0], 1, 4, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(particle_vao[0], 1, 1);

	glVertexArrayVertexBuffer(particle_vao[1], 0, particle_positions_buffer[0], 0, 4 * sizeof(float));
	glVertexArrayVertexBuffer(particle_vao[1], 1, particle_colors_buffer, 0, 4 * sizeof(float));

	glEnableVertexArrayAttrib(particle_vao[1], 0);
	glVertexArrayAttribFormat(particle_vao[1], 0, 4, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(particle_vao[1], 0, 0);

	glEnableVertexArrayAttrib(particle_vao[1], 1);
	glVertexArrayAttribFormat(particle_vao[1], 1, 4, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(particle_vao[1], 1, 1);
	// End For N-Body Simulation

	// Initializes the particle buffer.
	glGenBuffers(1, &particle_buffer);
	glGenBuffers(1, &mesh_position_buffer);
	glGenBuffers(1, &mesh_index_buffer);

	reset_particles();
	update_model();
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
	else if (display_mode == DISPLAY_SINGLE_ATTRACTOR_SCENE || display_mode == DISPLAY_MULTI_ATTRACTOR_SCENE) {

		std::uniform_real_distribution<> vel_dist(-10.0f, 10.f);  // Random position value

		// Zeroes the particle data.
		for (int i = 0; i < current_particle_count; i++) {
			// Initializes the particle position.
			particles[i].position = glm::vec4(random_inside_sphere(50.0f), 1.0f);
			particles[i].velocity = glm::vec3(vel_dist(gen), vel_dist(gen), vel_dist(gen));
			particles[i].lifetime = real_dist(gen);
			particles[i].remaining = particles[i].lifetime;
		}
	}
	else if (display_mode == DISPLAY_NBODY_SCENE) {
		// Zeroes the particle data.
		for (int i = 0; i < current_particle_count; i++) {
			// Initializes the particle position.
			const float alpha = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f * static_cast<float>(M_PI);
			const float beta = asinf(static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
			glm::vec3 point_on_sphere = glm::vec3(cosf(alpha) * cosf(beta), sinf(alpha) * cosf(beta), sinf(beta));
			
			//glm::vec3 point_on_sphere = random_inside_sphere(10.0f);
			
			particle_positions[0][i] = glm::vec4(point_on_sphere, 1.0f);
			particle_positions[1][i] = glm::vec4(point_on_sphere, 1.0f);
			particle_velocities[i] = glm::vec4(0.0f);
		}
	}
	else if (display_mode == DISPLAY_PARTICLE_SURFACE_ESTIMATOR_SCENE) {

		// Zeroes the particle data.
		for (int i = 0; i < current_particle_count; i++) {
			// Initializes the particle position.
			particles[i].position = glm::vec4(random_inside_sphere(25.0f), 1.0f);;
			particles[i].velocity = glm::vec3(0.0f);
		}
	}

	// Updates the particle buffer.
	update_particles_buffer();
}

glm::vec3 Application::random_inside_sphere(float radius) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
	std::uniform_real_distribution<float> radius_dist(0.0f, 1.0f);

	// Generate a random point in the unit cube
	glm::vec3 point(dist(gen), dist(gen), dist(gen));

	// Normalize the point to get a random direction on the unit sphere
	point = glm::normalize(point);

	// Scale by the cube root of a random radius to ensure uniform distribution
	float r = std::cbrt(radius_dist(gen));
	return point * (r * radius);
}

// Update Particles Buffer
void Application::update_particles_buffer() {
	current_particle_count = desired_particle_count;

	std::cout << "---" << std::endl;
	std::cout << "Desired particle count: " << current_particle_count << std::endl;

	if (display_mode == DISPLAY_NBODY_SCENE) 
	{
		glNamedBufferSubData(particle_positions_buffer[0], 0, sizeof(glm::vec4) * current_particle_count, particle_positions[0].data());
		glNamedBufferSubData(particle_positions_buffer[1], 0, sizeof(glm::vec4) * current_particle_count, particle_positions[1].data());
		glNamedBufferSubData(particle_velocities_buffer, 0, sizeof(glm::vec4) * current_particle_count, particle_velocities.data());

		std::cout << "Particles buffer size: " << sizeof(float) * (4 + 4 + 3) * current_particle_count << " bytes." << std::endl;
	}
	else 
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, particle_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Particle) * current_particle_count, particles.data(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		std::cout << "Particles buffer size: " << sizeof(Particle) * current_particle_count << " bytes." << std::endl;
	}

	std::cout << "Particles buffer updated." << std::endl;
}

void Application::update_model() {
	from_file(lecture_folder_path / MODEL_PATHS[current_model], false);

	size_t position_size = sizeof(glm::vec4) * mesh.positions.size();
	size_t index_size = sizeof(int) * mesh.indices.size();

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, mesh_position_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, position_size, mesh.positions.data(), GL_DYNAMIC_READ);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, mesh_index_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, index_size, mesh.indices.data(), GL_DYNAMIC_READ);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	std::cout << "Model Vertex Count: " << model_vertex_count << std::endl;
	std::cout << "Model Index Count: " << model_index_count << std::endl;
	std::cout << "Model Buffer Size: " << position_size + index_size << " bytes." << std::endl;
	std::cout << "Model Updated." << std::endl;
}

// Update Particles on GPU
void Application::update_particles_gpu()
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particle_positions_buffer[current_read]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, particle_positions_buffer[current_write]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, particle_velocities_buffer);

	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	nbody_update_program.use();
	nbody_update_program.uniform("t_delta", (float)t_delta * 0.0001f);
	nbody_update_program.uniform("current_particle_count", current_particle_count);
	nbody_update_program.uniform("acceleration_factor", acceleration_factor);
	nbody_update_program.uniform("distance_threshold", distance_threshold);

	// Dispatches the compute shader and measures the elapsed time.
	glBeginQuery(GL_TIME_ELAPSED, render_time_query);
	glDispatchCompute(current_particle_count / local_size_x, 1, 1);
	glEndQuery(GL_TIME_ELAPSED);

	// Waits for OpenGL - don't forget OpenGL is asynchronous.
	glFinish();

	// Evaluates the query.
	GLuint64 render_time;
	glGetQueryObjectui64v(render_time_query, GL_QUERY_RESULT, &render_time);
	compute_fps_gpu = 1000.f / (static_cast<float>(render_time) * 1e-6f);
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

	if (display_mode == DISPLAY_NBODY_SCENE) {
		update_particles_gpu();
	}
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

// Attracting Simulation (DISPLAY_SINGLE_ATTRACTOR_SCENE)
void Application::render_attracting_simulation() {

	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	attracting_particle_program.use();
	attracting_particle_program.uniform("t_time", (float)elapsed_time);
	attracting_particle_program.uniform("t_delta", (float)t_delta * 0.0001f);
	attracting_particle_program.uniform("particle_size_vs", particle_size);
	attracting_particle_program.uniform("attractor_point", attraction_points[0]);
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

// Attracting Simulation (DISPLAY_MULTI_ATTRACTOR_SCENE)
void Application::render_multi_attracting_simulation() {

	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	multi_attracting_particle_program.use();
	multi_attracting_particle_program.uniform("t_time", (float)elapsed_time);
	multi_attracting_particle_program.uniform("t_delta", (float)t_delta * 0.0001f);
	multi_attracting_particle_program.uniform("particle_size_vs", particle_size);
	multi_attracting_particle_program.uniform("attractor_used", attractor_used);
	for (int i = 0; i < attractor_used; i++) {
		multi_attracting_particle_program.uniform("attractor_points[" + std::to_string(i) + "]", attraction_points[i]);
	}
	multi_attracting_particle_program.uniform("attractor_force", attraction_force);

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

// N-Body Simulation (DISPLAY_NBODY_SCENE)
void Application::render_nbody_simulation() {
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	nbody_particle_program.use();
	nbody_particle_program.uniform("particle_size_vs", particle_size);

	// Binds the particle texture.
	glBindTextureUnit(0, star_tex);

	// Binds the particle buffer.
	glBindVertexArray(particle_vao[current_write]);
	glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
	glDrawArrays(GL_POINTS, 0, current_particle_count);

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	std::swap(current_read, current_write);
}

void Application::render_surface_estimator() {
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	particle_surface_estimator_program.use();
	particle_surface_estimator_program.uniform("t_time", (float)elapsed_time);
	particle_surface_estimator_program.uniform("t_delta", (float)t_delta * 0.0001f);
	particle_surface_estimator_program.uniform("vertex_count", model_vertex_count);
	particle_surface_estimator_program.uniform("index_count", model_index_count);
	particle_surface_estimator_program.uniform("particle_size_vs", particle_size);

	// Binds the particle texture.
	glBindTextureUnit(0, star_tex);

	// Binds the particle buffer.
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, particle_buffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, mesh_position_buffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, mesh_index_buffer);

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
	else if (display_mode == DISPLAY_SINGLE_ATTRACTOR_SCENE) {
		render_attracting_simulation();
	}
	else if (display_mode == DISPLAY_MULTI_ATTRACTOR_SCENE) {
		render_multi_attracting_simulation();
	}
	else if (display_mode == DISPLAY_NBODY_SCENE) {
		render_nbody_simulation();
	}
	else if (display_mode == DISPLAY_PARTICLE_SURFACE_ESTIMATOR_SCENE) {
		render_surface_estimator();
	}

	// Resets the VAO and the program.
	glBindVertexArray(0);
	glUseProgram(0);

	// Stops measuring the elapsed time.
	glEndQuery(GL_TIME_ELAPSED);

	// Waits for OpenGL - don't forget OpenGL is asynchronous.
	glFinish();

	// Evaluates the query.
	if (display_mode != DISPLAY_NBODY_SCENE) {
		GLuint64 render_time;
		glGetQueryObjectui64v(render_time_query, GL_QUERY_RESULT, &render_time);
		fps_gpu = 1000.f / (static_cast<float>(render_time) * 1e-6f);
	}
	else {
		fps_gpu = compute_fps_gpu;
	}
}

// GUI
void Application::render_ui() {

	if (!show_ui) return;

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
		if (display_mode == DISPLAY_SINGLE_ATTRACTOR_SCENE || display_mode == DISPLAY_MULTI_ATTRACTOR_SCENE) {
			ImGui::SliderInt("Attractors Count", &attractor_used, 1, max_attractors);
			
			for (int i = 0; i < ((display_mode == DISPLAY_SINGLE_ATTRACTOR_SCENE) ? 1 : attractor_used); i++) {
				std::string label = "Attractor " + std::to_string(i);
				ImGui::InputFloat3(label.c_str(), glm::value_ptr(attraction_points[i]));
			}
			ImGui::SliderFloat("Force", &attraction_force, 0.1f, 25.0f, "%.1f");
		}
		else if (display_mode == DISPLAY_NBODY_SCENE) {
			ImGui::SliderFloat("Acceleration Factor", &acceleration_factor, 0.1f, 5.0f, "%.1f");
			ImGui::SliderFloat("Distance Threshold", &distance_threshold, 0.001f, 0.1f, "%.3f");
		}
		else if (display_mode == DISPLAY_PARTICLE_SURFACE_ESTIMATOR_SCENE) {
			if (ImGui::Combo("Model", &current_model, MODEL_NAMES, IM_ARRAYSIZE(MODEL_NAMES))) {
				update_model();
			}
		}
	}

	ImGui::End();
}

void Application::on_resize(int width, int height) {
	PV227Application::on_resize(width, height);
	resize_fullscreen_textures();
}

void Application::on_key_pressed(int key, int scancode, int action, int mods) {
	PV227Application::on_key_pressed(key, scancode, action, mods);

	if (action == GLFW_PRESS) {
		switch (key) {
		case GLFW_KEY_SPACE:
			reset_particles();
			break;
		case GLFW_KEY_H:
			show_ui = !show_ui;
			break;
		}
	}
}