#include "framerenderer.hpp"

#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QImage>
#include <QThread>
#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QInputDialog>

#include "logwidget.hpp"
#include "polygonrenderer.hpp"

static const char *basicVertexShader =
    "#version 330\n"
    "in vec2 position;\n"
    "in vec2 texcoord;\n"
    "out vec2 Texcoord;\n"
    "void main()\n"
    "{\n"
    "    Texcoord = texcoord;\n"
    "    gl_Position = vec4(position, 0.0, 1.0);\n"
    "}\n";

static const char *basicFragmentShader =
    "#version 330\n"
    "uniform sampler2D tex;\n"
    "uniform vec2 canvasSize;\n"
    "uniform vec4 canvasBoundary; // left right bottom top    \n"
    "uniform bool justUseRed;\n"
    "in vec2 Texcoord;\n"
    "out vec4 outColor;\n"
    "void main()\n"
    "{\n"
    "   vec2 canvasCoord = vec2(gl_FragCoord.x/canvasSize.x, gl_FragCoord.y/canvasSize.y);\n"
    "   vec2 textureCoord = vec2( canvasCoord.x*(canvasBoundary.y-canvasBoundary.x) + canvasBoundary.x, canvasCoord.y*(canvasBoundary.w-canvasBoundary.z) + canvasBoundary.z);\n"
    "   vec4 texColor = texture(tex,textureCoord);\n"
    "   outColor = vec4(texColor.r, texColor.r,texColor.r, 1.0);\n"
    "  // outColor = vec4(canvasCoord,0.0, 1.0);\n"
    "}\n";

namespace render_utility
{

    struct GLSL_Vec2
    {
        float x = 0.f;
        float y = 0.f;
    };
    // 为了防止和Qt的坐标系混淆
    struct GLSL_Rect
    {
        float min_x = 0.f; // 对应屏幕左边
        float min_y = 0.f; // 对应屏幕下边
        float max_x = 1.f; // 对应屏幕右边
        float max_y = 1.f; // 对应屏幕上边

        float width() const
        {
            return max_x - min_x;
        }

        float height() const
        {
            return max_y - min_y;
        }

        void translate(float dx, float dy)
        {
            min_x += dx;
            max_x += dx;
            min_y += dy;
            max_y += dy;
        }

        // get center
        GLSL_Vec2 center() const
        {
            GLSL_Vec2 center;
            center.x = (min_x + max_x) / 2.f;
            center.y = (min_y + max_y) / 2.f;
            return center;
        }

        // scale at uniform location (x,y) with factor_x and factor_y
        void scaleAtFixedPoint(float x, float y, float factorX, float factorY)
        {
            float actualLocationX = min_x + x * width();
            float actualLocationY = min_y + y * height();

            float newWidth = width() * factorX;
            float newHeight = height() * factorY;

            min_x = actualLocationX - x * newWidth;
            max_x = actualLocationX + (1.f - x) * newWidth;
            min_y = actualLocationY - y * newHeight;
            max_y = actualLocationY + (1.f - y) * newHeight;
        }
    };

    QPointF mapScreenToTexture(const QPoint &screenPoint, const QSize &screenSize, const QRectF &frameRect)
    {
        float x = (float)screenPoint.x() / (float)screenSize.width();
        float y = (float)screenPoint.y() / (float)screenSize.height();
        float left = frameRect.left();
        float right = frameRect.right();
        float top = frameRect.top();
        float bottom = frameRect.bottom();
        float textureX = x * (right - left) + left;
        float textureY = y * (bottom - top) + top;
        return QPointF(textureX, textureY);
    }

    QPointF unformPosition(const QPoint &p, const QSize &size)
    {
        float x = (float)p.x() / (float)size.width();
        float y = (float)p.y() / (float)size.height();
        return QPointF(x, y);
    }

    GLSL_Rect getAutoFitFrameRect(const QSize &textureSize, const QSize &Screensize)
    {
        float textureAspectRatio = (float)textureSize.width() / (float)textureSize.height();
        float screenAspectRatio = (float)Screensize.width() / (float)Screensize.height();

        GLSL_Rect rect;

        if (textureAspectRatio > screenAspectRatio)
        {
            // 纹理胖矮型 (相对于显示屏幕)
            rect.min_x = 0.0f;
            rect.max_x = 1.0f;

            rect.min_y = 0.5f - 0.5f * textureAspectRatio / screenAspectRatio;
            rect.max_y = 0.5f + 0.5f * textureAspectRatio / screenAspectRatio;
        }
        else
        {
            // 纹理瘦高型
            rect.min_y = 0.0f;
            rect.max_y = 1.0f;

            rect.min_x = 0.5f - 0.5f * screenAspectRatio / textureAspectRatio;
            rect.max_x = 0.5f + 0.5f * screenAspectRatio / textureAspectRatio;
        }

        return rect;
    }

    // 一个屏幕像素对应多少纹理像素再换算到归一化的纹理坐标系下
    float getAutoFitPixelRatio(const QSize &textureSize, const QSize &Screensize)
    {
        float textureAspectRatio = (float)textureSize.width() / (float)textureSize.height();
        float screenAspectRatio = (float)Screensize.width() / (float)Screensize.height();

        float pixelRatio;

        if (textureAspectRatio > screenAspectRatio)
        {
            // 纹理胖矮型
            pixelRatio = 1.f / (float)Screensize.height();
        }
        else
        {
            // 纹理瘦高型
            pixelRatio = 1.f / (float)Screensize.width();
        }

        return pixelRatio;
    }

}

// 用于绘制边框
class FrameBorder : protected QOpenGLFunctions_3_3_Core
{
public:
    FrameBorder() : VAO(0), VBO(0), glFuncs(nullptr) {}
    ~FrameBorder()
    {
        if (glFuncs != nullptr)
        {
            glFuncs->glDeleteVertexArrays(1, &VAO);
            glFuncs->glDeleteBuffers(1, &VBO);
        }
    }

    void initialize(QOpenGLFunctions_3_3_Core *f)
    {
        glFuncs = f;
        initShaders();
        initGeometry();
    }
    void draw()
    {
        shaderProgram.bind();
        glFuncs->glBindVertexArray(VAO);
        glFuncs->glDrawArrays(GL_LINE_LOOP, 0, 4);
        glFuncs->glBindVertexArray(0);
        shaderProgram.release();
    }

private:
    QOpenGLShaderProgram shaderProgram;
    GLuint VAO;
    GLuint VBO;
    QOpenGLFunctions_3_3_Core *glFuncs;

    void initShaders()
    {
        shaderProgram.addShaderFromSourceCode(QOpenGLShader::Vertex,
                                              "#version 330 core\n"
                                              "layout (location = 0) in vec2 position;\n"
                                              "void main() {\n"
                                              "   gl_Position = vec4(position.x, position.y, 0.0, 1.0);\n"
                                              "}");
        shaderProgram.addShaderFromSourceCode(QOpenGLShader::Fragment,
                                              "#version 330 core\n"
                                              "out vec4 FragColor;\n"
                                              "void main() {\n"
                                              "   FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
                                              "}");
        shaderProgram.link();
    }
    void initGeometry()
    {
        GLfloat vertices[] = {
            -0.999f, 0.999f,
            0.999f, 0.999f,
            0.999f, -0.999f,
            -0.999f, -0.999f};

        glFuncs->glGenVertexArrays(1, &VAO);
        glFuncs->glGenBuffers(1, &VBO);
        glFuncs->glBindVertexArray(VAO);
        glFuncs->glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glFuncs->glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glFuncs->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
        glFuncs->glEnableVertexAttribArray(0);
        glFuncs->glBindBuffer(GL_ARRAY_BUFFER, 0);
        glFuncs->glBindVertexArray(0);
    }
};

struct PolygonInfo
{
    std::vector<QVector2D> pointsInTexture;
    float infillIntensity = 1.0f;
    bool isClosed = false;
};

struct FrameRenderer::Impl
{
    FrameRenderer &owner;

    QOpenGLShaderProgram *shaderProgram = nullptr;
    QOpenGLTexture *cameraTexture = nullptr;
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo;
    QOpenGLBuffer ebo;

    // image transform
    render_utility::GLSL_Rect frame; // OpenGL视口在纹理坐标系的位置和大小信息
    float scaleFactor = 1.0f;        // 缩放因子
    bool isMoving = false;           // 是否正在移动画框
    QPoint lastPos;                  // 上一次鼠标的位置
    QSize lastSize;

    std::vector<PolygonInfo> polygons; // 所有多边形信息（纹理坐标系下）

    FrameBorder *frameBorder;
    PolygonRenderer *polygonRenderer;

    // cons
    Impl(FrameRenderer &o)
        : owner(o),
          frameBorder(new FrameBorder()),
          polygonRenderer(new PolygonRenderer())
    {
    }

    // dtor
    ~Impl()
    {
        polygons.clear();
        delete shaderProgram;
        delete cameraTexture;
    }

    void initializeGLResource()
    {
        owner.makeCurrent();

        // 初始化shader
        shaderProgram = new QOpenGLShaderProgram();
        shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, basicVertexShader);
        shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, basicFragmentShader);
        shaderProgram->bindAttributeLocation("position", 0);
        shaderProgram->bindAttributeLocation("texcoord", 1);
        shaderProgram->link();
        shaderProgram->bind();
        shaderProgram->setUniformValue("justUseRed", true);

        // 初始化默认纹理
        QImage image;
        bool loaded = image.load(":/placeholder.png"); // 替换为实际的文件路径
        if (!loaded)
        {
            qDebug() << "Failed to load image";
            image = QImage(100, 100, QImage::Format_RGB888);
            image.fill(Qt::black);
        }

        image = image.mirrored(false, true);

        QImage glCompatibleImage = image.convertToFormat(QImage::Format_RGBA8888);
        cameraTexture = new QOpenGLTexture(glCompatibleImage);
        cameraTexture->setWrapMode(QOpenGLTexture::ClampToBorder);
        cameraTexture->setBorderColor(QColor(Qt::black));

        // Initialize buffer data
        GLfloat vertices[] = {
            // Positions    // Texture Coords
            -1.0f, -1.0f, 0.0f, 0.0f, // Bottom-left
            -1.0f, 1.0f, 0.0f, 1.0f,  // Top-left
            1.0f, -1.0f, 1.0f, 0.0f,  // Bottom-right
            1.0f, 1.0f, 1.0f, 1.0f    // Top-right
        };

        GLuint indices[] = {
            // Note that we start from 0!
            0, 1, 2, // First Triangle
            0, 2, 3  // Second Triangle
        };

        vao.create();
        QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

        // 设置vbo
        vbo.create();
        vbo.bind();
        vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
        vbo.allocate(vertices, sizeof(vertices));

        // 配置vbo属性
        vbo.bind();
        owner.glEnableVertexAttribArray(0);
        owner.glEnableVertexAttribArray(1);
        owner.glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid *)0);
        owner.glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid *)(2 * sizeof(GLfloat)));
        vbo.release();

        // 设置ebo
        ebo.create();
        ebo.bind();
        ebo.setUsagePattern(QOpenGLBuffer::StaticDraw);
        ebo.allocate(indices, sizeof(indices));

        // 初始化frame border
        frameBorder->initialize(&owner);

        // 初始化polygon renderer
        polygonRenderer->initialize(&owner);

        GLenum error = owner.glGetError();
        if (error != GL_NO_ERROR)
        {
            Log::error(QString("OpenGL Error occured in FrameRenderer: %1").arg(error));
        }
    }

    std::vector<QVector2D> ndcPoints(const std::vector<QVector2D> &pointsInTexture) const
    {
        std::vector<QVector2D> pointsInNDC;
        for (auto &p : pointsInTexture)
        {
            QVector2D pointInNDC;
            pointInNDC.setX((p.x() - frame.min_x) / frame.width() * 2.0f - 1.0f);
            pointInNDC.setY((p.y() - frame.min_y) / frame.height() * 2.0f - 1.0f);
            pointsInNDC.push_back(pointInNDC);
        }
        return pointsInNDC;
    }
};

FrameRenderer::FrameRenderer(lzx::TripleBuffer<lzx::Frame> *tripleBuffer, QWidget *parent)
    : QOpenGLWidget(parent), impl(new Impl(*this)), frameBuffer(tripleBuffer)
{
    setMouseTracking(true);

    // 设置OpenGL版本
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setSwapInterval(1);
    setFormat(format);

    impl->lastSize = this->size();

    // Timer控制从相机不断获取帧
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, [=]()
            {
                qDebug() << "timer timeout";
                updateTimer->stop();
                if (frameBuffer)
                {
                    lzx::Frame *frame = frameBuffer->consume();
                    bool needRepaint = false;
                    if (frame && frame->width() > 0 && frame->height() > 0 && frame->channels() > 0)
                    {
                        onFrameChangedDirectMode(frame->data(), frame->width(), frame->height(), frame->channels());
                        needRepaint = true;
                    }
                    frameBuffer->consumeDone();
                    if (needRepaint)
                    {
                        update(); // repaint
                    }
                }
                if (enableUpdate)
                {
                    updateTimer->start(10);
                } });
    updateTimer->start(10); // 默认构造时就开始更新
}

FrameRenderer::~FrameRenderer()
{
    delete impl;
}

void FrameRenderer::setFrameBuffer(lzx::TripleBuffer<lzx::Frame> *tripleBuffer)
{
    frameBuffer = tripleBuffer;
}

std::vector<MaskPolygon> FrameRenderer::getMaskPolygons() const
{
    // Convert Impl polygons to MaskPolygon
    std::vector<MaskPolygon> maskPolygons;
    for (auto &polygon : impl->polygons)
    {
        MaskPolygon maskPolygon;
        maskPolygon.vertices = polygon.pointsInTexture;
        maskPolygon.intensity = polygon.infillIntensity;
        maskPolygons.push_back(maskPolygon);
    }
    return maskPolygons;
}

// initialize gl
void FrameRenderer::initializeGL()
{
    initializeOpenGLFunctions();

    impl->initializeGLResource();
}

void FrameRenderer::paintGL()
{
    qDebug() << "FrameRenderer::paintGL";

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // 渲染相机实时采图
    {
        static bool isFirstTime = true;
        if (isFirstTime)
        {
            isFirstTime = false;
            onAutoFit();
        }

        QOpenGLVertexArrayObject::Binder vaoBinder(&impl->vao);
        impl->shaderProgram->bind();

        // 激活并绑定纹理
        glActiveTexture(GL_TEXTURE0);
        impl->cameraTexture->bind();

        impl->shaderProgram->setUniformValue("tex", 0);

        {
            int canvasSizeWidth = this->width() * this->devicePixelRatio();
            int canvasSizeHeight = this->height() * this->devicePixelRatio();
            qDebug() << "canvasSizeWidth:" << canvasSizeWidth << "canvasSizeHeight:" << canvasSizeHeight;
            impl->shaderProgram->setUniformValue("canvasSize", canvasSizeWidth, canvasSizeHeight);
        }

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        impl->shaderProgram->release();
    }

    // 绘制边框 (DrawLines模式下)
    if (currentMode == Mode::DrawLines)
        impl->frameBorder->draw();

    // 绘制多边形
    for (auto &polygon : impl->polygons)
    {
        if (polygon.pointsInTexture.size() >= 2)
        {
            auto ndcPoints = impl->ndcPoints(polygon.pointsInTexture);
            impl->polygonRenderer->draw(ndcPoints,
                                        polygon.isClosed ? PolygonRenderer::Mode::Filler : PolygonRenderer::Mode::Open,
                                        polygon.infillIntensity);
        }
    }
}

void FrameRenderer::wheelEvent(QWheelEvent *event)
{
    // 计算缩放因子
    impl->scaleFactor += event->angleDelta().ry() / 300.0f;
    impl->scaleFactor = qMax(1.f, qMin(impl->scaleFactor, 10.0f)); // 限制缩放因子在 [0.1, 10] 范围内
    qDebug() << "scaleFactor:" << impl->scaleFactor;

    // 计算像素等效纹理尺寸
    QSize textureSize(impl->cameraTexture->width(), impl->cameraTexture->height());
    auto baseFrameRect = render_utility::getAutoFitFrameRect(textureSize, this->size()); // 1.0倍缩放时的frame

    // 获取鼠标的位置及不动点的纹理坐标
    QPoint mousePos = event->position().toPoint();
    render_utility::GLSL_Vec2 mouseGLSL, fixedPoint;
    mouseGLSL.x = (float)mousePos.x() / (float)this->size().width();
    mouseGLSL.y = 1.f - (float)mousePos.y() / (float)this->size().height();
    qDebug() << "mouseGLSL: x = " << mouseGLSL.x << " y = " << mouseGLSL.y;

    fixedPoint.x = impl->frame.min_x + impl->frame.width() * mouseGLSL.x;
    fixedPoint.y = impl->frame.min_y + impl->frame.height() * mouseGLSL.y;
    qDebug() << "fixedPoint: x = " << fixedPoint.x << " y = " << fixedPoint.y;

    // 保持不动点，缩放周围的区域
    render_utility::GLSL_Rect newFrame;
    newFrame.min_x = fixedPoint.x - baseFrameRect.width() / impl->scaleFactor * mouseGLSL.x;
    newFrame.max_x = fixedPoint.x + baseFrameRect.width() / impl->scaleFactor * (1.f - mouseGLSL.x);
    newFrame.min_y = fixedPoint.y - baseFrameRect.height() / impl->scaleFactor * mouseGLSL.y;
    newFrame.max_y = fixedPoint.y + baseFrameRect.height() / impl->scaleFactor * (1.f - mouseGLSL.y);
    qDebug() << "newFrame: min_x = " << newFrame.min_x << " min_y = " << newFrame.min_y << " max_x = " << newFrame.max_x << " max_y = " << newFrame.max_y;

    // 更新impl
    impl->frame = newFrame;

    // 更新shader
    impl->shaderProgram->bind();
    impl->shaderProgram->setUniformValue("canvasBoundary",
                                         impl->frame.min_x,  // left
                                         impl->frame.max_x,  // right
                                         impl->frame.min_y,  // bottom
                                         impl->frame.max_y); // top
    impl->shaderProgram->release();

    // 更新窗口
    update();
}

// 更新帧 (Direct模式)
void FrameRenderer::onFrameChangedDirectMode(const unsigned char *data, int width, int height, int channels)
{
    bool needAutoFit = false;

    static bool firstUpdateFromCamera = true;

    // 检查是否size一致
    if (!impl->cameraTexture || QSize(impl->cameraTexture->width(), impl->cameraTexture->height()) != QSize(width, height) || firstUpdateFromCamera)
    {
        qDebug() << "FrameRenderer -> Recreate Texture";

        // 删除旧的纹理
        delete impl->cameraTexture;

        // 创建一个新的纹理
        impl->cameraTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
        impl->cameraTexture->setSize(width, height);
        impl->cameraTexture->setFormat(QOpenGLTexture::RGBA8_UNorm);
        impl->cameraTexture->setWrapMode(QOpenGLTexture::ClampToBorder);
        impl->cameraTexture->setBorderColor(QColor(Qt::black));
        impl->cameraTexture->allocateStorage();
        impl->cameraTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        impl->cameraTexture->setMagnificationFilter(QOpenGLTexture::Linear);

        needAutoFit = true;
        firstUpdateFromCamera = false;
    }

    // 更新纹理
    updateOpenGLTexture(impl->cameraTexture->textureId(), width, height, data, channels);
    qDebug() << "FrameRenderer::onFrameChangedDirectMode";

    // 自适应大小
    needAutoFit ? onAutoFit() : void();
}

void FrameRenderer::onFrameChangedFromCamera(lzx::ICamera *camera)
{
}

void FrameRenderer::onAutoFit()
{
    // 更新impl
    QSize textureSize = QSize(impl->cameraTexture->width(), impl->cameraTexture->height());

    // 考虑DPI问题
    QSize actualSize = this->size() * this->devicePixelRatio();
    qDebug() << "pixelRatio:" << this->devicePixelRatio();

    impl->frame = render_utility::getAutoFitFrameRect(textureSize, actualSize);
    qDebug() << "frame: min_x = " << impl->frame.min_x << " min_y = " << impl->frame.min_y << " max_x = " << impl->frame.max_x << " max_y = " << impl->frame.max_y;
    impl->scaleFactor = 1.0f;

    // 更新shader参数 canvasBoundary
    impl->shaderProgram->bind();
    impl->shaderProgram->setUniformValue("canvasBoundary",
                                         impl->frame.min_x,  // left
                                         impl->frame.max_x,  // right
                                         impl->frame.min_y,  // bottom
                                         impl->frame.max_y); // top
    impl->shaderProgram->release();

    // 更新窗口
    update();
}

void FrameRenderer::onModeChanged(FrameRenderer::Mode mode)
{
    if (mode == Mode::Normal)
    {
        currentMode = Mode::Normal;
        update();
    }
    else if (mode == Mode::DrawLines)
    {
        currentMode = Mode::DrawLines;
        Log::info("Enter DrawLines Mode");
        // 更新
        update();
    }
    else
    {
    }
}

void FrameRenderer::enterDrawMode(bool enter)
{
    if (enter)
    {
        onModeChanged(Mode::DrawLines);
    }
    else
    {
        onModeChanged(Mode::Normal);
    }
}

void FrameRenderer::clearMask()
{
    impl->polygons.clear();
    update();
}

void FrameRenderer::onEnableUpdate(bool enable)
{
    if (enable)
    {
        enableUpdate = true;
        updateTimer->start(10); // 开启定时器
    }
    else
    {
        enableUpdate = false;
    }
}

void FrameRenderer::onFrameChanged(const QImage &frame)
{
    bool needAutoFit = false;

    // 将 QImage 转换为 OpenGL 可以使用的格式
    QImage glImage = frame.convertToFormat(QImage::Format_RGBA8888).mirrored(false, true);

    // 检查 QImage 的大小是否改变
    if (!impl->cameraTexture || QSize(impl->cameraTexture->width(), impl->cameraTexture->height()) != glImage.size())
    {
        // 删除旧的纹理
        delete impl->cameraTexture;

        // 创建一个新的纹理
        impl->cameraTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
        impl->cameraTexture->setSize(glImage.width(), glImage.height());
        impl->cameraTexture->setFormat(QOpenGLTexture::RGBA8_UNorm);
        impl->cameraTexture->setWrapMode(QOpenGLTexture::ClampToBorder);
        impl->cameraTexture->setBorderColor(QColor(Qt::black));
        impl->cameraTexture->allocateStorage();
        impl->cameraTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        impl->cameraTexture->setMagnificationFilter(QOpenGLTexture::Linear);

        needAutoFit = true;
        // 将 QImage 上传到 GPU
        impl->cameraTexture->setData(0, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, glImage.constBits());
    }
    else
    {
        // 使用子图像方式上传到GPU
        impl->cameraTexture->setData(0, 0, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, glImage.constBits());
    }

    // 自适应大小
    needAutoFit ? onAutoFit() : void();

    // 请求重绘
    update();
}

void FrameRenderer::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (currentMode == Mode::Normal)
    {
    }
    else if (currentMode == Mode::DrawLines)
    {
        // 左键双击
        if (event->button() == Qt::LeftButton)
        {
            // 如果点的数量大于等于3，那么闭合当前多边形
            if (impl->polygons.size() && impl->polygons.back().pointsInTexture.size() >= 3)
            {
                impl->polygons.back().isClosed = true;

                // 弹出对话框，输入填充强度 0 - 255
                bool ok;
                int infillIntensity = QInputDialog::getInt(this, tr("输入填充强度"),
                                                           tr("填充强度:"), 255, 0, 255, 1, &ok);
                if (ok && impl->polygons.size())
                {
                    impl->polygons.back().infillIntensity = infillIntensity / 255.0f;
                }

                // 创建新的多边形
                impl->polygons.push_back(PolygonInfo());

                // 更新
                update();
            }
        }
    }
    else
    {
    }
}

void FrameRenderer::mousePressEvent(QMouseEvent *event)
{
    if (currentMode == Mode::Normal)
    {
        if (event->button() == Qt::LeftButton)
        {
        }
        else if (event->button() == Qt::RightButton)
        {
        }
        else if (event->button() == Qt::MiddleButton)
        {

            impl->isMoving = true;
            impl->lastPos = event->pos();
        }
        else
        {
        }
    }
    else if (currentMode == Mode::DrawLines)
    {

        if (event->button() == Qt::LeftButton)
        {
            QPoint mousePos = event->pos();
            QPointF mousePosUniform = {(float)mousePos.x() / (float)this->size().width(),
                                       (float)mousePos.y() / (float)this->size().height()};
            auto &rectViewport = impl->frame;
            QPointF mousePosTexture;
            mousePosTexture.setX(rectViewport.min_x + mousePosUniform.x() * rectViewport.width());
            mousePosTexture.setY(rectViewport.min_y + (1.0 - mousePosUniform.y()) * rectViewport.height());
            // 记录点
            if (impl->polygons.size())
            {
                impl->polygons.back().pointsInTexture.push_back(QVector2D(mousePosTexture.x(), mousePosTexture.y()));
                update();
            }
            else
            {
                // 创建新的多边形
                impl->polygons.push_back(PolygonInfo());
                impl->polygons.back().pointsInTexture.push_back(QVector2D(mousePosTexture.x(), mousePosTexture.y()));
            }
        }
        else if (event->button() == Qt::MiddleButton)
        {

            impl->isMoving = true;
            impl->lastPos = event->pos();

            update();
        }
    }
    else
    {
    }
}

// mouseMoveEvent
void FrameRenderer::mouseMoveEvent(QMouseEvent *event)
{

    if (currentMode == Mode::Normal)
    {
        // 中键拖拽图像
        if (event->buttons() & Qt::MiddleButton)
        {
            if (impl->isMoving)
            {

                // 计算鼠标移动的距离
                QPoint mouseDelta = event->pos() - impl->lastPos;

                // 计算frame的移动
                render_utility::GLSL_Vec2 deltaGLSL;
                deltaGLSL.x = -mouseDelta.x() / (float)this->size().width() * impl->frame.width();
                deltaGLSL.y = mouseDelta.y() / (float)this->size().height() * impl->frame.height();

                // 更新impl
                impl->frame.translate(deltaGLSL.x, deltaGLSL.y);
                impl->lastPos = event->pos();

                // 更新shader
                impl->shaderProgram->bind();
                impl->shaderProgram->setUniformValue("canvasBoundary",
                                                     impl->frame.min_x,  // left
                                                     impl->frame.max_x,  // right
                                                     impl->frame.min_y,  // bottom
                                                     impl->frame.max_y); // top
                impl->shaderProgram->release();

                // 更新窗口
                update();
            }
        }
    }
    else if (currentMode == Mode::DrawLines)
    {
        qDebug() << "mouseMoveEvent in Draw lines mode " << event->pos().x() << " " << event->pos().y();

        // 中键拖拽图像
        if (event->buttons() & Qt::MiddleButton)
        {
            if (impl->isMoving)
            {

                // 计算鼠标移动的距离
                QPoint mouseDelta = event->pos() - impl->lastPos;

                // 计算frame的移动
                render_utility::GLSL_Vec2 deltaGLSL;
                deltaGLSL.x = -mouseDelta.x() / (float)this->size().width() * impl->frame.width();
                deltaGLSL.y = mouseDelta.y() / (float)this->size().height() * impl->frame.height();

                // 更新impl
                impl->frame.translate(deltaGLSL.x, deltaGLSL.y);
                impl->lastPos = event->pos();

                // 更新shader
                impl->shaderProgram->bind();
                impl->shaderProgram->setUniformValue("canvasBoundary",
                                                     impl->frame.min_x,  // left
                                                     impl->frame.max_x,  // right
                                                     impl->frame.min_y,  // bottom
                                                     impl->frame.max_y); // top
                impl->shaderProgram->release();

                // 更新窗口
                update();
            }
        }
        else
        {
            // 计算当前point 替换最后一个point
            QPoint mousePos = event->pos();
            QPointF mousePosUniform = {(float)mousePos.x() / (float)this->size().width(),
                                       (float)mousePos.y() / (float)this->size().height()};
            auto &rectViewport = impl->frame;
            QPointF mousePosTexture;
            mousePosTexture.setX(rectViewport.min_x + mousePosUniform.x() * rectViewport.width());
            mousePosTexture.setY(rectViewport.min_y + (1.0 - mousePosUniform.y()) * rectViewport.height());
            // 替换最后一个点
            if (impl->polygons.size() && impl->polygons.back().pointsInTexture.size() > 0)
            {
                // 检查是否只有一个点，如果是，那么添加一个点，否则替换最后一个点
                if (impl->polygons.back().pointsInTexture.size() == 1)
                {
                    impl->polygons.back().pointsInTexture.push_back(QVector2D(mousePosTexture.x(), mousePosTexture.y()));
                }
                else
                {
                    impl->polygons.back().pointsInTexture.back() = QVector2D(mousePosTexture.x(), mousePosTexture.y());
                }
            }

            update();
        }
    }
    else
    {
    }
}

// mouseReleaseEvent
void FrameRenderer::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
    }
    else if (event->button() == Qt::RightButton)
    {
    }
    else if (event->button() == Qt::MiddleButton)
    {
        impl->isMoving = false;
    }
    else
    {
    }
}

void FrameRenderer::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu contextMenu(this);

    QAction *autoFitAction = new QAction("自适应大小", this);
    connect(autoFitAction, &QAction::triggered, this, &FrameRenderer::onAutoFit);
    contextMenu.addAction(autoFitAction);

    contextMenu.exec(event->globalPos());
}

// resizeGL
void FrameRenderer::resizeGL(int width, int height)
{

    qreal pixelRatio = this->devicePixelRatio();

    int actualWidth = width * pixelRatio;
    int actualHeight = height * pixelRatio;

    glViewport(0, 0, actualWidth, actualHeight);

    // 保持中间点不动，更新frame
    float widthRatio = (float)actualWidth / (float)impl->lastSize.width();
    float heightRatio = (float)actualHeight / (float)impl->lastSize.height();
    impl->frame.scaleAtFixedPoint(0.5f, 0.5f, widthRatio, heightRatio);

    // 更新shader
    impl->shaderProgram->bind();
    impl->shaderProgram->setUniformValue("canvasBoundary",
                                         impl->frame.min_x,  // left
                                         impl->frame.max_x,  // right
                                         impl->frame.min_y,  // bottom
                                         impl->frame.max_y); // top

    impl->shaderProgram->release();

    // 更新impl
    impl->lastSize = QSize(actualWidth, actualHeight);

    // 更新窗口
    update();
}

// 更新纹理内容
void FrameRenderer::updateOpenGLTexture(GLuint textureID, int width, int height, const GLubyte *data, int bytesPerPixel)
{
    GLenum availFormats[] = {GL_RED, GL_RG, GL_RGB, GL_RGBA};
    GLenum format = availFormats[bytesPerPixel - 1];
    glBindTexture(GL_TEXTURE_2D, textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, (bytesPerPixel == 4) ? 4 : 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
}
