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

// Function to generate a random number based on input (simple hash function)
float random(float p)
{
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

vec3 random_direction(float min, float max)
{
    return vec3(
        random(gl_VertexID + 1) * (max - min) + min, // X component
        random(gl_VertexID + 2) * (max - min) + min, // Y component
        random(gl_VertexID + 3) * (max - min) + min  // Z component
    );
}

// ----------------------------------------------------------------------------
// Output Variables
// ----------------------------------------------------------------------------
out VertexData
{
	vec3 color;	       // The particle color.
	vec4 position_vs;  // The particle position in view space.
	float lifetime;    // The lifetime of the particle.
	float remaining;   // The remaining lifetime of the particle.
} out_data;


// ----------------------------------------------------------------------------
// Main Method
// ----------------------------------------------------------------------------
void main()
{
	Particle particle = particles[gl_VertexID];

	float rv = random(gl_VertexID + t_time * 0.001);
	float gv = random(gl_VertexID + t_time * 0.001 + 1);
	float bv = random(gl_VertexID + t_time * 0.001 + 2);

	vec3 color = vec3(rv, gv, bv);

    if(particle.remaining < 0) {
		vec3 rand_dir = random_direction(-1,1);
		rand_dir = normalize(rand_dir);

		// Random inside sphere
		float radius = (random(gl_VertexID + 1) * (2.5f - 1.5f) + 1.5f) * sin(t_time  * 0.0001) + (random(gl_VertexID + 3) * (7.5f - 5.5f) + 5.5f);
		particle.position = vec4(vec3(0.0f) + rand_dir * radius, 1);

		particle.velocity = rand_dir * 3;

		particle.color = color;

		particle.lifetime =  random(gl_VertexID + t_delta * 0.001) * (5 - 0.5) + 0.5;
		particle.remaining = particle.lifetime;
	}

	particle.remaining -= t_delta;

	// Update the particle's position based on its velocity
	particle.position += vec4(particle.velocity, 0) * t_delta;

    // Set the particle's position back into the buffer
    particles[gl_VertexID] = particle;

    // Output gl_Position for the current particle
	out_data.color = particle.color;
    out_data.position_vs = view * particle.position;
	out_data.lifetime = particle.lifetime;
	out_data.remaining = particle.remaining;
}