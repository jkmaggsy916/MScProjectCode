#include "glwidget.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include <QOpenGLShaderProgram>
#include <QCoreApplication>
#include <QTime>
#include <math.h>
#include <QTimer>
#include <chrono>

bool GLWidget::m_transparent = false;

int perm[256] = { 151,160,137,91,90,15,
  131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
  190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
  88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
  77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
  102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
  135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
  5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
  223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
  129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
  251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
  49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
  138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180 };

int grad4[32][4] = { {0,1,1,1}, {0,1,1,-1}, {0,1,-1,1}, {0,1,-1,-1}, // 32 tesseract edges
                   {0,-1,1,1}, {0,-1,1,-1}, {0,-1,-1,1}, {0,-1,-1,-1},
                   {1,0,1,1}, {1,0,1,-1}, {1,0,-1,1}, {1,0,-1,-1},
                   {-1,0,1,1}, {-1,0,1,-1}, {-1,0,-1,1}, {-1,0,-1,-1},
                   {1,1,0,1}, {1,1,0,-1}, {1,-1,0,1}, {1,-1,0,-1},
                   {-1,1,0,1}, {-1,1,0,-1}, {-1,-1,0,1}, {-1,-1,0,-1},
                   {1,1,1,0}, {1,1,-1,0}, {1,-1,1,0}, {1,-1,-1,0},
                   {-1,1,1,0}, {-1,1,-1,0}, {-1,-1,1,0}, {-1,-1,-1,0} };
/*
 * initGradTexture(GLuint *texID) - create and load a 2D texture for
 * a combined index permutation and gradient lookup table.
 * This texture is used for 2D, 3D and 4D noise, both classic and simplex.
 */


GLWidget::GLWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    if (m_transparent) {
        QSurfaceFormat fmt = format();
        fmt.setAlphaBufferSize(8);
        setFormat(fmt);
    }

    fps = 0;

    QTimer* fpsTimer = new QTimer();
    QObject::connect(fpsTimer, SIGNAL(timeout()), this, SLOT(showFps()));
    fpsTimer->start(1000);

    this->timer = new QTimer();
    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(processing()));
    timer->start(0);
}

void GLWidget::showFps()
{
    emit fpsChanged(fps);
    fps = 0;
}

void GLWidget::processing()
{
    update();
    fps++;
}

GLWidget::~GLWidget()
{
    cleanup();
}

QSize GLWidget::minimumSizeHint() const
{
    return QSize(50, 50);
}

QSize GLWidget::sizeHint() const
{
    return QSize(400, 400);
}

static void qNormalizeAngle(int& angle)
{
    while (angle < 0)
        angle += 360 * 16;
    while (angle > 360 * 16)
        angle -= 360 * 16;
}

void GLWidget::setXRotation(int angle)
{
    qNormalizeAngle(angle);
    if (angle != m_xRot) {
        m_xRot = angle;
        emit xRotationChanged(angle);
        update();
    }
}

void GLWidget::setYRotation(int angle)
{
    qNormalizeAngle(angle);
    if (angle != m_yRot) {
        m_yRot = angle;
        emit yRotationChanged(angle);
        update();
    }
}

void GLWidget::setZRotation(int angle)
{
    qNormalizeAngle(angle);
    if (angle != m_zRot) {
        m_zRot = angle;
        emit zRotationChanged(angle);
        update();
    }
}

void GLWidget::cleanup()
{
    if (m_program == nullptr)
        return;
    makeCurrent();
    m_terrainVbo.destroy();
    delete m_program;
    m_program = nullptr;
    doneCurrent();
}


QString fileContent(const QString& filePath, const char* codec = 0) {
    QFile f(filePath);
    if (!f.open(QFile::ReadOnly | QFile::Text)) return QString();
    QTextStream stream(&f);
    if (codec != 0) stream.setCodec(codec);
    return stream.readAll();
}


void GLWidget::initializeGL()
{
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &GLWidget::cleanup);

    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2, 0.2, 0.2, m_transparent ? 0 : 1);
   
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    initializeFallingSnow();
    initializeGroundSnow();
    initializeHeightMapShader();
    initializeFinalShader();
    
    
    initAndRunNoise();
    initializeDeformationCompute();
    m_camera.setToIdentity();
    m_camera.translate(0, -3.5, -14.0);

    const char* cmd = "\"C:\\Program Files\\ffmpeg\\bin\\ffmpeg.exe\" -r 60 -f rawvideo -pix_fmt rgba -s 1000x800 -i - -threads 0 -preset fast -y -pix_fmt yuv420p -crf 21 -vf vflip Videos\\output.mp4";
    ffmpeg = _popen(cmd, "wb");
    buffer = new int[1000 * 800];
    auto start = std::chrono::steady_clock::now();
}

void GLWidget::paintGL()
{
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<float> elapsed_seconds = end - start;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_world.setToIdentity();
    m_world.rotate(180.0f - (m_xRot / 16.0f), 1, 0, 0);
    m_world.rotate(m_yRot / 16.0f, 0, 1, 0);
    m_world.rotate(m_zRot / 16.0f, 0, 0, 1);

    runFallingSnow();
    runHeightMapShader();
    runGroundSnow();
    runDeformationCompute();
    runFinalShader();

    if (recording == true) {
        glReadPixels(0, 0, 1000, 800, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
        fwrite(buffer, sizeof(int) * 1000 * 800, 1, ffmpeg);
    }
    if (end_recording == true) {
        recording = false;
        _pclose(ffmpeg);
        end_recording = false;
    }
}

void GLWidget::resizeGL(int w, int h)
{
    m_proj.setToIdentity();
    m_proj.perspective(45.0f, GLfloat(w) / h, 1.0f, 50.0f);
}

void GLWidget::mousePressEvent(QMouseEvent* event)
{
    m_lastPos = event->pos();
}

void GLWidget::mouseMoveEvent(QMouseEvent* event)
{
    int dx = event->x() - m_lastPos.x();
    int dy = event->y() - m_lastPos.y();

    if (event->buttons() & Qt::LeftButton) {
        setXRotation(m_xRot + 8 * dy);
        setYRotation(m_yRot + 8 * dx);
    }
    else if (event->buttons() & Qt::RightButton) {
        setXRotation(m_xRot + 8 * dy);
        setZRotation(m_zRot + 8 * dx);
    }
    m_lastPos = event->pos();
}

void GLWidget::keyPressEvent(QKeyEvent* event)
{
    float m_cameraSpeed = 0.05;
    if (event->key() == Qt::Key_E) {
        m_worldPosition += m_up * m_cameraSpeed;
    }
    if (event->key() == Qt::Key_Q) {
        m_worldPosition -= m_up * m_cameraSpeed;
    }
    if (event->key() == Qt::Key_W) {
        m_worldPosition += m_forward * m_cameraSpeed;
    }
    if (event->key() == Qt::Key_S) {
        m_worldPosition -= m_forward * m_cameraSpeed;
    }
    if (event->key() == Qt::Key_A) {
        m_worldPosition += m_right * m_cameraSpeed;
    }
    if (event->key() == Qt::Key_D) {
        m_worldPosition -= m_right * m_cameraSpeed;
    }
    if (event->key() == Qt::Key_Up) {
        ball_translation += QVector3D(0.0, 0.0, m_cameraSpeed);
    }
    if (event->key() == Qt::Key_Down) {
        ball_translation += QVector3D(0.0, 0.0, -m_cameraSpeed);
    }
    if (event->key() == Qt::Key_Left) {
        ball_translation += QVector3D(-m_cameraSpeed, 0.0,0.0);
    }
    if (event->key() == Qt::Key_Right) {
        ball_translation += QVector3D(+m_cameraSpeed, 0.0, 0.0);
    }
    if (event->key() == Qt::Key_T) {
        location++;
    }
    m_camera.translate(m_worldPosition);
    m_worldPosition = QVector3D(0.0, 0.0, 0.0);
}


void GLWidget::loadTerrain(Terrain& terrain) {
    //terrain.heights = pgm::readPGM("Images/mountain.pgm", terrain.width, terrain.height);
    terrain.heights = pgm::readPGM("Images/heightmap.pgm", terrain.width, terrain.height);


    float scale = 6.0;
    for (int i = -terrain.width / 2; i < terrain.width / 2 - 1; i++) {
        for (int j = -terrain.height / 2; j < terrain.height / 2 - 1; j++) {
            QVector3D x_0z_0 = QVector3D(i + 0.0, scale * terrain.heights[(i + terrain.width / 2) * terrain.width + (j + terrain.height / 2)], j + 0.0);
            QVector3D x_0z_1 = QVector3D(i + 0.0, scale * terrain.heights[(i + terrain.width / 2) * terrain.width + (j + 1 + terrain.height / 2)], j + 1.0);
            QVector3D x_1z_0 = QVector3D(i + 1.0, scale * terrain.heights[(i + 1 + terrain.width / 2) * terrain.width + (j + terrain.height / 2)], j + 0.0);
            QVector3D x_1z_1 = QVector3D(i + 1.0, scale * terrain.heights[(i + 1 + terrain.width / 2) * terrain.width + (j + 1 + terrain.height / 2)], j + 1.0);

            QVector3D normal_1 = QVector3D::crossProduct((x_0z_1 - x_0z_0), (x_1z_0 - x_0z_0));
            QVector3D normal_2 = QVector3D::crossProduct((x_1z_0 - x_1z_1), (x_0z_1 - x_1z_1));

            //triangle 1
            normals.push_back(normal_1);
            QVector3D colour1v0 = calculateColour(x_0z_0.y(), scale);
            colours.push_back(colour1v0);
            triangles.push_back(x_0z_0);

            QVector3D colour1v1 = calculateColour(x_0z_1.y(), scale);
            colours.push_back(colour1v1);
            triangles.push_back(x_0z_1);

            QVector3D colour1v2 = calculateColour(x_1z_0.y(), scale);
            colours.push_back(colour1v2);
            triangles.push_back(x_1z_0);

            //triangle 2
            normals.push_back(normal_2);

            colours.push_back(colour1v1);
            triangles.push_back(x_0z_1);

            QVector3D colour2v1 = calculateColour(x_1z_1.y(), scale);
            colours.push_back(colour2v1);
            triangles.push_back(x_1z_1);

            colours.push_back(colour1v2);
            triangles.push_back(x_1z_0);
        }
    }

    for (int i = 0; i < triangles.size() / 3; i++) {
        QVector3D n = normals[i];
        QVector3D c0 = colours[i * 3 + 0];
        QVector3D c1 = colours[i * 3 + 1];
        QVector3D c2 = colours[i * 3 + 2];
        QVector3D v0 = triangles[i * 3 + 0];
        vertices.push_back(v0.x());
        vertices.push_back(v0.y());
        vertices.push_back(v0.z());
        vertices.push_back(n.x());
        vertices.push_back(n.y());
        vertices.push_back(n.z());
        vertices.push_back(c0.x());
        vertices.push_back(c0.y());
        vertices.push_back(c0.z());
        QVector3D v1 = triangles[i * 3 + 1];
        vertices.push_back(v1.x());
        vertices.push_back(v1.y());
        vertices.push_back(v1.z());
        vertices.push_back(n.x());
        vertices.push_back(n.y());
        vertices.push_back(n.z());
        vertices.push_back(c1.x());
        vertices.push_back(c1.y());
        vertices.push_back(c1.z());
        QVector3D v2 = triangles[i * 3 + 2];
        vertices.push_back(v2.x());
        vertices.push_back(v2.y());
        vertices.push_back(v2.z());
        vertices.push_back(n.x());
        vertices.push_back(n.y());
        vertices.push_back(n.z());
        vertices.push_back(c2.x());
        vertices.push_back(c2.y());
        vertices.push_back(c2.z());
    }
}

QVector3D GLWidget::calculateColour(float height, float scale) {
    float normalized_height = ((height / scale) + 1.0f) / 2.0f; //between 0 and 1;
    if (normalized_height > 0.90) {
        return QVector3D(1.0f, 1.0f, 1.0f);
    }
    else if (normalized_height > 0.85) {
        QVector3D colourTop = QVector3D(1.0f, 1.0f, 1.0f);
        QVector3D colourBot = QVector3D(0.65f, 0.77f, 0.86f);
        float diff = (0.9 - normalized_height) / 0.05;
        QVector3D colour = (1.0f - diff) * colourTop + diff * colourBot;
        return colour;
    }
    else if (normalized_height > 0.75) {
        return QVector3D(0.6f, 0.6f, 0.6f);
    }
    else if (normalized_height > 0.5f) {
        QVector3D colourTop = QVector3D(0.6f, 0.6f, 0.6f);
        QVector3D colourBot = QVector3D(0.58f, 0.58f, 0.4f);
        float diff = (0.9f - normalized_height) / 0.25f;
        QVector3D colour = (1.0f - diff) * colourTop + diff * colourBot;
        return colour;
        //return QVector3D(0.58f, 0.58f, 0.4f);
    }
    else if (normalized_height > 0.2f) {
        return QVector3D(0.0f, 0.65f, 0.15f);
    }
    else {
        return QVector3D(0.0f, 0.5f, 0.75f);
    }
}

void GLWidget::initializeFallingSnow()
{
    m_fallingSnowProgram = new QOpenGLShaderProgram;
    m_fallingSnowProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, fileContent("Shaders/snowFalling.vert"));
    m_fallingSnowProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fileContent("Shaders/snowFalling.frag"));
    m_fallingSnowProgram->bindAttributeLocation("vertex", 0);

    m_fallingSnowProgram->bind();
    m_projMatrixLoc = m_fallingSnowProgram->uniformLocation("projMatrix");
    m_mvMatrixLoc = m_fallingSnowProgram->uniformLocation("mvMatrix");
    m_normalMatrixLoc = m_fallingSnowProgram->uniformLocation("normalMatrix");

    /*Snow shader uniform inputs*/
    m_iResolutionLoc = m_fallingSnowProgram->uniformLocation("iResolution");
    m_iTimeLoc = m_fallingSnowProgram->uniformLocation("iTime");
    m_iMouseLoc = m_fallingSnowProgram->uniformLocation("iMouse");


    glGenFramebuffers(1, &fallingSnowFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, fallingSnowFBO);
   
    glGenTextures(1, &fallingSnowTex);
    glBindTexture(GL_TEXTURE_2D, fallingSnowTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1000, 800, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, fallingSnowTex);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fallingSnowTex, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    m_snowFallingVao.create();
    QOpenGLVertexArrayObject::Binder vaoBinder(&m_snowFallingVao);
    
    QVector<float> g_quad_vertex_buffer_data = {
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        1.0f,  1.0f, 0.0f,
    };

    m_quadVbo.create();
    m_quadVbo.bind();
    m_quadVbo.allocate(g_quad_vertex_buffer_data.constData(), g_quad_vertex_buffer_data.size() * sizeof(float));

    m_quadVbo.bind();
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    m_quadVbo.release();

    m_fallingSnowProgram->release();
}

void GLWidget::initializeGroundSnow()
{
    sphere.generateSphere(1.0, 320, 200);
    groundplane.generateMesh(10, 10, 0.015f);
    //groundplane.generateMesh(1024, 1024, 0.5f);
    

    groundplane.triangles += sphere.triangles;

    m_groundSnowProgram = new QOpenGLShaderProgram;
    m_groundSnowProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, fileContent("Shaders/snowGround.vert"));
    m_groundSnowProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fileContent("Shaders/snowGround.frag"));
    m_groundSnowProgram->bindAttributeLocation("vertex", 0);
    m_groundSnowProgram->bindAttributeLocation("texCoords", 1);
    m_groundSnowProgram->bindAttributeLocation("type", 2);
    m_groundSnowProgram->bind();

    m_projMatrixLoc = m_groundSnowProgram->uniformLocation("projMatrix");
    m_mvMatrixLoc = m_groundSnowProgram->uniformLocation("mvMatrix");
    m_ballMatrixLoc = m_groundSnowProgram->uniformLocation("ballMatrix");

    glGenFramebuffers(1, &groundSnowFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, groundSnowFBO);
 
    glGenTextures(1, &groundSnowTex);
    glBindTexture(GL_TEXTURE_2D, groundSnowTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1000, 800, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, groundSnowTex);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, groundSnowTex, 0);

    glGenRenderbuffers(1, &groundSnowRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, groundSnowRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 1000, 800);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, groundSnowRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    m_groundSnowVAO.create();
    QOpenGLVertexArrayObject::Binder vaoBinder(&m_groundSnowVAO);

    m_groundSnowVBO.create();
    m_groundSnowVBO.bind();
    m_groundSnowVBO.allocate(groundplane.triangles.constData(), groundplane.triangles.size() * sizeof(float));
    m_groundSnowVBO.bind();

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    f->glEnableVertexAttribArray(2);
    f->glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(5 * sizeof(float)));

    m_groundSnowProgram->release();
}

void GLWidget::initializeFinalShader()
{

    m_screenShaderProgram = new QOpenGLShaderProgram;
    m_screenShaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, fileContent("Shaders/screen.vert"));
    m_screenShaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fileContent("Shaders/screen.frag"));
    m_screenShaderProgram->bindAttributeLocation("aPos", 0);
    m_screenShaderProgram->bindAttributeLocation("aTexCoords", 1);
    m_screenShaderProgram->bind();
    
    m_snowGroundTexLoc =  m_screenShaderProgram->uniformLocation("snowGroundTexture");
    m_snowFallingTexLoc =  m_screenShaderProgram->uniformLocation("snowFallingTexture");
    m_debugTexLoc =  m_screenShaderProgram->uniformLocation("debugTexture");

    quadVAO.create();
    QOpenGLVertexArrayObject::Binder vaoBinder(&quadVAO);
    QVector<float> quadVertices = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    quadVBO.create();
    quadVBO.bind();
    quadVBO.allocate(quadVertices.constData(), quadVertices.size() * sizeof(float));
    quadVBO.bind();
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    quadVBO.release();
    m_fallingSnowProgram->release();
}

void GLWidget::initializeHeightMapShader()
{
    m_heightmapShader = new QOpenGLShaderProgram;
    m_heightmapShader->addShaderFromSourceCode(QOpenGLShader::Vertex, fileContent("Shaders/snowHeight.vert"));
    m_heightmapShader->addShaderFromSourceCode(QOpenGLShader::Fragment, fileContent("Shaders/snowHeight.frag"));
    m_heightmapShader->bindAttributeLocation("vertex", 0);
    m_heightmapShader->bindAttributeLocation("texCoords", 1);
    m_heightmapShader->bindAttributeLocation("type", 2);
    m_heightmapShader->bind();

    m_orthoProjMatrixLoc = m_heightmapShader->uniformLocation("projMatrix");
    m_ballMatrixLoc = m_heightmapShader->uniformLocation("ballMatrix");

    glGenFramebuffers(1, &heightMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, heightMapFBO);
    glGenTextures(1, &heightMapTex);
    glBindTexture(GL_TEXTURE_2D, heightMapTex);
    /*glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, heightMapTex);*/
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, heightMapTex, 0);
    //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, heightMapTex, 0);
    
    //glBindTexture(GL_TEXTURE_2D, heightMapTex);
    

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    m_groundSnowVAO2.create();
    QOpenGLVertexArrayObject::Binder vaoBinder(&m_groundSnowVAO2);
    m_groundSnowVBO2.create();
    m_groundSnowVBO2.bind();
    m_groundSnowVBO2.allocate(groundplane.triangles.constData(), groundplane.triangles.size() * sizeof(float));
    m_groundSnowVBO2.bind();

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    f->glEnableVertexAttribArray(2);
    f->glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(5 * sizeof(float)));

    m_heightmapShader->release();
}

void GLWidget::initializeDeformationCompute()
{
    funcs = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_4_3_Core>();
    if (!funcs) {
        qWarning() << "Could not obtain required OpenGL context version";
        exit(1);
    }

    glActiveTexture(GL_TEXTURE0);
    m_deformationTexture1 = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_deformationTexture1->create();
    m_deformationTexture1->setFormat(QOpenGLTexture::RGBA8_UNorm);
    m_deformationTexture1->setSize(1024, 1024);
    m_deformationTexture1->setMinificationFilter(QOpenGLTexture::Linear);
    m_deformationTexture1->setMagnificationFilter(QOpenGLTexture::Linear);
    m_deformationTexture1->allocateStorage();
    m_deformationTexture1->bind();
    
    glActiveTexture(GL_TEXTURE1);
    m_deformationTexture2 = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_deformationTexture2->create();
    m_deformationTexture2->setFormat(QOpenGLTexture::RGBA8_UNorm);
    m_deformationTexture2->setSize(1024, 1024);
    m_deformationTexture2->setMinificationFilter(QOpenGLTexture::Linear);
    m_deformationTexture2->setMagnificationFilter(QOpenGLTexture::Linear);
    m_deformationTexture2->allocateStorage();
    m_deformationTexture2->bind();
    

    m_deformationCompute = new QOpenGLShaderProgram;
    m_deformationCompute->addShaderFromSourceCode(QOpenGLShader::Compute, fileContent("Shaders/deformation.comp"));
    m_deformationCompute->link();
    m_deformationCompute->bind();
    m_deformationCompute->release();

    m_currentReadTex = 2;
    m_currentWriteTex = 1;
}

//IMPLEMENTATION BUILT OFF Stefan Gustavson'S IMPROVED NOISE CODE. - MIT LICENCE
//PAPER USED: http://weber.itn.liu.se/~stegu/simplexnoise/simplexnoise.pdf

void GLWidget::initGradTexture(GLuint* texID)
{
    char* pixels;
    int i, j;

    glGenTextures(1, texID); // Generate a unique texture ID
    glBindTexture(GL_TEXTURE_2D, *texID); // Bind the texture to texture unit 0

    pixels = (char*)malloc(256 * 256 * 4);
    for (i = 0; i < 256; i++)
        for (j = 0; j < 256; j++) {
            int offset = (i * 256 + j) * 4;
            char value = perm[(j + perm[i]) & 0xFF];
            pixels[offset] = grad4[value & 0x0F][0] * 64 + 128;   // Gradient x
            pixels[offset + 1] = grad4[value & 0x0F][1] * 64 + 128; // Gradient y
            pixels[offset + 2] = grad4[value & 0x0F][2] * 64 + grad4[value & 0x0F][3] * 16 + 128; // Gradient z,w
            pixels[offset + 3] = value;                     // Permuted index
        }

    // GLFW texture loading functions won't work here - we need GL_NEAREST lookup.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

GLuint gradTextureID;

void GLWidget::initAndRunNoise()
{
    
    funcs = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_4_3_Core>();
    if (!funcs) {
        qWarning() << "Could not obtain required OpenGL context version";
        exit(1);
    }

    initGradTexture(&gradTextureID);
    

    glActiveTexture(GL_TEXTURE2);
    m_texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_texture->create();
    m_texture->setFormat(QOpenGLTexture::RGBA8_UNorm);
    m_texture->setSize(1024, 1024);
    m_texture->setMinificationFilter(QOpenGLTexture::Linear);
    m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
    m_texture->allocateStorage();
    m_texture->bind();

    funcs->glBindImageTexture(0, m_texture->textureId(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    m_computeShader = new QOpenGLShaderProgram;
    m_computeShader->addShaderFromSourceCode(QOpenGLShader::Compute, fileContent("Shaders/noise.comp"));
    m_computeShader->link();
    m_computeShader->bind();
    m_computeShader->release();

    static GLint destLoc = glGetUniformLocation(m_computeShader->programId(), "noiseImg");
    m_computeShader->bind();
    m_texture->bind();
    glUniform1i(destLoc, 0);

    glBindTexture(GL_TEXTURE_2D, gradTextureID);
    static GLint permLoc = glGetUniformLocation(m_computeShader->programId(), "permTexture");
    glUniform1i(permLoc, 0);
    funcs->glDispatchCompute(m_texture->width() / 32, m_texture->height() / 32, 1);
    funcs->glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    m_computeShader->release();
    delete m_computeShader;
    m_computeShader = nullptr;

}

void GLWidget::runFallingSnow()
{
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<float> elapsed_seconds = end - start;

    glBindFramebuffer(GL_FRAMEBUFFER, fallingSnowFBO);
    glViewport(0, 0, 1000, 800);

    QOpenGLVertexArrayObject::Binder vaoBinder(&m_snowFallingVao);
    m_fallingSnowProgram->bind();
    m_fallingSnowProgram->setUniformValue(m_projMatrixLoc, m_proj);
    m_fallingSnowProgram->setUniformValue(m_mvMatrixLoc, m_camera * m_world);
    QMatrix3x3 normalMatrix = m_world.normalMatrix();
    m_fallingSnowProgram->setUniformValue(m_normalMatrixLoc, normalMatrix);
    m_fallingSnowProgram->setUniformValue(m_iResolutionLoc, QVector2D(width(), height()));
    float time_seconds = elapsed_seconds.count();
    m_fallingSnowProgram->setUniformValue(m_iTimeLoc, time_seconds);
    m_fallingSnowProgram->setUniformValue(m_iMouseLoc, 16.0*QVector2D(m_lastPos.x(), m_lastPos.y()));

    int snow_amount_loc = m_fallingSnowProgram->uniformLocation("num_snow_flakes");
    m_fallingSnowProgram->setUniformValue(snow_amount_loc, num_snowflakes);

    int snow_blizzard_loc = m_fallingSnowProgram->uniformLocation("blizzard_amount");
    m_fallingSnowProgram->setUniformValue(snow_blizzard_loc, blizzard_amount);

    int snow_turbulance_loc = m_fallingSnowProgram->uniformLocation("turb_amount");
    m_fallingSnowProgram->setUniformValue(snow_turbulance_loc, turb_amount);

    glDrawArrays(GL_TRIANGLES, 0, 6);
    m_fallingSnowProgram->release();
}

void GLWidget::runGroundSnow()
{
    glBindFramebuffer(GL_FRAMEBUFFER, groundSnowFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glViewport(0, 0, 1000, 800);
    QOpenGLVertexArrayObject::Binder vaoBinder(&m_groundSnowVAO);

    QMatrix4x4 ball_pos = QMatrix4x4(1.0, 0.0, 0.0, ball_translation.x(),
        0.0, 1.0, 0.0, ball_translation.y(),
        0.0, 0.0, 1.0, ball_translation.z(),
        0.0, 0.0, 0.0, 1.0);

    m_groundSnowProgram->bind();
    m_groundSnowProgram->setUniformValue(m_projMatrixLoc, m_proj);
    m_groundSnowProgram->setUniformValue(m_mvMatrixLoc, m_camera * m_world);
    m_groundSnowProgram->setUniformValue(m_ballMatrixLoc, ball_pos);

    m_noiseTexLoc = m_groundSnowProgram->uniformLocation("noiseTexture");
    glActiveTexture(GL_TEXTURE0);
    m_texture->bind();
    glUniform1i(m_noiseTexLoc, 0);

    m_heightTecLoc = m_groundSnowProgram->uniformLocation("heightTexture");
    glActiveTexture(GL_TEXTURE1);
    m_deformationTexture1->bind();
    //glBindTexture(GL_TEXTURE_2D, heightMapTex);
    glUniform1i(m_heightTecLoc, 1);

    glDrawArrays(GL_TRIANGLES, 0, groundplane.triangles.size() / 6);
    //glDrawArrays(GL_LINES, 0, groundplane.triangles.size() / 6);
    m_groundSnowProgram->release();
}

void GLWidget::runFinalShader()
{   
    m_screenShaderProgram->bind();
    QOpenGLVertexArrayObject::Binder vaoBinder(&quadVAO);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fallingSnowTex);
    glUniform1i(m_snowFallingTexLoc, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, groundSnowTex);
    glUniform1i(m_snowGroundTexLoc, 1);

    /*debug texture*/
    int debugEnabledLoc = m_screenShaderProgram->uniformLocation("debugEnabled");
    
    switch (debug_image_index)
    {
    case 0:
        m_screenShaderProgram->setUniformValue(debugEnabledLoc, false);
        break;
    case 1:
        m_screenShaderProgram->setUniformValue(debugEnabledLoc, true);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, fallingSnowTex);
        glUniform1i(m_debugTexLoc, 2);
        break;
    case 2:
        m_screenShaderProgram->setUniformValue(debugEnabledLoc, true);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, groundSnowTex);
        glUniform1i(m_debugTexLoc, 2);
        break;
    case 3:
        m_screenShaderProgram->setUniformValue(debugEnabledLoc, true);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, heightMapTex);
        glUniform1i(m_debugTexLoc, 2);
        break;
    case 4:
        m_screenShaderProgram->setUniformValue(debugEnabledLoc, true);
        glActiveTexture(GL_TEXTURE2);
        m_texture->bind();
        glUniform1i(m_debugTexLoc, 2);
        break;
    case 5:
        m_screenShaderProgram->setUniformValue(debugEnabledLoc, true);
        glActiveTexture(GL_TEXTURE2);
        m_deformationTexture1->bind();
        glUniform1i(m_debugTexLoc, 2);
        break;
    case 6:
        m_screenShaderProgram->setUniformValue(debugEnabledLoc, true);
        glActiveTexture(GL_TEXTURE2);
        m_deformationTexture2->bind();
        glUniform1i(m_debugTexLoc, 2);
        break;
    }
    
   

    glDrawArrays(GL_TRIANGLES, 0, 6);
    m_screenShaderProgram->release();
    glEnable(GL_DEPTH_TEST);
}

void GLWidget::runHeightMapShader()
{
    glDisable(GL_CULL_FACE);
    m_orthoProj.setToIdentity();
    m_orthoProj.ortho(-5.0, 5.0, -5.0, 5.0, 0.01, 5.01);
    heightView.setToIdentity();
    heightView.lookAt(QVector3D(0.0, -3.01, 0.0),
        QVector3D(0.00001, 0.0, 0.0), QVector3D(0.0, 1.0, 0.0));
    
    
    glBindFramebuffer(GL_FRAMEBUFFER, heightMapFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, 1024, 1024);
    QOpenGLVertexArrayObject::Binder vaoBinder(&m_groundSnowVAO);

    QMatrix4x4 ball_pos = QMatrix4x4(1.0, 0.0, 0.0, ball_translation.x(),
        0.0, 1.0, 0.0, ball_translation.y(),
        0.0, 0.0, 1.0, ball_translation.z(),
        0.0, 0.0, 0.0, 1.0);

    m_heightmapShader->bind();
    m_heightmapShader->setUniformValue(m_orthoProjMatrixLoc, m_orthoProj * heightView);
    //m_heightmapShader->setUniformValue(m_orthoProjMatrixLoc, m_proj * m_camera * m_world);
    m_heightmapShader->setUniformValue(m_ballMatrixLoc, ball_pos);
    glDrawArrays(GL_TRIANGLES, 0, groundplane.triangles.size());
    m_heightmapShader->release();
    glEnable(GL_CULL_FACE);
}

void GLWidget::runDeformationCompute()
{
    funcs = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_4_3_Core>();
    if (!funcs) {
        qWarning() << "Could not obtain required OpenGL context version";
        exit(1);
    }

    static GLint heightTecLoc = glGetUniformLocation(m_deformationCompute->programId(), "heightTexture");
    //std::cout << heightTecLoc << std::endl;
    m_deformationCompute->bind();
    
    /*glActiveTexture(GL_TEXTURE0);
    
    funcs->glBindTexture(GL_TEXTURE_2D, heightMapTex);
    glUniform1i(heightTecLoc,0);*/
    m_heightTecLoc = m_deformationCompute->uniformLocation("heightTexture");
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, heightMapTex);
    glUniform1i(heightTecLoc, 1);

    int m_call_loc = m_deformationCompute->uniformLocation("call_no");
    m_deformationCompute->setUniformValue(m_call_loc, call_no);
    call_no = 1;

    if (m_currentWriteTex == 1) {
        funcs->glBindImageTexture(1, m_deformationTexture1->textureId(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);

        funcs->glBindImageTexture(2, m_deformationTexture2->textureId(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);

        m_currentWriteTex = 2;
        m_currentReadTex = 1;
    }
    else {
        funcs->glBindImageTexture(1, m_deformationTexture2->textureId(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);

        funcs->glBindImageTexture(2, m_deformationTexture1->textureId(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);

        m_currentWriteTex = 1;
        m_currentReadTex = 2;
    }

    
    funcs->glDispatchCompute(m_deformationTexture1->width() / 32, m_deformationTexture1->height() / 32, 1);
    funcs->glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    m_deformationCompute->release();
}
