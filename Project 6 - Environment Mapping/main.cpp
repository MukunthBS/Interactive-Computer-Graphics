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

// number of vertices and faces in given obj
int obj_num_v;
int obj_num_f;

// number of vertices and faces in cube
int cube_num_v;
int cube_num_f;

// vector to store vertices
std::vector<GLfloat> vertices;

// texture image width and height
unsigned img_width = 2048;
unsigned img_height = 2048;

// camera speeds
float rot_speed = 0.02;
float obj_zoom_speed = 0.15;

// rotation and zoom with mouse
float obj_t_z = -100;
float obj_r_x = 0;
float obj_r_y = 0;

// previous positions of mouse (before zoom/rotate)
int prev_y_zoom = 0;
int prev_x = 0;
int prev_y = 0;

int mouse_button = -1;

// GL variables

// IDs
cyGLSLProgram obj_program;
cyGLSLProgram ref_program;
cyGLSLProgram plane_program;
cyGLSLProgram cube_program;
GLuint obj_program_id;
GLuint ref_program_id;
GLuint plane_program_id;
GLuint cube_program_id;
GLuint obj_vs_id;
GLuint obj_fs_id;
GLuint ref_vs_id;
GLuint ref_fs_id;
GLuint plane_vs_id;
GLuint plane_fs_id;
GLuint cube_vs_id;
GLuint cube_fs_id;
GLuint plane_tex_id;

cyTriMesh cube_mesh;
cyGLTextureCubeMap envmap;

// Uniforms
GLuint obj_mvp;
GLuint obj_mv;
GLuint ref_mvp;
GLuint ref_mv;
GLuint cube_mvp;
GLuint plane_mvp;
GLuint plane_mv;

// window dimensions
GLfloat display_width = 800;
GLfloat display_height = 600;

// VAOs and VBOs
GLuint obj_vao, obj_vbo[2];
GLuint ref_vao, ref_vbo[2];
GLuint plane_vao, plane_vbo;
GLuint cube_vao, cube_vbo;

// OBJ reader
cyTriMesh reader;

// Render Buffer
cyGLRenderTexture2D renderBuffer;

// find center x & y points to translate to
cyVec3f findCenter(float depth)
{

    cyVec3f max = reader.GetBoundMax();
    cyVec3f min = reader.GetBoundMin();

    float center_x = min.x + ((max.x - min.x) / 2.0f);
    float center_y = min.y + ((max.y - min.y) / 2.0f);

    return cyVec3f(center_x, center_y, depth);
}

// calculate MV matrix
cyMatrix4f getRefMV()
{
    // generate translation and rotation matrices then multiply them
    return cyMatrix4f::Translation(findCenter(obj_t_z)) * cyMatrix4f::RotationXYZ(3.145 + obj_r_x, obj_r_y, 0);
}

// calculate MVP matrix
cyMatrix4f getRefMVP()
{
    // perspective projection matrix values
    float fov = 3.145 * 40.0 / 180.0;
    float aspect = 1;
    float near_p = 0.1f;
    float far_p = obj_t_z * 2;

    // generate perspective projection, translation, and rotation matrices and multiply them
    return (cyMatrix4f::Perspective(fov, aspect, near_p, far_p) * cyMatrix4f::Translation(findCenter(obj_t_z)) * cyMatrix4f::RotationXYZ(3.145 + obj_r_x, obj_r_y, 0));
}

// calculate MV matrix
cyMatrix4f getMV()
{
    // generate translation and rotation matrices then multiply them
    return cyMatrix4f::Translation(findCenter(obj_t_z)) * cyMatrix4f::RotationXYZ(obj_r_x, obj_r_y, 0);
}

// calculate MVP matrix
cyMatrix4f getMVP()
{
    // perspective projection matrix values
    float fov = 3.145 * 40.0 / 180.0;
    float aspect = display_width / display_height;
    float near_p = 0.1f;
    float far_p = obj_t_z * 2;

    // generate perspective projection, translation, and rotation matrices and multiply them
    return (cyMatrix4f::Perspective(fov, aspect, near_p, far_p) * cyMatrix4f::Translation(findCenter(obj_t_z)) * cyMatrix4f::RotationXYZ(obj_r_x, obj_r_y, 0));
}

// calculate MV matrix for plane
cyMatrix4f getPlaneMV()
{
    // generate translation, and rotation matrices and multiply them
    return (cyMatrix4f::Translation(cyVec3f(0, 0, -5)) * cyMatrix4f::RotationXYZ(obj_r_x, obj_r_y, 0));
}

// calculate MVP matrix for plane
cyMatrix4f getPlaneMVP()
{
    // perspective projection matrix values
    float fov = 3.145 * 20.0 / 180.0;
    float aspect = display_width / display_height;
    float n = 0.1f;
    float f = obj_t_z * 2;

    // generate perspective projection, translation, and rotation matrices and multiply them
    return (cyMatrix4f::Perspective(fov, aspect, n, f) * cyMatrix4f::Translation(cyVec3f(0, 0, -5)) * cyMatrix4f::RotationXYZ(obj_r_x, obj_r_y, 0));
}

// calculate MVP matrix for cube
cyMatrix4f getCubeMVP()
{
    // perspective projection matrix values
    float fov = 3.145 * 40.0 / 180.0;
    float aspect = display_width / display_height;
    float n = 0.1f;
    float f = obj_t_z + obj_t_z;

    // generate perspective projection, translation, and rotation matrices and multiply them
    return (cyMatrix4f::Perspective(fov, aspect, n, f) * cyMatrix4f::Translation(cyVec3f(0, 0, 0)) * cyMatrix4f::RotationXYZ(obj_r_x, obj_r_y, 0));
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
    obj_num_v = reader.NV();
    obj_num_f = reader.NF();

    // fill vector with vertex data
    int i;
    for (i = 0; i < obj_num_v; i++)
    {
        cyVec3f _verts = reader.V(i);
        vertices.push_back(_verts[0]);
        vertices.push_back(_verts[1]);
        vertices.push_back(_verts[2]);
    }

    // fill vector with vertex data
    cyVec3f *v = new cyVec3f[obj_num_f * 3];
    for (i = 0; i < obj_num_f; i++)
    {
        cyTriMesh::TriFace _face = reader.F(i);
        v[3 * i + 0] = reader.V(_face.v[0]);
        v[3 * i + 1] = reader.V(_face.v[1]);
        v[3 * i + 2] = reader.V(_face.v[2]);
    }

    // fill vector with normal data
    cyVec3f *n = new cyVec3f[obj_num_f * 3];
    for (i = 0; i < obj_num_f; i++)
    {
        cyTriMesh::TriFace fn = reader.FN(i);
        n[3 * i + 0] = reader.VN(fn.v[0]);
        n[3 * i + 1] = reader.VN(fn.v[1]);
        n[3 * i + 2] = reader.VN(fn.v[2]);
    }

    // generate and bind vertex buffer objects and vertex array object
    glGenVertexArrays(1, &obj_vao);
    glBindVertexArray(obj_vao);

    glGenBuffers(3, obj_vbo);

    glBindBuffer(GL_ARRAY_BUFFER, obj_vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, obj_num_f * 3 * sizeof(cyVec3f), v, GL_STATIC_DRAW);
    GLuint obj_pos = glGetAttribLocation(obj_program_id, "pos");
    glEnableVertexAttribArray(obj_pos);
    glVertexAttribPointer(obj_pos, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);

    glBindBuffer(GL_ARRAY_BUFFER, obj_vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, obj_num_f * 3 * sizeof(cyVec3f), n, GL_STATIC_DRAW);
    GLuint obj_norm = glGetAttribLocation(obj_program_id, "norm");
    glEnableVertexAttribArray(obj_norm);
    glVertexAttribPointer(obj_norm, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);

    // generate and bind vertex buffer objects and vertex array object
    glGenVertexArrays(1, &ref_vao);
    glBindVertexArray(ref_vao);

    glGenBuffers(3, ref_vbo);

    glBindBuffer(GL_ARRAY_BUFFER, ref_vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, obj_num_f * 3 * sizeof(cyVec3f), v, GL_STATIC_DRAW);
    GLuint ref_pos = glGetAttribLocation(ref_program_id, "pos");
    glEnableVertexAttribArray(ref_pos);
    glVertexAttribPointer(ref_pos, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);

    glBindBuffer(GL_ARRAY_BUFFER, ref_vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, obj_num_f * 3 * sizeof(cyVec3f), n, GL_STATIC_DRAW);
    GLuint ref_norm = glGetAttribLocation(ref_program_id, "norm");
    glEnableVertexAttribArray(ref_norm);
    glVertexAttribPointer(ref_norm, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);
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

void setCubeVBO()
{
    bool r;
    if (!(r = cube_mesh.LoadFromFileObj("cube.obj")))
    {
        fprintf(stderr, "Error: cannot read cube file");
        exit(1);
    }
    cube_num_v = cube_mesh.NV();
    cube_num_f = cube_mesh.NF();

    cyVec3f *v = new cyVec3f[cube_num_f * 3];
    for (int i = 0; i < cube_num_f; i++)
    {
        cyTriMesh::TriFace f = cube_mesh.F(i);
        v[3 * i + 0] = cube_mesh.V(f.v[0]);
        v[3 * i + 1] = cube_mesh.V(f.v[1]);
        v[3 * i + 2] = cube_mesh.V(f.v[2]);
    }

    // create and bind vertex buffer object with vertex data
    glGenVertexArrays(1, &cube_vao);
    glBindVertexArray(cube_vao);
    glGenBuffers(1, &cube_vbo);

    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
    glBufferData(GL_ARRAY_BUFFER, cube_num_f * 3 * sizeof(cyVec3f), v, GL_STATIC_DRAW);
    GLuint pos = glGetAttribLocation(cube_program_id, "pos");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);
}

void setCubeMap()
{
    envmap.Initialize();
    const char *files[6] = {"cubemap_posx.png",
                            "cubemap_negx.png",
                            "cubemap_posy.png",
                            "cubemap_negy.png",
                            "cubemap_posz.png",
                            "cubemap_negz.png"};

    for (int i = 0; i < 6; i++)
    {
        std::string file(files[i]);
        std::vector<unsigned char> image;
        unsigned error = lodepng::decode(image, img_width, img_height, file);
        if (error)
            std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;

        GLubyte *_image = new GLubyte[img_width * img_width * 4]; // allocate memory
        for (int i = 0; i < image.size(); i++)
        {
            _image[i] = image[i];
        }

        // set image data
        envmap.SetImageRGBA((cyGLTextureCubeMapSide)i, _image, img_width, img_width);

        delete[] _image; // free memory
    }
    envmap.BuildMipmaps();
    envmap.SetSeamless();
    envmap.Bind(0);
}

// build render buffer
void setRenderBuffer()
{
    // Initialize Render Buffer
    renderBuffer.Initialize(true, 3, display_width, display_height);
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
    obj_mv = glGetUniformLocation(obj_program_id, "mv");

    // create program
    ref_program_id = glCreateProgram();

    // read, compile and attach shader files
    if (!(shader.CompileFile("ref_shader.vert", GL_VERTEX_SHADER)))
    {
        fprintf(stderr, "Error: reflection vertex shader compilation failed");
        exit(1);
    }

    ref_vs_id = shader.GetID();
    glAttachShader(ref_program_id, ref_vs_id);

    if (!(shader.CompileFile("ref_shader.frag", GL_FRAGMENT_SHADER)))
    {
        fprintf(stderr, "Error: reflection fragment shader compilation failed");
        exit(1);
    }

    ref_fs_id = shader.GetID();
    glAttachShader(ref_program_id, ref_fs_id);

    // obj program
    glLinkProgram(ref_program_id);
    glUseProgram(ref_program_id);

    // set uniform variables location
    ref_mvp = glGetUniformLocation(ref_program_id, "mvp");
    ref_mv = glGetUniformLocation(ref_program_id, "mv");

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
    plane_mvp = glGetUniformLocation(plane_program_id, "mvp");
    plane_mv = glGetUniformLocation(plane_program_id, "mv");

    // create program
    cube_program_id = glCreateProgram();

    // read, compile and attach shader files
    if (!(shader.CompileFile("cube_shader.vert", GL_VERTEX_SHADER)))
    {
        fprintf(stderr, "Error: cube vertex shader compilation failed");
        exit(1);
    }

    cube_vs_id = shader.GetID();
    glAttachShader(cube_program_id, cube_vs_id);

    if (!(shader.CompileFile("cube_shader.frag", GL_FRAGMENT_SHADER)))
    {
        fprintf(stderr, "Error: cube fragment shader compilation failed");
        exit(1);
    }

    cube_fs_id = shader.GetID();
    glAttachShader(cube_program_id, cube_fs_id);

    // cube program
    glLinkProgram(cube_program_id);
    glUseProgram(cube_program_id);

    // set uniform variables location
    cube_mvp = glGetUniformLocation(cube_program_id, "mvp");
}

// detach shaders from program and delete them
void removeShaders()
{
    glDetachShader(obj_program_id, obj_vs_id);
    glDetachShader(obj_program_id, obj_vs_id);
    glDeleteShader(obj_vs_id);
    glDeleteShader(obj_fs_id);

    glDetachShader(ref_program_id, ref_vs_id);
    glDetachShader(ref_program_id, ref_vs_id);
    glDeleteShader(ref_vs_id);
    glDeleteShader(ref_fs_id);

    glDetachShader(plane_program_id, plane_vs_id);
    glDetachShader(plane_program_id, plane_vs_id);
    glDeleteShader(plane_vs_id);
    glDeleteShader(plane_fs_id);

    glDetachShader(cube_program_id, cube_vs_id);
    glDetachShader(cube_program_id, cube_vs_id);
    glDeleteShader(cube_vs_id);
    glDeleteShader(cube_fs_id);
}

// change viewport and preserve object size when resizing window
void resize(int w, int h)
{
    display_width = w;
    display_height = h;
    glViewport(0, 0, w, h);
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
        mouse_button = button;
    }
    else
        mouse_button = -1;
}

// mouse motion listener
void mouseMotion(int x, int y)
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

    glutPostRedisplay();
}

// display function
void draw()
{
    // obj render to texture
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(cube_program_id);

    glDepthFunc(GL_ALWAYS);

    float _cmvp[16];
    getCubeMVP().Get(_cmvp);
    glUniformMatrix4fv(cube_mvp, 1, false, _cmvp);

    glBindVertexArray(cube_vao);

    // draw cubemap
    glDrawArrays(GL_TRIANGLES, 0, cube_num_f * 3);
    glDepthFunc(GL_LESS);

    // REFLECTION -----------------------------------
    renderBuffer.Bind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // obj render
    glUseProgram(ref_program_id);

    // calculate and set mvp/mv matrix
    float _rmvp[16];
    float _rmv[16];
    getRefMVP().Get(_rmvp);
    getRefMV().Get(_rmv);
    glUniformMatrix4fv(ref_mvp, 1, false, _rmvp);
    glUniformMatrix4fv(ref_mv, 1, false, _rmv);

    glBindVertexArray(ref_vao);

    // draw obj
    glDrawArrays(GL_TRIANGLES, 0, obj_num_f * 3 * sizeof(cyVec3f));

    renderBuffer.Unbind();

    // ----------------------------------------------

    // PLANE ---------------------------------------
    glClear(GL_DEPTH_BUFFER_BIT);

    glUseProgram(plane_program_id);

    float _pmvp[16];
    float _pmv[16];
    getPlaneMVP().Get(_pmvp);
    getPlaneMV().Get(_pmv);
    glUniformMatrix4fv(plane_mvp, 1, false, _pmvp);
    glUniformMatrix4fv(plane_mv, 1, false, _pmv);

    GLuint planeTexID = glGetUniformLocation(plane_program_id, "renderTex");
    glUniform1i(planeTexID, 0);
    renderBuffer.BindTexture(0);

    glBindVertexArray(plane_vao);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    // ----------------------------------------------

    // OBJECT ---------------------------------------
    glClear(GL_DEPTH_BUFFER_BIT);

    // obj render
    glUseProgram(obj_program_id);

    // calculate and set mvp/mv matrix
    float _mvp[16];
    float _mv[16];
    getMVP().Get(_mvp);
    getMV().Get(_mv);
    glUniformMatrix4fv(obj_mvp, 1, false, _mvp);
    glUniformMatrix4fv(obj_mv, 1, false, _mv);

    glBindVertexArray(obj_vao);

    // draw obj
    glDrawArrays(GL_TRIANGLES, 0, obj_num_f * 3 * sizeof(cyVec3f));

    // ----------------------------------------------

    glutSwapBuffers();
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
    glutCreateWindow("Project 6");

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
    setRenderBuffer();
    setVBO();
    setCubeVBO();
    setCubeMap();
    setPlaneVBO();

    // start
    glutMainLoop();

    // clear obj_vbo buffers
    glDeleteBuffers(1, &obj_vbo[0]);
    glDeleteBuffers(1, &obj_vbo[1]);
    glDeleteBuffers(1, &cube_vbo);
    glDeleteBuffers(1, &plane_vbo);

    glDeleteVertexArrays(1, &obj_vao);
    glDeleteVertexArrays(1, &cube_vao);
    glDeleteVertexArrays(1, &plane_vao);

    return 0;
}
