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

// window dimensions
GLfloat displayWidth = 800;
GLfloat displayHeight = 600;

// image dimensions
GLfloat imageWidth = 512;
GLfloat imageHeight = 512;

// camera control variables
double distance = 300;
double phi = 0 * M_PI;
double theta = 0.45 * M_PI;

// light variables
double distance_L = 85;
double phi_L = 0 * M_PI;
double theta_L = 0.45 * M_PI;

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

// callback variables
int mouseButton = -1;
int lastX;
int lastY;
int lastZoom;
bool ctrlPressed = false;
bool renderOutline = false;

// normal map and displacement map
cyGLTexture2D normalMap;
bool hasDisp = false;
cyGLTexture2D displacementMap;

// programs
cy::GLSLProgram program;
cy::GLSLProgram program_outline;
cy::GLSLProgram program_shadow;
cy::GLSLProgram program_hint;

// image data
std::vector<unsigned char> image_normal, image_disp;

// VAOs and VBOs
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

    program_outline.SetUniformMatrix4("vp", vp.cell);

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

// change tesselation level
void changeTessLevel(int delta = 0)
{
    static int value = 1;

    if (delta != 0)
    {
        value += delta;

        if (value < 1)
            value = 1;
        if (value > 64)
            value = 64;

        program.SetUniform("tessLevel", value);
        program_outline.SetUniform("tessLevel", value);
        program_shadow.SetUniform("tessLevel", value);
    }
}

// update shader uniforms
void setUniforms()
{
    program.SetUniformMatrix4("m", modelMatrix.cell);
    program_outline.SetUniformMatrix4("m", modelMatrix.cell);
    program.SetUniform("lightFovRad", lightFOV);
    program.SetUniform("dispSize", 8.0f);
    program_outline.SetUniform("dispSize", 8.0f);
    program_shadow.SetUniform("dispSize", 8.0f);
    program.SetUniform("tessLevel", 1);
    program_outline.SetUniform("tessLevel", 1);
    program_shadow.SetUniform("tessLevel", 1);
}

// compile outline, shadow, and light hint shaders
void compileShaders()
{
    // object shaders
    if (!program.BuildFiles("shader.vert", "shader.frag", nullptr, "shader.tesc", "shader.tese"))
        exit(1);

    // outline shaders
    if (!program_outline.BuildFiles("shader.vert", "outline.frag", "outline.geom", "shader.tesc", "outline.tese"))
        exit(1);

    // shadow shaders
    if (!program_shadow.BuildFiles("shader.vert", "shadow.frag", nullptr, "shader.tesc", "shadow.tese"))
        exit(1);

    // light hint shaders
    if (!program_hint.BuildFiles("hint.vert", "hint.frag"))
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

// listen for 'Esc' (to quit) and Space key (to render triangulation)
void keyListener(unsigned char key, int x, int y)
{
    switch (key)
    {
    // Esc key
    case 27:
        glutLeaveMainLoop();
        break;

    case 32:
        renderOutline = !renderOutline;
        glutPostRedisplay();
        break;
    }
}

// listen for 'F6' to re-compile shaders, left and right arrow keys to increase tesselation levels
void spclKeyListener(int key, int x, int y)
{
    switch (key)
    {
    // F6 key
    case GLUT_KEY_F6:
        compileShaders();
        break;

    case GLUT_KEY_LEFT:
        changeTessLevel(-1);
        break;

    case GLUT_KEY_RIGHT:
        changeTessLevel(1);
        break;
    }
    glutPostRedisplay();
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

// load image files using lodepng
bool loadImage(const char *fileName, unsigned width, unsigned height, std::vector<unsigned char> &outImage)
{
    unsigned read_status = lodepng::decode(outImage, width, height, fileName);
    if (read_status != 0)
    {
        std::cout << "Error: " << lodepng_error_text(read_status) << std::endl;
        return false;
    }
    return true;
}

// bind normal and displacement maps (if any) to shaders
void bindTextures()
{
    normalMap.Initialize();
    normalMap.SetImage(image_normal.data(), 4, imageWidth, imageHeight);
    normalMap.SetFilteringMode(GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
    normalMap.BuildMipmaps();

    program.SetUniform("normalMap", 1);

    if (hasDisp)
    {
        displacementMap.Initialize();
        displacementMap.SetImage(image_disp.data(), 4, imageWidth, imageHeight);
        displacementMap.SetFilteringMode(GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
        displacementMap.BuildMipmaps();

        program.SetUniform("dispMap", 2);
        program_outline.SetUniform("dispMap", 2);
        program_shadow.SetUniform("dispMap", 2);
    }

    program.SetUniform("hasDisp", hasDisp);
}

// parse arguments
void parseArgs(int argc, char *argv[])
{
    // edge cases
    if (argc < 2 || argc > 3)
    {
        std::cout << "Error: invalid number of arguments";
        std::cout << argc;
        exit(1);
    }
    else if (argc == 3)
    {
        // has a displacement map
        hasDisp = true;
        loadImage(argv[1], imageWidth, imageHeight, image_normal);
        loadImage(argv[2], imageWidth, imageHeight, image_disp);
    }
    else
    {
        // argc == 2, doesn't have a displacement map
        hasDisp = false;
        loadImage(argv[1], imageWidth, imageHeight, image_normal);
    }
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
        -squareSize / 2.0f,
        0.0f,
        -squareSize / 2.0f,
        squareSize / 2.0f,
        0.0f,
        squareSize / 2.0f,
        squareSize / 2.0f,
        0.0f,
        squareSize / 2.0f,
        -squareSize / 2.0f,
        0.0f,
    };
    static const GLfloat squareTexCoord[] = {
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        1.0f,
        1.0f,
    };

    // shader variables
    GLuint pos_square = program.AttribLocation("iPos");
    GLuint tex_square = program.AttribLocation("iTexCoord");

    // vbo
    glGenBuffers(2, vbo_square);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_square[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertexPos), squareVertexPos, GL_STATIC_DRAW);
    glVertexAttribPointer(pos_square, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(pos_square);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_square[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(squareTexCoord), squareTexCoord, GL_STATIC_DRAW);
    glVertexAttribPointer(tex_square, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(tex_square);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    glPatchParameteri(GL_PATCH_VERTICES, 4);
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

    program.SetUniform("shadowMap", 3);
}

// draw function
void draw()
{
    // render the plane in shadow camera  (if displacement map is rendered)
    if (hasDisp)
    {
        shadowMap.Bind();
        glClear(GL_DEPTH_BUFFER_BIT);
        program_shadow.Bind();
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, displacementMap.GetID());
        glBindVertexArray(vao_square);
        glDrawArrays(GL_PATCHES, 0, 4);
        shadowMap.Unbind();
    }

    // render the plane in world camera
    program.Bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalMap.GetID());
    if (hasDisp)
    {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, displacementMap.GetID());
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, shadowMap.GetTextureID());
    }
    glBindVertexArray(vao_square);
    glDrawArrays(GL_PATCHES, 0, 4);

    // render the outline of the plane
    if (renderOutline)
    {
        program_outline.Bind();
        if (hasDisp)
        {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, displacementMap.GetID());
        }
        glBindVertexArray(vao_square);
        glDrawArrays(GL_PATCHES, 0, 4);
    }

    // render the hint object
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
    glutCreateWindow("Project 8");

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
        std::cout << "Error: GLEW not initialized";
        exit(1);
    }

    // parse arguments
    parseArgs(argc, argv);

    // compile shaders
    compileShaders();

    // bind normal and displacement maps (if any)
    bindTextures();

    // set VBOs and VAOs
    getPlaneVBOs();
    getHintVBOs();

    // setup shadow map
    setShadowMap();

    glEnable(GL_DEPTH_TEST);

    // draw loop
    glutMainLoop();

    // clear VBOs and VAOs
    glDeleteBuffers(1, &vbo_square[0]);
    glDeleteBuffers(1, &vbo_square[1]);
    glDeleteBuffers(1, &vbo_hint);
    glDeleteVertexArrays(1, &vao_hint);
    glDeleteVertexArrays(1, &vao_square);

    return 0;
}