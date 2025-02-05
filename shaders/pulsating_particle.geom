#version 450 core

// The input primitive specification.
layout (points) in;
// The ouput primitive specification.
layout (triangle_strip, max_vertices = 4) out;

// ----------------------------------------------------------------------------
// Local Variables
// ----------------------------------------------------------------------------
// The texture coorinates for the emitted vertices.
const vec2 quad_tex_coords[4] = vec2[4](
	vec2(0.0, 1.0),
	vec2(0.0, 0.0),
	vec2(1.0, 1.0),
	vec2(1.0, 0.0)
);
// The position offsets for the emitted vertices.
const vec4 quad_offsets[4] = vec4[4](
	vec4(-0.5, +0.5, 0.0, 0.0),
	vec4(-0.5, -0.5, 0.0, 0.0),
	vec4(+0.5, +0.5, 0.0, 0.0),
	vec4(+0.5, -0.5, 0.0, 0.0)
);

// ----------------------------------------------------------------------------
// Input Variables
// ----------------------------------------------------------------------------
in VertexData
{
	vec3 color;	       // The particle color.
	vec4 position_vs;  // The particle position in view space.
	float lifetime;    // The lifetime of the particle.
	float remaining;   // The remaining lifetime of the particle.
} in_data[1];

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

// The size of a particle in view space.
uniform float particle_size_vs;
uniform float t_time;

// ----------------------------------------------------------------------------
// Output Variables
// ----------------------------------------------------------------------------
out VertexData
{
	vec3 color;	       // The particle color.
	vec2 tex_coord;    // The texture coordinates for the particle.
	float lifetime;    // The lifetime of the particle.
	float remaining;   // The remaining lifetime of the particle.
} out_data;

mat4 rotate(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat4(
        c, -s, 0, 0,
        s,  c, 0, 0,
        0,  0, 1, 0,
        0,  0, 0, 1
    );
}

float random(float seed) {
    return fract(sin(seed) * 43758.5453);
}

float get_random_angle(int particleID, float time) {
    float initialAngle = random(float(particleID)) * 6.28318530718; // Random initial angle
    float rotationAngle = time * 0.0005f; // Continuous rotation over time
    return initialAngle + rotationAngle; // Combine initial and continuous rotation
}

// ----------------------------------------------------------------------------
// Main Method
// ----------------------------------------------------------------------------
void main()
{
	// Generate a random rotation angle for this particle
    float angle = get_random_angle(gl_PrimitiveIDIn, t_time);

    // Create a rotation matrix
    mat4 rotation = rotate(angle);

    // Compute the scaled size of the particle based on its remaining lifetime
    float scale = in_data[0].remaining / in_data[0].lifetime;

    // Emit the four vertices of the quad
    for (int i = 0; i < 4; i++)
    {
        // Apply the rotation to the vertex offset
        vec4 rotatedOffset = rotation * quad_offsets[i];

        // Scale the offset based on the particle's remaining lifetime
        vec4 finalOffset = particle_size_vs * rotatedOffset * scale;

        // Compute the final position of the vertex
        gl_Position = projection * (in_data[0].position_vs + finalOffset);

        // Pass the color and texture coordinate to the fragment shader
        out_data.color = in_data[0].color;
        out_data.tex_coord = quad_tex_coords[i];
        out_data.lifetime = in_data[0].lifetime;
        out_data.remaining = in_data[0].remaining;

        // Emit the vertex
        EmitVertex();
    }

    EndPrimitive();
}