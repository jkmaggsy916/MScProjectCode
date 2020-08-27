#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions_4_3_Core>
#include <QMatrix4x4>
#include "pgm.h"
#include <glm/glm.hpp>
#include <iostream>
#include "tools.h"
#include <QOpenGLTexture>

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)

struct Terrain
{
    int width;
    int height;
    float* heights;
};

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    GLWidget(QWidget* parent = nullptr);
    ~GLWidget();

    static bool isTransparent() { return m_transparent; }
    static void setTransparent(bool t) { m_transparent = t; }

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

public slots:
    void setXRotation(int angle);
    void setYRotation(int angle);
    void setZRotation(int angle);
    void cleanup();
    void showFps();
    void processing();

signals:
    void xRotationChanged(int angle);
    void yRotationChanged(int angle);
    void zRotationChanged(int angle);
    void fpsChanged(int fps_value);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void setupVertexAttribs();
    void loadTerrain(Terrain& terrain);
    QVector3D calculateColour(float height, float scale);

	void initializeFallingSnow();
	void initializeGroundSnow();
	void initializeFinalShader();
    void initializeHeightMapShader();
    void initializeDeformationCompute();

    void initAndRunNoise();

	void runFallingSnow();
	void runGroundSnow();
	void runFinalShader();
    void runHeightMapShader();
    void runDeformationCompute();

    bool m_core;
    int m_xRot = 0;
    int m_yRot = 0;
    int m_zRot = 0;
    QPoint m_lastPos;
    Terrain m_terrain;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer m_terrainVbo;
    QOpenGLShaderProgram* m_program = nullptr;
    int m_projMatrixLoc = 0;
    int m_mvMatrixLoc = 0;
    int m_normalMatrixLoc = 0;
    int m_lightPosLoc = 0;
    QMatrix4x4 m_proj;
    QMatrix4x4 m_orthoProj;
    QMatrix4x4 heightView;
    QMatrix4x4 m_camera;
    QMatrix4x4 m_world;
    static bool m_transparent;
    QTimer* timer;
    int fps;

    /*Movement things*/
    QVector3D m_worldPosition = QVector3D(0.0, 0.0, 0.0);
    QVector3D m_up = QVector3D(0.0, 1.0, 0.0);
    QVector3D m_forward = QVector3D(0.0, 0.0, 1.0);
    QVector3D m_right = QVector3D(1.0, 0.0, 0.0);
    

    /*Falling Snow things*/
    int m_iResolutionLoc = 0;
    int m_iTimeLoc = 0;
    int m_iMouseLoc = 0;
    float playbackTime_seconds = 0;
    std::chrono::steady_clock::time_point start;

    std::vector<QVector3D> triangles;
    std::vector<QVector3D> colours;
    std::vector<QVector3D> normals;
    QVector<float> vertices;

    /*Falling Snow shader things*/
    unsigned int fallingSnowTex;
    unsigned int fallingSnowFBO;
    QOpenGLVertexArrayObject m_snowFallingVao;
    QOpenGLBuffer m_quadVbo;
    QOpenGLShaderProgram* m_fallingSnowProgram = nullptr;
    int num_snowflakes = 50;
    float blizzard_amount = 0.2;
    float turb_amount = 0.1;

    /*Ground snow shader things*/
    unsigned int groundSnowTex;
    unsigned int groundSnowFBO;
    unsigned int groundSnowRBO;
    QOpenGLVertexArrayObject m_groundSnowVAO;
    QOpenGLBuffer m_groundSnowVBO;
    QOpenGLShaderProgram* m_groundSnowProgram = nullptr;
    Plane groundplane{};
    Sphere sphere{};
    int m_noiseTexLoc;
    int m_heightTecLoc;

    /*Screen shader things*/
    QOpenGLVertexArrayObject quadVAO;
    QOpenGLBuffer quadVBO;
    QOpenGLShaderProgram* m_screenShaderProgram = nullptr;
    int m_snowFallingTexLoc = 0;
    int m_snowGroundTexLoc = 0;
    int m_debugTexLoc = 0;
    int debug_image_index = 0;

    /*Ball things*/
    QVector3D ball_translation = QVector3D(0.0, 0.0, 0.0);
    int m_ballMatrixLoc = 0;

    /*Screen recording things*/
    int* buffer;
    FILE* ffmpeg;
    bool recording = false;
    bool end_recording = false;
    
    /*compute shader things*/
    QOpenGLShaderProgram* m_computeShader;
    unsigned int noiseTex;
    QOpenGLFunctions_4_3_Core* funcs = 0;
    QOpenGLTexture* m_texture;

    /*snow depth map things*/
    QOpenGLShaderProgram* m_heightmapShader;
    unsigned int heightMapTex;
    unsigned int heightMapFBO;
    int m_orthoProjMatrixLoc;
    QOpenGLVertexArrayObject m_groundSnowVAO2;
    QOpenGLBuffer m_groundSnowVBO2;

    /*snow build up and deformation calculations*/
    QOpenGLTexture* m_deformationTexture1;
    QOpenGLTexture* m_deformationTexture2;
    int m_currentReadTex;
    int m_currentWriteTex;
    QOpenGLShaderProgram* m_deformationCompute;

    //things
    int location = 0;
    int call_no = 0;

public slots:
    void start_recording() { recording = true; end_recording = false; }
    void stop_recording() { end_recording = true; recording = false; }
    void set_snow_amount(int value) { num_snowflakes = value; }
    void set_blizzard_amount(float value) { blizzard_amount = value / 100.0; }
    void set_turb_amount(float value) { turb_amount = value / 100.0; }
    void change_debug_image(int index) { debug_image_index = index; }
};

#endif