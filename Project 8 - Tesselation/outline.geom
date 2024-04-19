#version 410 core

layout(triangles) in;
layout(line_strip, max_vertices = 6) out;

void main()
{
    // Loop through the input vertices and generate line segments for each edge
    for (int i = 0; i < 3; i++)
    {
        // Calculate the indices of the adjacent vertices
        int j = (i + 1) % 3;

        // Output the first endpoint of the line segment
        gl_Position = gl_in[i].gl_Position;
        EmitVertex();

        // Output the second endpoint of the line segment
        gl_Position = gl_in[j].gl_Position;
        EmitVertex();
    }

    // End the line strip
    EndPrimitive();
}
