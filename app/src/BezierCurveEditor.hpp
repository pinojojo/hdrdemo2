#pragma once

#include <QtWidgets>

class BezierCurveEditor : public QWidget
{
    Q_OBJECT

public:
    BezierCurveEditor(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QVector<QPointF> controlPoints; // 存储贝塞尔曲线的控制点
    int selectedPointIndex = -1;    // 当前选中的控制点索引，-1 表示没有选中
    // 其他需要的成员变量
};
