#version 450 core

// ----------------------------------------------------------------------------
// Input Variables
// ----------------------------------------------------------------------------

// The UBO with camera data.	
layout (std140, binding = 0) uniform CameraBuffer
{
	mat4 projection;		// The projection matrix.
	mat4 projection_inv;	// The inverse of the projection matrix.
	mat4 view;				// The view matrix
	mat4 view_inv;			// The inverse of the view matrix.
	mat3 view_it;			// The inverse of the transpose of the top-left part 3x3 of the view matrix
	vec3 eye_position;		// The position of the eye in world space.
};

uniform float t_time;	// Time current time.
uniform float t_delta;	// The time delta.
uniform int vertex_count; // The vertex count.
uniform int index_count; // The index count.
uniform float attractor_force = 9.81; // The attractor force.

struct Particle {
	vec4 position;	// The position of the particle.
	vec3 velocity;	// The velocity of the particle.
	float lifetime; // The lifetime of the particle.
	vec3 color;		// The color of the particle.
	float remaining; // The remaining lifetime of the particle.
};

layout (std430, binding = 3) buffer ParticleBuffer
{
	Particle particles[]; // The array with particles.
};

layout (std430, binding = 4) readonly buffer MeshPositionBuffer
{
	vec4 positions[]; // The array with positions.
};

layout (std430, binding = 5) readonly buffer MeshIndexBuffer
{
	int indices[]; // The array with indices.
};

// Function to generate a random number based on input (simple hash function)
float random(float p)
{
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

vec3 random_inside_triangle(vec3 a, vec3 b, vec3 c, float s1, float s2) {
    // Generate two random numbers using the random function
    float r1 = sqrt(random(s1));
    float r2 = random(s2);

    // Barycentric Coordinate Interpolation of the random point
    return (1.0 - r1) * a + (r1 * (1.0 - r2)) * b + (r1 * r2) * c;
}

vec3 get_random_position_on_triangle(int vertexID) {

    // Calculate the triangle index
    int triangle_idx = int(random(vertexID) * (index_count / 3));

    // Get the positions of the three vertices of the triangle
    vec3 a = positions[indices[triangle_idx * 3]].xyz;
    vec3 b = positions[indices[triangle_idx * 3 + 1]].xyz;
    vec3 c = positions[indices[triangle_idx * 3 + 2]].xyz;

    // Generate random numbers for random point inside the triangle
    float s1 = float(vertexID) + 1.0;
    float s2 = float(vertexID) + 2.0;

    // Get a random point inside the triangle
    return random_inside_triangle(a, b, c, s1, s2);
}

// ----------------------------------------------------------------------------
// Output Variables
// ----------------------------------------------------------------------------
out VertexData
{
	vec3 color;	       // The particle color.
	vec4 position_vs;  // The particle position in view space.
} out_data;


// ----------------------------------------------------------------------------
// Main Method
// ----------------------------------------------------------------------------
void main()
{
	Particle particle = particles[gl_VertexID];
	vec3 color = vec3(250 / 255.f, 202 / 255.f, 0.f);

	vec3 random_dest = get_random_position_on_triangle(gl_VertexID);

	if (length(particle.position.xyz - random_dest.xyz) > 0.05f)
	{
		vec3 dir_to_attractor = normalize(random_dest.xyz - particle.position.xyz) * attractor_force;
		
		particle.position += vec4(particle.velocity, 0) * t_delta + 0.5f * vec4(dir_to_attractor, 0) * t_delta * t_delta;
		particle.velocity += dir_to_attractor * t_delta;
	} else {
		particle.position = vec4(random_dest, 1.0f);
		particle.velocity = vec3(0);
	}

    // Set the particle's position back into the buffer
    particles[gl_VertexID] = particle;

    // Output gl_Position for the current particle
	out_data.color = color;
    out_data.position_vs = view * particle.position;
}