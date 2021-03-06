/*
* Copyright (c) 2008 Antonie Jovanoski <minimoog77_at_gmail.com>
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/

#include "widget.h"
#include <QtDebug>
#include <QFile>
#include <QKeyEvent>

Widget::Widget(QWidget *parent) :
    QGLWidget(parent), m_textureManager(this)
{
    makeCurrent();

    GLenum err = glewInit();
    if (GLEW_OK != err) {
      /* Problem: glewInit failed, something is seriously wrong. */
      fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    }
}

void Widget::initializeGL()
{
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    m_scene.loadProc("./data/level.proc", this, m_textureManager);

    // shader mader
    GLuint program = glCreateProgram();
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    QFile vertexFile;
    vertexFile.setFileName("simple.vsh");
    vertexFile.open(QIODevice::ReadOnly);

    QFile fragmentFile;
    fragmentFile.setFileName("simple.fsh");
    fragmentFile.open(QIODevice::ReadOnly);

    QByteArray vertexByteArray = vertexFile.readAll();
    vertexByteArray.append('\0');
    const char *vsSource = vertexByteArray.constData();

    QByteArray fragmentByteArray = fragmentFile.readAll();
    fragmentByteArray.append('\0');
    const char *fsSource = fragmentByteArray.constData();

    glShaderSource(vertexShader, 1, &vsSource, NULL);
    glCompileShader(vertexShader);
    checkShader(vertexShader);

    glShaderSource(fragmentShader, 1, &fsSource, NULL);
    glCompileShader(fragmentShader);
    checkShader(fragmentShader);

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glBindAttribLocation(program, 0, "vertexPosition");
    glBindAttribLocation(program, 1, "vertexTexCoord");
    glBindAttribLocation(program, 2, "vertexNormal");

    glLinkProgram(program);
    checkProgram(program);

    m_scene.setProgram(program);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    m_ViewMatrix = glGetUniformLocation(program, "viewMatrix");
    m_ProjectionMatrix = glGetUniformLocation(program,"projectionMatrix");

    TVec3 at(2558.0f, -1779.0f, 76.0f);
    TVec3 eye(2558.0f, -1759.0f, 76.0f);
    TVec3 up(0.0f, 0.0f, 1.0f);

    m_kamera.lookat(eye.vec_array, at.vec_array, up.vec_array);
    m_kamera.setPerspectiveViewFOV(float(3.14 * 0.25), float(width()) / float(height()), 0.1f, 5000.0f);

    glUniformMatrix4fv(m_ProjectionMatrix, 1, GL_FALSE, m_kamera.getProjMatrix());

    glViewport(0, 0, width(), height());
}

void Widget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUniformMatrix4fv(m_ViewMatrix, 1, GL_FALSE, m_kamera.getViewMatrix().mat_array);

    m_scene.render(m_kamera);

    glError(__FILE__, __LINE__);
}

void Widget::resizeGL(int w, int h)
{
    m_kamera.setPerspectiveViewFOV(float(3.14 * 0.25), float(w) / float(h), 0.1f, 5000.0f);
    glUniformMatrix4fv(m_ProjectionMatrix, 1, GL_FALSE, m_kamera.getProjMatrix());

    glViewport(0, 0, w, h);

    updateGL();
}

void Widget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Left:
        m_kamera.rotateYY(- 0.01);
        updateGL();
        break;

    case Qt::Key_Right:
        m_kamera.rotateYY(0.01);
        updateGL();
        break;

    case Qt::Key_Up:
        m_kamera.moveForward(3.0f);
        updateGL();
        break;

    case Qt::Key_Down:
        m_kamera.moveForward(-3.0f);
        updateGL();
        break;

    case Qt::Key_L:
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        updateGL();
        break;

    default:
        QWidget::keyPressEvent(event);
    }
}

Widget::~Widget()
{
}

void Widget::checkShader(GLuint shader)
{
    GLint status;

    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if (status == GL_FALSE)
        qDebug() << "Compile error";
    else
        qDebug() << "Compile ok";

    int loglen;
    char logBuffer[1024];
    glGetShaderInfoLog(shader, sizeof(logBuffer), &loglen, logBuffer);

    if (loglen > 0)
        qDebug() << logBuffer;
}

void Widget::checkProgram(GLuint program)
{
    GLint status;

    glGetProgramiv(program, GL_LINK_STATUS, &status);

    if (status == GL_FALSE)
        qDebug() << "Linker error";
    else
        qDebug() << "Linker ok";

    int loglen;
    char logbuffer[1024];

    glGetProgramInfoLog(program, sizeof(logbuffer), &loglen, logbuffer);
    if (loglen > 0)
        qDebug() << logbuffer;

    glValidateProgram(program);
    glGetProgramInfoLog(program, sizeof(logbuffer), &loglen, logbuffer);

    if (loglen > 0)
        qDebug() << logbuffer;
}

void Widget::glError(const char *file, int line)
{
    GLenum err (glGetError());

    while (err!=GL_NO_ERROR) {
        QString error;

        switch(err) {
        case GL_INVALID_OPERATION:      error="INVALID_OPERATION";      break;
        case GL_INVALID_ENUM:           error="INVALID_ENUM";           break;
        case GL_INVALID_VALUE:          error="INVALID_VALUE";          break;
        case GL_OUT_OF_MEMORY:          error="OUT_OF_MEMORY";          break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:  error="INVALID_FRAMEBUFFER_OPERATION";  break;
        }

        qDebug() << "GL_" << error << " - " << file << ":" << line;

        err=glGetError();
    }
}
