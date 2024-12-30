#pragma once

#include <QWidget>
#include <QDoubleSpinBox>
#include <QVector>

#include "qcustomplot.h"

class GrayMappingWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GrayMappingWidget(QWidget *parent = nullptr);

    void setHistogram(const std::vector<int> &histogram, int maxValue);

signals:
    void mappingChanged(const QVector<double> &lut);
    void rangeChanged(double min, double max);
    void gammaChanged(double gamma);

private:
    const int LUT_SIZE = 360;
    // UI elements
    QCustomPlot *plotWidget; // 用于显示曲线
    QDoubleSpinBox *minSpinBox;
    QDoubleSpinBox *maxSpinBox;
    QDoubleSpinBox *gammaSpinBox;

    QCPItemLine *blackLine;           // 黑点控制线
    QCPItemLine *whiteLine;           // 白点控制线
    QCPItemLine *currentDraggingLine; // 当前正在拖动的线

    // Data
    QVector<double> m_histogram; // 存储直方图数据
    QVector<double> m_lutCurve;  // 存储LUT曲线数据

    void setupUI();
    void updateLutCurve(bool fromDrag = false);
    void updatePlot();

    void onMousePress(QMouseEvent *event);
    void onMouseMove(QMouseEvent *event);
    void onMouseRelease(QMouseEvent *event);
    void onMouseHover(QMouseEvent *event);

private slots:
    void handleMinValueChanged();
    void handleMaxValueChanged();
};
