#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <stdlib.h>
#include "cyCodeBase/cyCore.h"
#include "cyCodeBase/cyVector.h"
#include "cyCodeBase/cyMatrix.h"
#include "cyCodeBase/cyTriMesh.h"
#include "cyCodeBase/cyGL.h"
#include "lodepng.h"

// number of vertices in given obj
int num_v;
// number of faces in given obj
int num_f;
// number of materials in given obj
int num_m;

// vector to store vertices
std::vector<GLfloat> vertices;

// texture image width and height
unsigned img_width = 2048;
unsigned img_height = 2048;

// camera speeds
float rot_speed = 0.02;
float zoom_speed = 0.15;
float light_speed = 0.02;

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

// IDs
cyGLSLProgram program;
GLuint program_id;
GLuint vs_id;
GLuint fs_id;

// Uniforms
GLuint mvp;
GLuint mvn;
GLuint K_a;
GLuint K_d;
GLuint K_s;
GLuint tex_a;
GLuint tex_d;
GLuint tex_s;

// Attributes
GLuint pos;
GLuint norm;
GLuint txc;

// VAO and VBOs
GLuint vao, vbo[3];

// OBJ reader
cyTriMesh reader;

// find center x & y points to translate to
cyVec3f findCenter()
{

    cyVec3f max = reader.GetBoundMax();
    cyVec3f min = reader.GetBoundMin();

    float center_x = min.x + ((max.x - min.x) / 2.0f);
    float center_y = min.y + ((max.y - min.y) / 2.0f);

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

cyTriMesh parseOBJ(int argc, const char *filename)
{
    // handle input arguments
    if (argc != 2)
    {
        fprintf(stderr, "Error: invalid arguments, expected - 2");
        exit(1);
    }

    // read OBJ file
    bool read_status;
    cyTriMesh reader;
    if (!(read_status = reader.LoadFromFileObj(filename, true)))
    {
        fprintf(stderr, "Error: cannot read file");
        exit(1);
    }
    return reader;
}

// generate vertex buffer object and bind data
void getVBO()
{
    // read vertex data
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

    // fill vector with texture data
    cyVec3f *t = new cyVec3f[num_f * 3];
    for (int i = 0; i < num_f; i++)
    {
        cyTriMesh::TriFace ft = reader.FT(i);
        t[3 * i + 0] = reader.VT(ft.v[0]);
        t[3 * i + 1] = reader.VT(ft.v[1]);
        t[3 * i + 2] = reader.VT(ft.v[2]);
    }

    // generate and bind vertex buffer objects and vertex array object
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(3, vbo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, num_f * 3 * sizeof(cyVec3f), v, GL_STATIC_DRAW);
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, num_f * 3 * sizeof(cyVec3f), n, GL_STATIC_DRAW);
    glEnableVertexAttribArray(norm);
    glVertexAttribPointer(norm, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
    glBufferData(GL_ARRAY_BUFFER, num_f * 3 * sizeof(cyVec3f), t, GL_STATIC_DRAW);
    glEnableVertexAttribArray(txc);
    glVertexAttribPointer(txc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);
}

// bind textures
void setTextures()
{
    num_m = reader.NM();

    int offset = 0;

    for (int i = 0; i < num_m; i++)
    {
        // set K_a, K_d, K_s
        glUniform3f(K_a, reader.M(i).Ka[0], reader.M(i).Ka[1], reader.M(i).Ka[2]);
        glUniform3f(K_d, reader.M(i).Kd[0], reader.M(i).Kd[1], reader.M(i).Kd[2]);
        glUniform3f(K_s, reader.M(i).Ks[0], reader.M(i).Ks[1], reader.M(i).Ks[2]);

        // decode diffuse PNG file
        unsigned error;

        // check for no texture
        if (reader.M(i).map_Ka.data != nullptr)
        {
            // decode ambient PNG file
            std::string ambient_mtl(reader.M(i).map_Ka.data);
            std::vector<unsigned char> ambient_image;
            error = lodepng::decode(ambient_image, img_width, img_height, ambient_mtl);
            if (error)
                std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;

            // generate and bind texture
            glGenTextures(1, &tex_a);

            glActiveTexture(GL_TEXTURE0 + offset);
            glBindTexture(GL_TEXTURE_2D, tex_a);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_width, img_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &ambient_image[0]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glGenerateMipmap(GL_TEXTURE_2D);

            glUniform1i(tex_a, offset);

            offset += 1;
        }
        else
        {
            // no texture
            glUniform1i(tex_a, 255);
        }

        // check for no texture
        if (reader.M(i).map_Kd.data != nullptr)
        {
            // decode diffuse PNG file
            std::string diffuse_mtl(reader.M(i).map_Kd.data);
            std::vector<unsigned char> diffuse_image;
            error = lodepng::decode(diffuse_image, img_width, img_height, diffuse_mtl);
            if (error)
                std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;

            // generate and bind texture
            glGenTextures(1, &tex_d);

            glActiveTexture(GL_TEXTURE0 + offset);
            glBindTexture(GL_TEXTURE_2D, tex_d);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_width, img_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &diffuse_image[0]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glGenerateMipmap(GL_TEXTURE_2D);

            glUniform1i(tex_d, offset);

            offset += 1;
        }
        else
        {
            // no texture
            glUniform1i(tex_d, 255);
        }

        // check for no texture
        if (reader.M(i).map_Ks.data != nullptr)
        {
            // decode specular PNG file
            std::string specular_mtl(reader.M(i).map_Ks.data);
            std::vector<unsigned char> specular_image;
            error = lodepng::decode(specular_image, img_width, img_height, specular_mtl);
            if (error)
                std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;

            // generate and bind texture
            glGenTextures(1, &tex_s);

            glActiveTexture(GL_TEXTURE0 + offset);
            glBindTexture(GL_TEXTURE_2D, tex_s);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_width, img_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &specular_image[0]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glGenerateMipmap(GL_TEXTURE_2D);

            glUniform1i(tex_s, offset);

            offset += 1;
        }
        else
        {
            // no texture
            glUniform1i(tex_s, 255);
        }
    }

    getVBO();
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
    K_a = glGetUniformLocation(program_id, "K_a");
    K_d = glGetUniformLocation(program_id, "K_d");
    K_s = glGetUniformLocation(program_id, "K_s");
    tex_a = glGetUniformLocation(program_id, "tex_a");
    tex_d = glGetUniformLocation(program_id, "tex_d");
    tex_s = glGetUniformLocation(program_id, "tex_s");

    // set attribute locations
    pos = glGetAttribLocation(program_id, "pos");
    norm = glGetAttribLocation(program_id, "norm");
    txc = glGetAttribLocation(program_id, "txc");
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
    case 'b':
    case 'B':
        zoom_speed = 20;
        break;
    case 's':
    case 'S':
        zoom_speed = 0.15;
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
        setTextures();
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
            if (prev_y < y)         // cursor moves up
                rot_x += rot_speed; // rotate along x-axis
            else if (prev_y > y)    // cursor moves down
                rot_x -= rot_speed; // rotate along x-axis
            prev_y = y;

            if (prev_x < x)         // cursor moves right
                rot_y += rot_speed; // rotate along y-axis
            else if (prev_x > x)    // cursor moves left
                rot_y -= rot_speed; // rotate along y-axis
            prev_x = x;
        }
        // right click - start zooming
        else if (mouse_button == GLUT_RIGHT_BUTTON)
        {
            if (prev_y_zoom < y)
                depth += zoom_speed; // zoom in
            else
                depth -= zoom_speed; // zoom out
            prev_y_zoom = y;
        }
    }
    // ctrl click and drag - rotate light
    else
    {
        if (mouse_button == GLUT_LEFT_BUTTON)
        {
            if (prev_y < y)             // cursor moves up
                light_x -= light_speed; // rotate along x-axis
            else if (prev_y > y)        // cursor moves down
                light_x += light_speed; // rotate along x-axis
            prev_y = y;

            if (prev_x < x)             // cursor moves right
                light_y -= light_speed; // rotate along y-axis
            else if (prev_x > x)        // cursor moves left
                light_y += light_speed; // rotate along y-axis
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
    glutCreateWindow("Project 4");

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
    reader = parseOBJ(argc, argv[1]);
    setTextures();

    // start
    glutMainLoop();

    // clear VBO buffers
    glDeleteBuffers(1, &vbo[0]);
    glDeleteBuffers(1, &vbo[1]);
    glDeleteBuffers(1, &vbo[2]);

    return 0;
}