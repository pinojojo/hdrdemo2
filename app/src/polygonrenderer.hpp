#pragma once

#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions_3_3_Core> // 使用 OpenGL 3.3 Core 版本
#include <QContextMenuEvent>

#include <tuple>
#include <array>
#include "earcut.hpp"

// 用于绘制多边形（多种模式）
class PolygonRenderer : protected QOpenGLFunctions_3_3_Core
{
public:
    PolygonRenderer() {}
    ~PolygonRenderer()
    {
        vao.destroy();
        vbo.destroy();
    }

    enum class Mode
    {
        Open,   // 绘制未闭合多边形（连线模式）
        Closed, // 绘制闭合的多边形（连线模式，但是会连接第一个和最后一个顶点）
        Filler, // 绘制填充的多边形（基于 Earcut 算法）
    };

    void initialize(QOpenGLFunctions_3_3_Core *f)
    {
        glFuncs = f;
        initializeOpenGLFunctions();

        initShaders();
        initGeometry();
    }

    void draw(const std::vector<QVector2D> &vertices, Mode mode = Mode::Open, float intensity = 1.0f)
    {
        updateVertices(vertices, mode);

        shaderProgram.bind();
        shaderProgram.setUniformValue("intensity", intensity);

        vao.bind();

        if (mode == Mode::Filler)
        {
            shaderProgram.setUniformValue("drawing", false);
            glFuncs->glDrawArrays(GL_TRIANGLES, 0, vertexCount); // 绘制三角形填充多边形
        }
        else if (mode == Mode::Closed && vertexCount >= 3)
        {
            shaderProgram.setUniformValue("drawing", true);
            glFuncs->glDrawArrays(GL_LINE_LOOP, 0, vertexCount); // 绘制闭合多边形
        }
        else
        {
            shaderProgram.setUniformValue("drawing", true);
            glFuncs->glDrawArrays(GL_LINE_STRIP, 0, vertexCount); // 绘制开放多边形
        }

        vao.release();
        shaderProgram.release();
    }

private:
    QOpenGLShaderProgram shaderProgram;
    QOpenGLBuffer vbo;
    QOpenGLVertexArrayObject vao;
    QOpenGLFunctions_3_3_Core *glFuncs = nullptr;
    GLsizei vertexCount = 0; // 顶点的数量

    void initShaders()
    {
        // Basic shader for drawing lines
        shaderProgram.addShaderFromSourceCode(QOpenGLShader::Vertex,
                                              "#version 330 core\n"
                                              "layout (location = 0) in vec2 position;\n"
                                              "void main() {\n"
                                              "   gl_Position = vec4(position, 0.0, 1.0);\n"
                                              "}");
        shaderProgram.addShaderFromSourceCode(QOpenGLShader::Fragment,
                                              "#version 330 core\n"
                                              "out vec4 FragColor;\n"
                                              "uniform float intensity;\n"
                                              "uniform bool drawing;\n"
                                              "void main() {\n"
                                              "   if (drawing) {\n"
                                              "       FragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
                                              "   } else {\n"
                                              "   FragColor = vec4( intensity, intensity, intensity, 1.0);\n"
                                              "   }\n"
                                              "}");
        shaderProgram.link();
    }
    void initGeometry()
    {
        // Initial empty buffer setup
        vao.create();
        vbo.create();

        vao.bind();
        vbo.bind();
        vbo.allocate(nullptr, 0); // Start with an empty buffer
        glFuncs->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void *)0);
        glFuncs->glEnableVertexAttribArray(0);
        vao.release();
    }

    void updateVertices(const std::vector<QVector2D> &vertices, Mode mode)
    {
        if (mode == Mode::Filler)
        {
            // 进行三角剖分
            auto triangles = triangulate(vertices);
            updateVertexBuffer(triangles); // 更新 VBO
        }
        else
        {
            // 更新 VBO
            updateVertexBuffer(vertices);
        }
    }

    void updateVertexBuffer(const std::vector<QVector2D> &vertices)
    {
        vertexCount = vertices.size();
        vao.bind();
        vbo.bind();
        vbo.allocate(vertices.data(), vertices.size() * sizeof(QVector2D));
        vao.release();
    }

    std::vector<QVector2D> triangulate(const std::vector<QVector2D> &vertices)
    {
        // Earcut 库所需的顶点格式
        using Coord = double; // Earcut 使用 double 坐标
        using Point = std::array<Coord, 2>;
        using N = uint32_t; // Earcut 使用 uint32_t 作为索引类型

        std::vector<std::vector<Point>> plys = {{}}; // Earcut 需要一个二维数组作为输入，这里只有一个多边形，所以只有一个数组
        for (const auto &v : vertices)
        {
            plys[0].push_back({v.x(), v.y()});
        }

        // 进行三角剖分
        std::vector<N> indices = mapbox::earcut<N>(plys); // Earcut 返回的是索引数组

        // 根据 Earcut 返回的索引构造三角形顶点数组
        std::vector<QVector2D> triangles;
        for (N index : indices)
        {
            triangles.push_back(vertices[index]);
        }

        return triangles;
    }
};
