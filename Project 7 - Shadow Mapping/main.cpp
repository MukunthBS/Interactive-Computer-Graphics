#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <stdlib.h>
#include "cyCodeBase/cyCore.h"
#include "cyCodeBase/cyVector.h"
#include "cyCodeBase/cyMatrix.h"
#include "cyCodeBase/cyTriMesh.h"
#include "cyCodeBase/cyGL.h"

// window dimensions
GLfloat displayWidth = 800;
GLfloat displayHeight = 600;

// camera control variables
double distance = 150;
double phi = 1.2 * M_PI;
double theta = 0.35 * M_PI;

// light variables
double distance_L = 50;
double phi_L = 1.2 * M_PI;
double theta_L = 0.4 * M_PI;

// camera properties
cyVec3f camPos = cyVec3f(0, -distance, 0);
cyVec3f camTarget = cyVec3f(0, 0, 0);
cyVec3f camUp = cyVec3f(0, 1, 0);
float camFOV = 0.25 * M_PI;

// light properties
cyVec3f lightPos = cyVec3f(0, distance, 0);
cyVec3f lightTarget = cyVec3f(0, 0, 0);
cyVec3f lightUp = cyVec3f(0, 1, 0);
float lightFOV = 0.3 * M_PI;

// MVP matrices
cy::Matrix4f projMatrix = cy::Matrix4f::Perspective(camFOV, displayWidth / displayHeight, 0.1f, 1000.0f);
cy::Matrix4f viewMatrix = cy::Matrix4f::View(camPos, camTarget, camUp);
cy::Matrix4f modelMatrix = cy::Matrix4f::Identity();

// light space matrices
cy::Matrix4f lightProjMatrix = cy::Matrix4f::Perspective(lightFOV, 1.0f, 0.1f, 1000.0f);
cy::Matrix4f lightMatrix = cy::Matrix4f::View(lightPos, lightTarget, lightUp);

// callback variables (mouse)
int mouseButton = -1;
int lastX;
int lastY;
int lastZoom;
bool ctrlPressed = false;

// OBJ reader
cy::TriMesh reader;

// programs
cy::GLSLProgram program;
cy::GLSLProgram program_shadow;
cy::GLSLProgram program_hint;

// to store vertex data
std::vector<cyVec3f> positionBufferData;
std::vector<cyVec3f> normalBufferData;
std::vector<GLuint> indexBufferData;

// VAOs and VBOs
GLuint vao, vbo[2], ibo;
GLuint vao_square, vbo_square[2];
GLuint vao_hint, vbo_hint;

// shadow map
cyGLRenderDepth2D shadowMap;

// calculate position with given theta and phi
cyVec3f calculatePos(double &iTheta, double &iPhi)
{
    if (iTheta > M_PI)
        iTheta = M_PI;
    if (iTheta <= 0)
        iTheta = 0.0001;

    if (iPhi > 2 * M_PI)
        iPhi -= 2 * M_PI;
    if (iPhi <= 0)
        iPhi += 2 * M_PI;

    return cy::Vec3f(
        sin(iTheta) * sin(iPhi),
        cos(iTheta),
        sin(iTheta) * cos(iPhi));
}

// update camera matrices and send to shaders
void setCamera()
{
    camPos = distance * calculatePos(theta, phi);

    viewMatrix = cy::Matrix4f::View(camPos, camTarget, camUp);

    cy::Matrix4f vp = projMatrix * viewMatrix * modelMatrix;
    cy::Matrix4f mvp = vp * modelMatrix;

    program.SetUniformMatrix4("mvp", mvp.cell);
    program.SetUniform("camPos", camPos.x, camPos.y, camPos.z);

    program_hint.SetUniformMatrix4("vp", vp.cell);
}

// update light space matrices and send to shaders
void setLight()
{
    cyVec3f spotDir = -calculatePos(theta_L, phi_L);
    lightPos = -distance_L * spotDir;

    program.SetUniform("spotDir", spotDir.x, spotDir.y, spotDir.z);
    program.SetUniform("lightPos", lightPos.x, lightPos.y, lightPos.z);

    lightMatrix = cy::Matrix4f::View(lightPos, lightTarget, lightUp);
    cyMatrix4f mvp = lightProjMatrix * lightMatrix * modelMatrix;
    program_shadow.SetUniformMatrix4("mvp", mvp.cell);

    float shadowBias = 0.00003f;
    cyMatrix4f mShadow = cyMatrix4f::Translation(cyVec3f(0.5f, 0.5f, 0.5f - shadowBias)) * cyMatrix4f::Scale(0.5f) * mvp;

    program.SetUniformMatrix4("matrixShadow", mShadow.cell);

    cyMatrix4f hintModel = cyMatrix4f::Translation(lightPos) * cyMatrix4f::Rotation(cyVec3f(0, 1, 0), spotDir);
    program_hint.SetUniformMatrix4("m", hintModel.cell);
}

// update shader uniforms
void setUniforms()
{
    program.SetUniformMatrix4("m", modelMatrix.cell);
    cy::Matrix3f mN = modelMatrix.GetInverse().GetTranspose().GetSubMatrix3();
    program.SetUniformMatrix3("mN", mN.cell);
    program.SetUniform("lightFovRad", lightFOV);
}

// compile object, shadow, and light hint shaders
void compileShaders()
{
    // object shaders
    if (!program.BuildFiles("obj_shader.vert", "obj_shader.frag"))
        exit(1);

    // shadow shaders
    if (!program_shadow.BuildFiles("shadow_shader.vert", "shadow_shader.frag"))
        exit(1);

    // light hint shaders
    if (!program_hint.BuildFiles("hint_shader.vert", "hint_shader.frag"))
        exit(1);

    setUniforms();

    setCamera();
    setLight();
}

// mouse button press listener
void mouseInput(int button, int state, int x, int y)
{
    // mouse button press event
    if (state == GLUT_DOWN)
    {
        // CTRL key press event
        if (glutGetModifiers() == GLUT_ACTIVE_CTRL)
            ctrlPressed = true;
        else
            ctrlPressed = false;
        mouseButton = button;
    }
    else
        mouseButton = -1;
}

// mouse motion listener
void mouseMotion(int x, int y)
{
    // left mouse button - change angle
    if (mouseButton == GLUT_LEFT_BUTTON)
    {
        float rotationSpeed = 0.15;

        // CTRL key - light
        if (ctrlPressed)
        {
            if (lastY < y)
                theta_L += rotationSpeed / 180.0f * M_PI;
            if (lastY > y)
                theta_L -= rotationSpeed / 180.0f * M_PI;
            if (lastX < x)
                phi_L += rotationSpeed / 180.0f * M_PI;
            if (lastX > x)
                phi_L -= rotationSpeed / 180.0f * M_PI;
            setLight();
        }
        else
        {
            if (lastY < y)
                theta -= rotationSpeed / 180.0f * M_PI;
            if (lastY > y)
                theta += rotationSpeed / 180.0f * M_PI;
            if (lastX < x)
                phi -= rotationSpeed / 180.0f * M_PI;
            if (lastX > x)
                phi += rotationSpeed / 180.0f * M_PI;
            setCamera();
        }

        lastX = x;
        lastY = y;
    }
    // right mouse button - zoom
    else if (mouseButton == GLUT_RIGHT_BUTTON)
    {
        float zoomSpeed = 0.15;

        // CTRL key - light
        if (ctrlPressed)
        {
            if (lastZoom < y)
                distance_L += zoomSpeed;
            if (lastZoom > y)
                distance_L -= zoomSpeed;
            if (distance_L <= 10)
                distance_L = 10;
            setLight();
        }
        else
        {
            if (lastZoom < y)
                distance -= zoomSpeed;
            if (lastZoom > y)
                distance += zoomSpeed;
            if (distance <= 10)
                distance = 10;
            setCamera();
        }

        lastZoom = y;
    }

    glutPostRedisplay();
}

// listen for 'Esc' (to quit)
void keyListener(unsigned char key, int x, int y)
{
    switch (key)
    {
    // Esc key
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
    // F6 key
    case GLUT_KEY_F6:
        compileShaders();
        break;
    }
}

// change viewport and preserve object size when resizing window
void resize(int w, int h)
{
    // update display width to window dimensions
    displayWidth = w;
    displayHeight = h;

    // update projection matrix aspect ratio
    projMatrix = cy::Matrix4f::Perspective(camFOV, displayWidth / displayHeight, 0.1f, 1000.0f);
    setCamera();

    // change viewport and re-display
    glViewport(0, 0, w, h);
    glutPostRedisplay();
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
    if (!(read_status = reader.LoadFromFileObj(filename)))
    {
        fprintf(stderr, "Error: cannot read file");
        exit(1);
    }
    // calculate mesh information
    reader.ComputeBoundingBox();
    return reader;
}

// read vertex data from reader
void getVertexData()
{
    unsigned int max = cy::Max(reader.NV(), reader.NVN());
    for (int vi = 0; vi < max; vi++)
    {
        positionBufferData.push_back(vi < reader.NV() ? reader.V(vi) : reader.V(0));
        normalBufferData.push_back(vi < reader.NVN() ? reader.VN(vi) : reader.VN(0));
    }

    for (int i = 0; i < reader.NF(); i++)
    {
        for (int j = 0; j < 3; j++)
        {
            // set up triangle vertex buffer data
            unsigned int index = reader.F(i).v[j];
            unsigned int indexN = reader.FN(i).v[j];

            if (indexN == index)
            {
                indexBufferData.push_back(index);
            }
            else
            {
                bool added = false;
                for (unsigned int mi = max; mi < positionBufferData.size(); mi++)
                {
                    if (positionBufferData.at(mi) == reader.V(index) &&
                        normalBufferData.at(mi) == reader.VN(indexN))
                    {
                        added = true;
                        indexBufferData.push_back(mi);
                        break;
                    }
                }
                if (!added)
                {
                    unsigned int newIndex = positionBufferData.size();
                    positionBufferData.push_back(reader.V(index));
                    normalBufferData.push_back(reader.VN(indexN));
                    indexBufferData.push_back(newIndex);
                }
            }
        }
    }
}

// generate and set VBOs for the object
void setVBOs()
{
    // vao
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // vbo
    glGenBuffers(2, &vbo[0]);

    // shader variables
    GLuint pos = program.AttribLocation("iPos");
    GLuint posN = program.AttribLocation("iNormal");

    // position
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cyVec3f) * positionBufferData.size(), positionBufferData.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(pos);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // normal
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cyVec3f) * normalBufferData.size(), normalBufferData.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(posN, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(posN);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // ibo
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indexBufferData.size(), indexBufferData.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
}

// generate and set VBOs for the plane
void getPlaneVBOs()
{
    // vao
    glGenVertexArrays(1, &vao_square);
    glBindVertexArray(vao_square);

    // vertex data
    float squareSize = 128.0f;
    static const GLfloat squareVertexPos[] = {
        -squareSize / 2.0f,
        0.0f,
        -squareSize / 2.0f,
        squareSize / 2.0f,
        0.0f,
        -squareSize / 2.0f,
        -squareSize / 2.0f,
        0.0f,
        squareSize / 2.0f,
        squareSize / 2.0f,
        0.0f,
        squareSize / 2.0f,
    };
    static const GLfloat squareVertexNorm[] = {
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
    };

    // shader variables
    GLuint pos_square = program.AttribLocation("iPos");
    GLuint posN_square = program.AttribLocation("iNormal");

    // vbo
    glGenBuffers(2, vbo_square);

    // position
    glBindBuffer(GL_ARRAY_BUFFER, vbo_square[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertexPos), squareVertexPos, GL_STATIC_DRAW);
    glVertexAttribPointer(pos_square, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(pos_square);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // normal
    glBindBuffer(GL_ARRAY_BUFFER, vbo_square[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertexNorm), squareVertexNorm, GL_STATIC_DRAW);
    glVertexAttribPointer(posN_square, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(posN_square);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
}

// generate and set VBOs for the light hint
void getHintVBOs()
{
    // vao
    glGenVertexArrays(1, &vao_hint);
    glBindVertexArray(vao_hint);

    // vertex data
    float height_hint = 2.0f;
    float width_hint = sqrt(2.0f) * height_hint * tan(lightFOV / 2);

    static const GLfloat hintVertexPos[] = {
        0.0f,
        0.0f,
        0.0f,
        -width_hint / 2.0f,
        height_hint,
        -width_hint / 2.0f,
        width_hint / 2.0f,
        height_hint,
        -width_hint / 2.0f,
        width_hint / 2.0f,
        height_hint,
        width_hint / 2.0f,
        -width_hint / 2.0f,
        height_hint,
        width_hint / 2.0f,
        -width_hint / 2.0f,
        height_hint,
        -width_hint / 2.0f,
    };

    // shader variable
    GLuint pos_hint = program_hint.AttribLocation("iPos");

    // vbo
    glGenBuffers(1, &vbo_hint);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_hint);
    glBufferData(GL_ARRAY_BUFFER, sizeof(hintVertexPos), hintVertexPos, GL_STATIC_DRAW);
    glVertexAttribPointer(pos_hint, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(pos_hint);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
}

// initialize shadow map
void setShadowMap()
{
    // shadow map dimensions
    int shadowMapWidth = 1024;
    int shadowMapHeight = 1024;

    // initialize shadow map
    shadowMap.Initialize(true, shadowMapWidth, shadowMapHeight);
    shadowMap.SetTextureFilteringMode(GL_LINEAR, GL_LINEAR);
    shadowMap.SetTextureWrappingMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

    program.SetUniform("shadow", 0);
}

// draw function
void draw()
{
    // render shadow map
    shadowMap.Bind();
    glClear(GL_DEPTH_BUFFER_BIT);
    program_shadow.Bind();
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexBufferData.size(), GL_UNSIGNED_INT, nullptr);
    shadowMap.Unbind();

    // render scene
    program.Bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, shadowMap.GetTextureID());
    glDrawElements(GL_TRIANGLES, indexBufferData.size(), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(vao_square);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // render light hint
    program_hint.Bind();
    glBindVertexArray(vao_hint);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 6);

    // swap buffers
    glutSwapBuffers();
}

// main
int main(int argc, char *argv[])
{
    // initialize GLUT
    glutInit(&argc, argv);

    // initialize window
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitContextFlags(GLUT_DEBUG);
    glutInitWindowSize(displayWidth, displayHeight);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Project 7");

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

    // assign reader
    reader = parseOBJ(argc, argv[1]);

    // read OBJ file
    getVertexData();

    // compile shaders
    compileShaders();

    // set VBOs and VAOs
    setVBOs();
    getPlaneVBOs();
    getHintVBOs();

    // setup shadow map
    setShadowMap();

    glEnable(GL_DEPTH_TEST);

    // draw loop
    glutMainLoop();

    // clear VBOs and VAOs
    glDeleteBuffers(1, &vbo[0]);
    glDeleteBuffers(1, &vbo[1]);
    glDeleteBuffers(1, &ibo);
    glDeleteBuffers(1, &vbo_square[0]);
    glDeleteBuffers(1, &vbo_square[1]);
    glDeleteBuffers(1, &vbo_hint);
    glDeleteVertexArrays(1, &vao);
    glDeleteVertexArrays(1, &vao_hint);
    glDeleteVertexArrays(1, &vao_square);

    return 0;
}
