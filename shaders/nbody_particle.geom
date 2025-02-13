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

// ----------------------------------------------------------------------------
// Output Variables
// ----------------------------------------------------------------------------
out VertexData
{
	vec3 color;	       // The particle color.
	vec2 tex_coord;    // The texture coordinates for the particle.
} out_data;

// ----------------------------------------------------------------------------
// Main Method
// ----------------------------------------------------------------------------
void main()
{
	// TASK 4: Compute the color, the texture coordinate, and the final position of each vertex of the quad.
	//		   Store them into out_data.color, out_data.tex_coord, and gl_Position,	respectively. 
	//         Don't forget the EmitVertex() function. Don't forget to transform the position with the projection matrix.
	//	 Hint: The data from the vertex shader is in in_data. Don't forget it is an array. 
	//         Also, texture coordinates and offsets of each of the quad vertex is in quad_tex_coords and quad_offsets arrays, in order they should be emitted.
	for (int i = 0; i < 4; i++)
	{
		out_data.color = in_data[0].color;
		out_data.tex_coord = quad_tex_coords[i];
		gl_Position = projection * (in_data[0].position_vs + particle_size_vs * quad_offsets[i]);
		EmitVertex();
	}
	EndPrimitive();
}