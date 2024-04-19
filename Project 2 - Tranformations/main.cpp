#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <stdlib.h>
#include "cyCodeBase/cyCore.h"
#include "cyCodeBase/cyVector.h"
#include "cyCodeBase/cyMatrix.h"
#include "cyCodeBase/cyTriMesh.h"
#include "cyCodeBase/cyGL.h"

// number of vertices in given obj
int num_v;

// vector to store vertices
std::vector<GLfloat> vertices;

// flag for orthogonal projection
bool ortho = false;

// rotation and zoom with mouse
float depth = -48;
float rot_x = -1;
float rot_y = 0;

// indicates which button is pressed
// -1 for none
int mouse_button = -1;

// previous positions of mouse (before zoom/rotate)
int prev_y_zoom = 0;
int prev_x = 0;
int prev_y = 0;

// initial display values
GLfloat width = 800;
GLfloat height = 600;
float aspect = width / height;
float fov = 3.145 * 40.0 / 180.0;

// GL variables
GLuint program_id;
GLuint vs_id;
GLuint fs_id;
GLuint mvp;

// find center x & y points to translate to
cyVec3f findCenter(std::vector<GLfloat> vertices)
{
    GLfloat min_x = vertices[0], max_x = vertices[0];
    GLfloat min_y = vertices[1], max_y = vertices[1];

    for (size_t i = 0; i < vertices.size(); i += 3)
    {
        GLfloat x = vertices[i];
        GLfloat y = vertices[i + 1];

        min_x = std::min(min_x, x);
        max_x = std::max(max_x, x);

        min_y = std::min(min_y, y);
        max_y = std::max(max_y, y);
    }

    GLfloat center_x = min_x + ((max_x - min_x) / 2.0f);
    GLfloat center_y = min_y + ((max_y - min_y) / 2.0f);

    return cyVec3f(center_x, center_y, depth);
}

// calculate MVP matrix
cyMatrix4f getMVP(bool ortho)
{
    // near and far planes
    float near_p = depth - depth;
    float far_p = depth + depth;

    // perspective projection matrix
    cyMatrix4f persp_m = cyMatrix4f::Perspective(fov, aspect, near_p, far_p);

    // for orthogonal projection
    if (ortho)
    {
        persp_m.Normalize();
    }

    return (persp_m * cyMatrix4f::Translation(findCenter(vertices)) * cyMatrix4f::RotationXYZ(rot_x, rot_y, 0));
}

// generate vertex buffer object and bind data
GLuint getVBO(int argc, const char *filename)
{
    // handle input arguments
    if (argc != 2)
    {
        fprintf(stderr, "Error: invalid arguments, expected - 2");
        exit(1);
    }

    // read vertex data
    bool read_status;
    cyTriMesh reader;
    if (!(read_status = reader.LoadFromFileObj(filename, false)))
    {
        fprintf(stderr, "Error: cannot read file");
        exit(1);
    }
    num_v = reader.NV();

    // fill vector with vertex data
    int i;
    for (i = 0; i < num_v; i++)
    {
        cyVec3f _verts = reader.V(i);
        vertices.push_back(_verts[0]);
        vertices.push_back(_verts[1]);
        vertices.push_back(_verts[2]);
    }

    // generate and bind vertex buffer object
    GLuint vbo_id;
    glGenBuffers(1, &vbo_id);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
    glBufferData(GL_ARRAY_BUFFER, num_v * sizeof(vertices), vertices.data(), GL_STATIC_DRAW);

    return vbo_id;
}

// read, compile and link shaders to program
void compileShaders()
{
    cyGLSLShader vert;
    cyGLSLShader frag;

    // read and compile shader files
    bool read_status;
    if (!(read_status = vert.CompileFile("shader.vert", GL_VERTEX_SHADER)))
    {
        fprintf(stderr, "Error: vertex shader compilation failed");
    }
    if (!(read_status = frag.CompileFile("shader.frag", GL_FRAGMENT_SHADER)))
    {
        fprintf(stderr, "Error: fragment shader compilation failed");
    }

    // attach shaders to program
    vs_id = vert.GetID();
    glAttachShader(program_id, vs_id);
    fs_id = frag.GetID();
    glAttachShader(program_id, fs_id);

    glLinkProgram(program_id);
    glUseProgram(program_id);
}

// detach shaders from program and delete them
void removeShaders()
{
    glDetachShader(program_id, vs_id);
    glDetachShader(program_id, fs_id);
    glDeleteShader(vs_id);
    glDeleteShader(fs_id);
}

// find and set uniform and attribute variables in shaders
void setShaderVariables()
{
    mvp = glGetUniformLocation(program_id, "mvp");

    GLuint pos = glGetAttribLocation(program_id, "pos");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);
}

// change viewport and preserve object size when resizing window
void resize(int w, int h)
{
    glViewport(0, 0, w, h);

    fov = atan(tan(fov / 2.0) * h / height) * 2.0;
    width = w;
    height = h;
    aspect = width / height;
}

// display function
void draw()
{
    // clear viewport
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(program_id);

    // calculate and set mvp matrix
    float matrix[16];
    getMVP(ortho).Get(matrix);
    glUniformMatrix4fv(mvp, 1, false, matrix);

    // draw
    glDrawArrays(GL_POINTS, 0, num_v);

    // swap buffers
    glutSwapBuffers();

    // re-display
    glutPostRedisplay();
}

// listen for 'Esc' (to quit) and 'P' or 'p' (to switch projection)
void keyListener(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 'p':
    case 'P':
        ortho = !ortho;
        break;
    case 27:
        glutLeaveMainLoop();
        break;
    }
}

// listen for 'F6' to re-compile shaders
void spclKeyListener(int key, int x, int y)
{
    switch (key)
    {
    case GLUT_KEY_F6:
        removeShaders();
        compileShaders();
        setShaderVariables();
        break;
    }
}

// mouse button press listener
void mouseInput(int button, int state, int x, int y)
{
    if (state == GLUT_DOWN)
        mouse_button = button;
    else
        mouse_button = -1;
}

// mouse motion listener
void mouseMotion(int x, int y)
{
    // left click - start rotation
    if (mouse_button == GLUT_LEFT_BUTTON)
    {
        if (prev_y < y)      // cursor moves up
            rot_x += 0.02;   // rotate along x-axis
        else if (prev_y > y) // cursor moves down
            rot_x -= 0.02;   // rotate along x-axis
        prev_y = y;

        if (prev_x < x)      // cursor moves right
            rot_y += 0.02;   // rotate along y-axis
        else if (prev_x > x) // cursor moves left
            rot_y -= 0.02;   // rotate along y-axis
        prev_x = x;
    }
    // right click - start zooming
    else if (mouse_button == GLUT_RIGHT_BUTTON)
    {
        if (prev_y_zoom < y)
            depth += 0.15; // zoom in
        else
            depth -= 0.15; // zoom out
        prev_y_zoom = y;
    }
    glutPostRedisplay();
}

// main function
int main(int argc, char *argv[])
{
    // initialize GLUT
    glutInit(&argc, argv);

    // initialize window
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(width, height);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Project 2");

    // set GLUT callbacks
    glutKeyboardFunc(keyListener);
    glutSpecialFunc(spclKeyListener);
    glutMouseFunc(mouseInput);
    glutMotionFunc(mouseMotion);
    glutReshapeFunc(resize);
    glutDisplayFunc(draw);

    // initialize GLEW
    GLenum glew_status = glewInit();
    if (glew_status != GLEW_OK)
    {
        fprintf(stderr, "GLEW error");
        exit(1);
    }

    // setup program, VBOs and shaders
    program_id = glCreateProgram();
    GLuint vbo_id = getVBO(argc, argv[1]);
    compileShaders();
    setShaderVariables();

    // start
    glutMainLoop();

    // clear VBO buffer
    glDeleteBuffers(1, &vbo_id);
    return 0;
}
