#pragma once

#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLFunctions_3_3_Core>
#include <QDebug>

#include "Common.h"

class ImageRenderer : protected QOpenGLFunctions_3_3_Core
{
public:
    ImageRenderer() {}
    ~ImageRenderer()
    {
        vao.destroy();
        vbo.destroy();
        delete texture;
    }

    void initialize(QOpenGLFunctions_3_3_Core *f)
    {
        glFuncs = f;
        initializeOpenGLFunctions();

        initShaders();
        initGeometry();
        initTexture();
        generateTransferFuntionTexture(TransferFunction());
    }

    void draw(bool inverse, TransferFunction tf, float rotation, const QVector2D &translation, bool flipHorizontal, bool flipVertical, int lumOffset)
    {
        qDebug() << "ImageRenderer::draw()";

        static TransferFunction lastTF;

        // 检查transfer function是否发生变化
        if (!(tf == lastTF))
        {
            qDebug() << "Transfer function changed, Regenerating gamma correction texture.";
            lastTF = tf;
            generateTransferFuntionTexture(tf);
        }

        // 更新纹理
        updateTextureFromCamera();

        shaderProgram.bind();

        // 设置颜色取反的uniform变量
        shaderProgram.setUniformValue("inverse", inverse);
        shaderProgram.setUniformValue("rotation", rotation);
        shaderProgram.setUniformValue("translation", translation);
        shaderProgram.setUniformValue("flipHorizontal", flipHorizontal);
        shaderProgram.setUniformValue("flipVertical", flipVertical);
        shaderProgram.setUniformValue("lumOffset", lumOffset);

        // 绑定纹理
        glFuncs->glActiveTexture(GL_TEXTURE0);
        texture->bind();

        // 绑定 gamma 校正的映射纹理
        glFuncs->glActiveTexture(GL_TEXTURE1);
        gammaCorrectionTexture->bind();
        shaderProgram.setUniformValue("gammaCorrectionTexture", 1);

        vao.bind();
        glFuncs->glDrawArrays(GL_TRIANGLES, 0, 6); // 绘制一个三角形
        vao.release();

        shaderProgram.release();
    }

private:
    QOpenGLShaderProgram shaderProgram;
    QOpenGLBuffer vbo;
    QOpenGLVertexArrayObject vao;
    QOpenGLTexture *texture = nullptr;
    QOpenGLTexture *gammaCorrectionTexture = nullptr; // 伽马校正纹理
    float gamma = 2.0f;                               // 伽马值
    QOpenGLFunctions_3_3_Core *glFuncs = nullptr;
    float aspect = 1024.0f / 768.0f;

    void initShaders()
    {
        shaderProgram.addShaderFromSourceCode(QOpenGLShader::Vertex,
                                              "#version 330 core\n"
                                              "layout (location = 0) in vec2 position;\n"
                                              "layout (location = 1) in vec2 texCoords;\n"
                                              "out vec2 TexCoords;\n"
                                              "uniform float rotation;\n"
                                              "uniform vec2 translation;\n"
                                              "uniform bool flipHorizontal;\n"
                                              "uniform bool flipVertical;\n"
                                              "uniform mat4 projection;\n" // 添加投影矩阵的uniform变量
                                              "void main() {\n"
                                              "   vec2 pos = position;\n"
                                              "   float cosRot = cos(rotation);\n"
                                              "   float sinRot = sin(rotation);\n"
                                              "   mat2 rotMat = mat2(cosRot, -sinRot, sinRot, cosRot);\n"
                                              "   pos = rotMat * pos; // 进行旋转\n"
                                              "   pos = pos + translation;\n"
                                              "   pos.x = flipHorizontal ? -pos.x : pos.x;\n"
                                              "   pos.y = flipVertical ? -pos.y : pos.y;\n"
                                              "   gl_Position = projection * vec4(pos, 0.0, 1.0); // 使用投影矩阵进行变换\n"
                                              "   TexCoords = texCoords;\n"
                                              "}");

        shaderProgram.addShaderFromSourceCode(QOpenGLShader::Fragment,
                                              "#version 330 core\n"
                                              "in vec2 TexCoords;\n"
                                              "out vec4 FragColor;\n"
                                              "uniform sampler2D texture1;\n"
                                              "uniform sampler1D gammaCorrectionTexture;\n"
                                              "uniform bool inverse;\n"
                                              "uniform int lumOffset;\n"
                                              "void main() {\n"
                                              "   FragColor = texture(texture1, TexCoords);\n"
                                              "   FragColor.g=FragColor.r;\n"
                                              "   FragColor.b=FragColor.r;\n"
                                              "   float colorGammaCorrected = texture(gammaCorrectionTexture, FragColor.r).r;\n"
                                              "   if (inverse) {\n"
                                              "       colorGammaCorrected = 1.0 - colorGammaCorrected;\n"
                                              "   }\n"
                                              "   colorGammaCorrected = colorGammaCorrected + lumOffset / 255.f;\n"
                                              "   FragColor = vec4(colorGammaCorrected, colorGammaCorrected, colorGammaCorrected, 1.0);\n"
                                              "}");

        shaderProgram.link();

        // 设置一些不会改变的 uniform 变量
        shaderProgram.bind();
        // 设置投影矩阵的uniform变量
        QMatrix4x4 projection;
        projection.ortho(-1.0f, 1.0f, -1.0f / aspect, 1.0f / aspect, -1.0f, 1.0f);
        shaderProgram.setUniformValue("projection", projection);
    }

    void initGeometry()
    {
        vao.create();
        vbo.create();

        vao.bind();
        vbo.bind();

        GLfloat vertices[] = {
            // 位置       // 纹理坐标
            // 三角形 左下
            -1.0f, 1.f / aspect, 0.f, 1.0f,    // 左上角
            -1.0f, -1.0f / aspect, 0.0f, 0.0f, // 左下角
            1.0f, -1.0f / aspect, 1.0f, 0.0f,  // 右下角

            // 三角形 右上
            -1.0f, 1.0f / aspect, 0.0f, 1.0f, // 左上角
            1.0f, -1.0f / aspect, 1.0f, 0.0f, // 右下角
            1.0f, 1.0f / aspect, 1.0f, 1.0f   // 右上角
        };

        vbo.allocate(vertices, sizeof(vertices));

        glFuncs->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *)0);
        glFuncs->glEnableVertexAttribArray(0);

        glFuncs->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *)(2 * sizeof(GLfloat)));
        glFuncs->glEnableVertexAttribArray(1);

        vao.release();
    }

    void initTexture()
    {
        texture = new QOpenGLTexture(QImage(":/placeholder.png").mirrored());
        texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        texture->setMagnificationFilter(QOpenGLTexture::Linear);
    }

    void generateTransferFuntionTexture(TransferFunction tf)
    {
        // 如果纹理已经存在，则删除它
        if (gammaCorrectionTexture)
        {
            delete gammaCorrectionTexture;
        }

        // 创建一个新的 1D 纹理
        gammaCorrectionTexture = new QOpenGLTexture(QOpenGLTexture::Target1D);
        gammaCorrectionTexture->create();

        // 设置纹理参数
        gammaCorrectionTexture->setMinificationFilter(QOpenGLTexture::Linear);
        gammaCorrectionTexture->setMagnificationFilter(QOpenGLTexture::Linear);
        gammaCorrectionTexture->setWrapMode(QOpenGLTexture::ClampToEdge);

        // 生成映射表数据
        const int lutSize = 256;
        unsigned char lutData[lutSize];
        for (int i = 0; i < lutSize; ++i)
        {
            float colorNorm = static_cast<float>(i) / (lutSize - 1);

            // min 和 max 之间作gamma校正， 然后乘以 intensity，小于min的值设为0，大于max的值设为1
            if (colorNorm < tf.min + 0.0001)
            {
                lutData[i] = 0;
                continue;
            }

            if (colorNorm > tf.max - 0.0001)
            {
                lutData[i] = 255;
                continue;
            }

            float colorGammaCorrected = std::pow((colorNorm - tf.min) / (tf.max - tf.min), tf.gamma) * tf.intensity;

            // 保证值在0-1之间
            colorGammaCorrected = std::max(0.0f, std::min(1.0f, colorGammaCorrected));

            lutData[i] = static_cast<unsigned char>(colorGammaCorrected * 255);
        }

        // 将数据上传到 GPU
        gammaCorrectionTexture->setSize(lutSize);
        gammaCorrectionTexture->setFormat(QOpenGLTexture::R8_UNorm);
        gammaCorrectionTexture->allocateStorage();
        gammaCorrectionTexture->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, lutData);
    }

    void updateTextureFromCamera();

    void updateOpenGLTexture(GLuint textureID, int width, int height, const GLubyte *data, int bytesPerPixel);
};