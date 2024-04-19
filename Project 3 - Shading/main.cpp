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
// number of faces in given obj
int num_f;

// vector to store vertices
std::vector<GLfloat> vertices;

// rotation and zoom with mouse
float depth = -60;
float rot_x = -1;
float rot_y = 0;

// indicates which button is pressed
// -1 for none
int mouse_button = -1;

// previous positions of mouse (before zoom/rotate)
int prev_y_zoom = 0;
int prev_x = 0;
int prev_y = 0;

// indicates if CTRL is pressed
bool ctrl_press = false;

// light rotation
float light_x = 0;
float light_y = 0;

// initial display values
GLfloat width = 800;
GLfloat height = 600;
float aspect = width / height;
float fov = 3.145 * 40.0 / 180.0;

// GL variables
cyGLSLProgram program;
GLuint program_id;
GLuint vs_id;
GLuint fs_id;
GLuint mvp;
GLuint mvn;
GLuint vao, vbo[2];

// find center x & y points to translate to
cyVec3f findCenter()
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
cyMatrix4f getMVP()
{
    // near and far planes
    float near_p = 0.1f;
    float far_p = depth + depth;

    return (cyMatrix4f::Perspective(fov, aspect, near_p, far_p) * cyMatrix4f::Translation(findCenter()) * cyMatrix4f::RotationXYZ(rot_x, rot_y, 0));
}

// calculate mvn matrix
cyMatrix4f getMVN()
{
    return cyMatrix4f::Translation(findCenter()) * cyMatrix4f::RotationXYZ(rot_x + light_x, rot_y + light_y, 0);
}

// generate vertex buffer object and bind data
GLuint *getVBO(int argc, const char *filename)
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
    num_f = reader.NF();

    // fill vector with vertex data
    int i;
    for (i = 0; i < num_v; i++)
    {
        cyVec3f _verts = reader.V(i);
        vertices.push_back(_verts[0]);
        vertices.push_back(_verts[1]);
        vertices.push_back(_verts[2]);
    }

    // fill vector with vertex data
    cyVec3f *v = new cyVec3f[num_f * 3];
    for (i = 0; i < num_f; i++)
    {
        cyTriMesh::TriFace _face = reader.F(i);
        v[3 * i + 0] = reader.V(_face.v[0]);
        v[3 * i + 1] = reader.V(_face.v[1]);
        v[3 * i + 2] = reader.V(_face.v[2]);
    }

    // fill vector with normal data
    cyVec3f *n = new cyVec3f[num_f * 3];
    for (i = 0; i < num_f; i++)
    {
        cyTriMesh::TriFace fn = reader.FN(i);
        n[3 * i + 0] = reader.VN(fn.v[0]);
        n[3 * i + 1] = reader.VN(fn.v[1]);
        n[3 * i + 2] = reader.VN(fn.v[2]);
    }

    // generate and bind vertex buffer objects and vertex array object
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(2, vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, num_f * 3 * sizeof(cyVec3f), v, GL_STATIC_DRAW);

    GLuint pos = glGetAttribLocation(program_id, "pos");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, num_f * 3 * sizeof(cyVec3f), n, GL_STATIC_DRAW);

    GLuint norm = glGetAttribLocation(program_id, "norm");
    glEnableVertexAttribArray(norm);
    glVertexAttribPointer(norm, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);

    return vbo;
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

    // set uniform variable locations
    mvp = glGetUniformLocation(program_id, "mvp");
    mvn = glGetUniformLocation(program_id, "mvn");
}

// detach shaders from program and delete them
void removeShaders()
{
    glDetachShader(program_id, vs_id);
    glDetachShader(program_id, fs_id);
    glDeleteShader(vs_id);
    glDeleteShader(fs_id);
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
    glEnable(GL_DEPTH_TEST);
    glUseProgram(program_id);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // calculate and set mvp matrix
    float matrix[16];
    float _mvn[16];
    getMVP().Get(matrix);
    getMVN().Get(_mvn);
    glUniformMatrix4fv(mvp, 1, false, matrix);
    glUniformMatrix4fv(mvn, 1, false, _mvn);

    // draw
    glDrawArrays(GL_TRIANGLES, 0, num_f * 3 * sizeof(cyVec3f));

    // Swap buffers
    glutSwapBuffers();

    // re-display
    glutPostRedisplay();
}

// listen for 'Esc' (to quit)
void keyListener(unsigned char key, int x, int y)
{
    switch (key)
    {
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
        break;
    }
}

// mouse button press listener
void mouseInput(int button, int state, int x, int y)
{
    if (state == GLUT_DOWN)
    {
        if (glutGetModifiers() == GLUT_ACTIVE_CTRL)
            ctrl_press = true;
        else
            ctrl_press = false;
        mouse_button = button;
    }
    else
        mouse_button = -1;
}

// mouse motion listener
void mouseMotion(int x, int y)
{
    if (!ctrl_press)
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
    }
    // ctrl click and drag - rotate light
    else
    {
        if (mouse_button == GLUT_LEFT_BUTTON)
        {
            if (prev_y < y)      // cursor moves up
                light_x -= 0.02; // rotate along x-axis
            else if (prev_y > y) // cursor moves down
                light_x += 0.02; // rotate along x-axis
            prev_y = y;

            if (prev_x < x)      // cursor moves right
                light_y -= 0.02; // rotate along y-axis
            else if (prev_x > x) // cursor moves left
                light_y += 0.02; // rotate along y-axis
            prev_x = x;
        }
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
    glutCreateWindow("Project 3");

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
    compileShaders();
    GLuint *vbo = getVBO(argc, argv[1]);

    // start
    glutMainLoop();

    // clear VBO buffers
    glDeleteBuffers(1, &vbo[0]);
    glDeleteBuffers(1, &vbo[1]);

    return 0;
}