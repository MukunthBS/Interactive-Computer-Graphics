#include <GL/freeglut.h>

float hue = 0.0f;
float hueChange = 0.001f;

void display()
{

    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0, hue, hue, 1.0f);

    glutSwapBuffers();
}

void keyListener(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 27:
        glutLeaveMainLoop();
    }
}

void bgAnimator()
{

    hue += hueChange;
    if (hue > 1.0f || hue <= 0.0f)
    {
        hueChange *= -1;
        hue += hueChange;
    }

    glutPostRedisplay();
}

int main(int argc, char **argv)
{

    glutInit(&argc, argv);

    glutInitWindowSize(600, 600);
    glutInitWindowPosition(50, 50);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);

    glClearColor(0, 0, 0, 0);

    glutCreateWindow("Project 1");

    glutKeyboardFunc(keyListener);
    glutDisplayFunc(display);
    glutIdleFunc(bgAnimator);

    glutMainLoop();

    glClear(GL_COLOR_BUFFER_BIT);
    return 0;
}