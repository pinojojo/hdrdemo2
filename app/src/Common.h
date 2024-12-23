#pragma once

#include <QVector2D>
#include <vector>

struct MaskWindowProperty
{
    bool visible = false;
    int width = 1024;
    int height = 768;
    int offsetX = 0;
    int offsetY = 0;
};

struct MaskPolygon
{
    std::vector<QVector2D> vertices; // 纹理坐标系下的顶点坐标
    float intensity = 1.0f;
};

struct MaskData
{
    bool continuousMode = false; // 是否为连续模式(相机实时图像作为Mask)

    // 连续模式下
    float gamma = 1.0f; // 伽马值

    // 非连续模式
    std::vector<MaskPolygon> polygons;      // 多边形列表
    float globalBackgroundIntensity = 1.0f; // 全局背景亮度
};

enum class DMDWorkMode
{
    Normal, // 正常模式
    Encoded // 编码模式
};

struct TransferFunction
{
    float min = 0.0f;
    float max = 1.0f;
    float gamma = 1.0f;
    float intensity = 1.0f;
};

// 针对 TransferFunction 重载 等号操作符
inline bool operator==(const TransferFunction &lhs, const TransferFunction &rhs)
{
    // 一个lambda函数，用于比较两个浮点数是否相等
    auto floatEqual = [](float a, float b) -> bool
    {
        return std::abs(a - b) < 1e-6;
    };

    return floatEqual(lhs.min, rhs.min) &&
           floatEqual(lhs.max, rhs.max) &&
           floatEqual(lhs.gamma, rhs.gamma) &&
           floatEqual(lhs.intensity, rhs.intensity);
}