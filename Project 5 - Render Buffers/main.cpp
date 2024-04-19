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

// vector to store vertices
std::vector<GLfloat> vertices;

// texture image width and height
unsigned img_width = 2048;
unsigned img_height = 2048;

// camera speeds
float rot_speed = 0.02;
float obj_zoom_speed = 0.15;
float plane_zoom_speed = 0.02;
float light_speed = 0.02;
float plane_speed = 0.01;

// rotation and zoom with mouse
float obj_t_z = -60;
float obj_r_x = -1;
float obj_r_y = 0;

float plane_t_z = -4;
float plane_r_x = 0;
float plane_r_y = 0;

// previous positions of mouse (before zoom/rotate)
int prev_y_zoom = 0;
int prev_x = 0;
int prev_y = 0;

// light rotation
float light_x = 0;
float light_y = 0;

// indicates if CTRL is pressed
bool ctrl_press = false;

// indicates if ALT is pressed
bool alt_press = false;

int mouse_button = -1;

// GL variables

// IDs
cyGLSLProgram obj_program;
cyGLSLProgram plane_program;
GLuint obj_program_id;
GLuint plane_program_id;
GLuint obj_vs_id;
GLuint obj_fs_id;
GLuint plane_vs_id;
GLuint plane_fs_id;
GLuint obj_tex_id;

// Uniforms
GLuint obj_mvp;
GLuint obj_mvn;
GLuint tex_d;
GLuint K_a;
GLuint K_d;
GLuint K_s;
GLuint plane_mvp;

// window dimensions
GLfloat display_width = 800;
GLfloat display_height = 600;

// VAOs and VBOs
GLuint obj_vao, obj_vbo[3];
GLuint plane_vao, plane_vbo;

// OBJ reader
cyTriMesh reader;

// Render Buffer
cyGLRenderTexture2D renderBuffer;

// render buffer dimensions
int rb_width = 600;
int rb_height = 600;

// find center x & y points to translate to
cyVec3f findCenter()
{

    cyVec3f max = reader.GetBoundMax();
    cyVec3f min = reader.GetBoundMin();

    float center_x = min.x + ((max.x - min.x) / 2.0f);
    float center_y = min.y + ((max.y - min.y) / 2.0f);

    return cyVec3f(center_x, center_y, obj_t_z);
}

// calculate MVP matrix
cyMatrix4f getMVP()
{
    // perspective projection matrix values
    float fov = 3.145 * 40.0 / 180.0;
    float aspect = rb_width / rb_height;
    float near_p = 0.1f;
    float far_p = obj_t_z + obj_t_z;

    // generate perspective projection, translation, and rotation matrices and multiply them
    return (cyMatrix4f::Perspective(fov, aspect, near_p, far_p) * cyMatrix4f::Translation(findCenter()) * cyMatrix4f::RotationXYZ(obj_r_x, obj_r_y, 0));
}

// calculate MVP matrix for plane
cyMatrix4f getPlaneMVP()
{
    // perspective projection matrix values
    float fov = 0.7;
    float aspect = display_width / display_height;
    float n = 0.1f;
    float f = plane_t_z * 2;

    // generate perspective projection, translation, and rotation matrices and multiply them
    return (cyMatrix4f::Perspective(fov, aspect, n, f) * cyMatrix4f::Translation(cyVec3f(0, 0, plane_t_z))) * cyMatrix4f::RotationXYZ(plane_r_x, plane_r_y, 0);
}

// calculate object MVN matrix
cyMatrix4f getMVN()
{
    // generate translation and rotation matrices then multiply them
    return cyMatrix4f::Translation(findCenter()) * cyMatrix4f::RotationXYZ(obj_r_x + light_x, obj_r_y + light_y, 0);
}

// setup reader and parse OBJ file
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
void setVBO()
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
    glGenVertexArrays(1, &obj_vao);
    glBindVertexArray(obj_vao);

    glGenBuffers(3, obj_vbo);

    glBindBuffer(GL_ARRAY_BUFFER, obj_vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, num_f * 3 * sizeof(cyVec3f), v, GL_STATIC_DRAW);
    GLuint obj_pos = glGetAttribLocation(obj_program_id, "pos");
    glEnableVertexAttribArray(obj_pos);
    glVertexAttribPointer(obj_pos, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);

    glBindBuffer(GL_ARRAY_BUFFER, obj_vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, num_f * 3 * sizeof(cyVec3f), n, GL_STATIC_DRAW);
    GLuint obj_norm = glGetAttribLocation(obj_program_id, "norm");
    glEnableVertexAttribArray(obj_norm);
    glVertexAttribPointer(obj_norm, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);

    glBindBuffer(GL_ARRAY_BUFFER, obj_vbo[2]);
    glBufferData(GL_ARRAY_BUFFER, num_f * 3 * sizeof(cyVec3f), t, GL_STATIC_DRAW);
    GLuint obj_txc = glGetAttribLocation(obj_program_id, "txc");
    glEnableVertexAttribArray(obj_txc);
    glVertexAttribPointer(obj_txc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);
}

// generate VBO for plane
void setPlaneVBO()
{
    // quad
    static const GLfloat plane[] = {
        -1,
        -1,
        0,
        1,
        -1,
        0,
        -1,
        1,
        0,
        -1,
        1,
        0,
        1,
        -1,
        0,
        1,
        1,
        0,
    };

    glGenVertexArrays(1, &plane_vao);
    glBindVertexArray(plane_vao);
    glGenBuffers(1, &plane_vbo);

    glBindBuffer(GL_ARRAY_BUFFER, plane_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane), plane, GL_STATIC_DRAW);
    GLuint plane_pos = glGetAttribLocation(plane_program_id, "pos");
    glVertexAttribPointer(plane_pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(plane_pos);
}

// read and load diffuse texture to texture unit 0
void setupTexture()
{
    std::string file(reader.M(0).map_Kd);
    std::vector<unsigned char> image;
    unsigned error = lodepng::decode(image, img_width, img_height, file);
    if (error)
        std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;

    glGenTextures(1, &obj_tex_id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, obj_tex_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_width, img_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &image[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
}

// build render buffer
void setRenderBuffer()
{
    // Initialize Render Buffer
    renderBuffer.Initialize(true, 3, rb_width, rb_height);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        fprintf(stderr, "Error: render buffer initialization failed");
        exit(1);
    }
    renderBuffer.BuildTextureMipmaps();

    // bilinear filtering for magnification
    renderBuffer.SetTextureFilteringMode(GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);

    // mip-mapping with anisotropic filtering for minification
    renderBuffer.SetTextureMaxAnisotropy();
}

// read, compile and link shaders to program
void compileShaders()
{
    cyGLSLShader shader;

    // create program
    obj_program_id = glCreateProgram();

    // read, compile and attach shader files
    if (!(shader.CompileFile("obj_shader.vert", GL_VERTEX_SHADER)))
    {
        fprintf(stderr, "Error: object vertex shader compilation failed");
        exit(1);
    }

    obj_vs_id = shader.GetID();
    glAttachShader(obj_program_id, obj_vs_id);

    if (!(shader.CompileFile("obj_shader.frag", GL_FRAGMENT_SHADER)))
    {
        fprintf(stderr, "Error: object fragment shader compilation failed");
        exit(1);
    }

    obj_fs_id = shader.GetID();
    glAttachShader(obj_program_id, obj_fs_id);

    // obj program
    glLinkProgram(obj_program_id);
    glUseProgram(obj_program_id);

    // set uniform variables location
    obj_mvp = glGetUniformLocation(obj_program_id, "mvp");
    obj_mvn = glGetUniformLocation(obj_program_id, "mvn");
    K_a = glGetUniformLocation(obj_program_id, "K_a");
    K_d = glGetUniformLocation(obj_program_id, "K_d");
    K_s = glGetUniformLocation(obj_program_id, "K_s");
    tex_d = glGetUniformLocation(obj_program_id, "tex_d");

    // create program
    plane_program_id = glCreateProgram();

    // read, compile and attach shader files
    if (!(shader.CompileFile("plane_shader.vert", GL_VERTEX_SHADER)))
    {
        fprintf(stderr, "Error: plane vertex shader compilation failed");
        exit(1);
    }

    plane_vs_id = shader.GetID();
    glAttachShader(plane_program_id, plane_vs_id);

    if (!(shader.CompileFile("plane_shader.frag", GL_FRAGMENT_SHADER)))
    {
        fprintf(stderr, "Error: plane fragment shader compilation failed");
        exit(1);
    }

    plane_fs_id = shader.GetID();
    glAttachShader(plane_program_id, plane_fs_id);

    // plane program
    glLinkProgram(plane_program_id);
    glUseProgram(plane_program_id);

    // set uniform variables location
    plane_mvp = glGetUniformLocation(plane_program_id, "p_mvp");
}

// detach shaders from program and delete them
void removeShaders()
{
    glDetachShader(obj_program_id, obj_vs_id);
    glDetachShader(obj_program_id, obj_vs_id);
    glDeleteShader(obj_vs_id);
    glDeleteShader(obj_fs_id);

    glDetachShader(plane_program_id, plane_vs_id);
    glDetachShader(plane_program_id, plane_vs_id);
    glDeleteShader(plane_vs_id);
    glDeleteShader(plane_fs_id);
}

// change viewport and preserve object size when resizing window
void resize(int w, int h)
{
    display_width = w;
    display_height = h;
    glViewport(0, 0, w, h);
}

// display function
void draw()
{
    // obj render to texture
    glUseProgram(obj_program_id);
    renderBuffer.Bind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // calculate and set mvp/mvn matrix
    float matrix[16];
    float _mvn[16];
    getMVP().Get(matrix);
    getMVN().Get(_mvn);
    glUniformMatrix4fv(obj_mvp, 1, false, matrix);
    glUniformMatrix4fv(obj_mvn, 1, false, _mvn);

    glBindVertexArray(obj_vao);

    glUniform1i(tex_d, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, obj_tex_id);

    glUniform3f(K_a, reader.M(0).Ka[0], reader.M(0).Ka[1], reader.M(0).Ka[2]);
    glUniform3f(K_d, reader.M(0).Kd[0], reader.M(0).Kd[1], reader.M(0).Kd[2]);
    glUniform3f(K_s, reader.M(0).Ks[0], reader.M(0).Ks[1], reader.M(0).Ks[2]);

    // draw obj
    glDrawArrays(GL_TRIANGLES, 0, num_f * 3 * sizeof(cyVec3f));

    renderBuffer.Unbind();

    // render buffer texture on plane
    glUseProgram(plane_program_id);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float _p_mvp[16];
    getPlaneMVP().Get(_p_mvp);
    glUniformMatrix4fv(plane_mvp, 1, false, _p_mvp);

    GLuint rb_tex = glGetUniformLocation(plane_program_id, "render_tex");
    glUniform1i(rb_tex, 0);
    renderBuffer.BindTexture(0);

    glBindVertexArray(plane_vao);

    glDrawArrays(GL_TRIANGLES, 0, 6);

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

        if (glutGetModifiers() == GLUT_ACTIVE_ALT)
            alt_press = true;
        else
            alt_press = false;

        mouse_button = button;
    }
    else
        mouse_button = -1;
}

// mouse motion listener
void mouseMotion(int x, int y)
{
    if (!ctrl_press && !alt_press)
    {
        // left click - start rotation
        if (mouse_button == GLUT_LEFT_BUTTON)
        {
            if (prev_y < y)           // cursor moves up
                obj_r_x += rot_speed; // rotate along x-axis
            else if (prev_y > y)      // cursor moves down
                obj_r_x -= rot_speed; // rotate along x-axis
            prev_y = y;

            if (prev_x < x)           // cursor moves right
                obj_r_y += rot_speed; // rotate along y-axis
            else if (prev_x > x)      // cursor moves left
                obj_r_y -= rot_speed; // rotate along y-axis
            prev_x = x;
        }
        // right click - start zooming
        else if (mouse_button == GLUT_RIGHT_BUTTON)
        {
            if (prev_y_zoom < y)
                obj_t_z += obj_zoom_speed; // zoom in
            else
                obj_t_z -= obj_zoom_speed; // zoom out
            prev_y_zoom = y;
        }
    }
    // ctrl click and drag - rotate light
    else
    {
        if (ctrl_press)
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

        if (alt_press)
        {
            if (mouse_button == GLUT_LEFT_BUTTON)
            {
                if (prev_y < y)               // cursor moves up
                    plane_r_x += plane_speed; // rotate along x-axis
                else if (prev_y > y)          // cursor moves down
                    plane_r_x -= plane_speed; // rotate along x-axis
                prev_y = y;

                if (prev_x < x)               // cursor moves right
                    plane_r_y += plane_speed; // rotate along y-axis
                else if (prev_x > x)          // cursor moves left
                    plane_r_y -= plane_speed; // rotate along y-axis
                prev_x = x;
            }
            // right click - start zooming
            else if (mouse_button == GLUT_RIGHT_BUTTON)
            {
                if (prev_y_zoom < y)
                    plane_t_z += plane_zoom_speed; // zoom in
                else
                    plane_t_z -= plane_zoom_speed; // zoom out
                prev_y_zoom = y;
            }
        }
    }
    glutPostRedisplay();
}

int main(int argc, char *argv[])
{
    // initialize GLUT
    glutInit(&argc, argv);

    // initialize window
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitContextFlags(GLUT_DEBUG);
    glutInitWindowSize(display_width, display_height);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Project 5");

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

    // read and parse OBJ
    reader = parseOBJ(argc, argv[1]);

    compileShaders();
    setVBO();
    setPlaneVBO();
    setupTexture();
    setRenderBuffer();

    // start
    glutMainLoop();

    // clear obj_vbo buffers
    glDeleteBuffers(1, &obj_vbo[0]);
    glDeleteBuffers(1, &obj_vbo[1]);
    glDeleteBuffers(1, &obj_vbo[2]);
    glDeleteBuffers(1, &plane_vbo);

    return 0;
}
