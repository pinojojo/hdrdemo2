#include "GrayMappingWidget.h"

#include <QDebug>

// GrayMappingWidget.cpp
GrayMappingWidget::GrayMappingWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();

    updateLutCurve();

    connect(minSpinBox, &QDoubleSpinBox::editingFinished,
            this, &GrayMappingWidget::handleMinValueChanged);
    connect(maxSpinBox, &QDoubleSpinBox::editingFinished,
            this, &GrayMappingWidget::handleMaxValueChanged);
    connect(gammaSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &GrayMappingWidget::updateLutCurve);

    connect(plotWidget, &QCustomPlot::mousePress, this, &GrayMappingWidget::onMousePress);
    connect(plotWidget, &QCustomPlot::mouseMove, this, &GrayMappingWidget::onMouseMove);
    connect(plotWidget, &QCustomPlot::mouseRelease, this, &GrayMappingWidget::onMouseRelease);
}

void GrayMappingWidget::setHistogram(const std::vector<int> &histogram, int maxValue)
{

    if (maxValue != m_maxValue)
    {
        m_maxValue = maxValue;

        minSpinBox->setRange(0, m_maxValue);
        maxSpinBox->setRange(0, m_maxValue);

        blackLine->start->setCoords(0, 0);
        blackLine->end->setCoords(0, 255);

        whiteLine->start->setCoords(m_maxValue, 0);
        whiteLine->end->setCoords(m_maxValue, 255);

        updateLutCurve();
    }

    int histogramPeak = *std::max_element(histogram.begin(), histogram.end());

    this->m_histogram.clear();

    if (m_histogram.size() != histogram.size())
        this->m_histogram.resize(histogram.size());

    for (int i = 0; i < m_histogram.size(); i++)
    {
        this->m_histogram[i] = (histogram[i] / (double)histogramPeak * 255); // 归一化到0-255,因为绘制的时候y轴是0-255
    }

    updatePlot();
}

void GrayMappingWidget::setupUI()
{
    auto layout = new QVBoxLayout(this);

    layout->setContentsMargins(0, 0, 0, 0);

    // 创建绘图区域
    plotWidget = new QCustomPlot(this);
    plotWidget->setMinimumHeight(150);

    plotWidget->xAxis->setRange(0, m_maxValue);
    plotWidget->yAxis->setRange(0, 255);
    plotWidget->setBackground(QBrush(QColor(31, 33, 39)));

    QColor axisColor(240, 240, 240);                      // 乳白色
    QColor subTickColor = QColor(140, 140, 140);          // 灰色
    plotWidget->xAxis->setTickLabelColor(axisColor);      // 刻度文字颜色
    plotWidget->xAxis->setBasePen(QPen(axisColor));       // 轴线颜色
    plotWidget->xAxis->setTickPen(QPen(axisColor));       // 刻度线颜色
    plotWidget->xAxis->setSubTickPen(QPen(subTickColor)); // 次刻度线颜色
    plotWidget->xAxis->setLabelColor(axisColor);          // 轴标签颜色
    plotWidget->yAxis->setTickLabelColor(axisColor);      // 刻度文字颜色
    plotWidget->yAxis->setBasePen(QPen(axisColor));       // 轴线颜色
    plotWidget->yAxis->setTickPen(QPen(axisColor));       // 刻度线颜色
    plotWidget->yAxis->setSubTickPen(QPen(subTickColor)); // 次刻度线颜色
    plotWidget->yAxis->setLabelColor(axisColor);          // 轴标签颜色

    // 如果有网格线，也可以设置网格线颜色
    plotWidget->xAxis->grid()->setPen(QPen(QColor(140, 140, 140, 128), .5, Qt::DashLine));
    plotWidget->yAxis->grid()->setPen(QPen(QColor(140, 140, 140, 128), .5, Qt::DashLine));

    // 两条线，调整黑白点
    {
        blackLine = new QCPItemLine(plotWidget);
        whiteLine = new QCPItemLine(plotWidget);

        // 设置线条样式
        QPen linePen(QColor(255, 255, 255, 100)), selectedLinePen(QColor(255, 255, 255, 100));
        linePen.setWidth(2);
        selectedLinePen.setWidth(2);

        blackLine->setPen(linePen);
        blackLine->setSelectedPen(selectedLinePen);
        whiteLine->setPen(linePen);
        whiteLine->setSelectedPen(selectedLinePen);

        // 设置初始位置（垂直线，从底部到顶部）
        blackLine->start->setCoords(0, 0); // 底部点
        blackLine->end->setCoords(0, 255); // 顶部点

        whiteLine->start->setCoords(m_maxValue, 0);
        whiteLine->end->setCoords(m_maxValue, 255);

        blackLine->setSelectable(true);
        whiteLine->setSelectable(true);
    }

    // 允许选择和拖动
    plotWidget->setInteractions(QCP::iSelectItems | QCP::iMultiSelect);

    // 创建控制面板
    auto controlPanel = new QHBoxLayout;

    // 最小值控制
    minSpinBox = new QDoubleSpinBox(this);
    minSpinBox->setRange(0, m_maxValue);
    minSpinBox->setValue(0);
    minSpinBox->setDecimals(0);

    // 最大值控制
    maxSpinBox = new QDoubleSpinBox(this);
    maxSpinBox->setRange(0, m_maxValue);
    maxSpinBox->setValue(m_maxValue);
    maxSpinBox->setDecimals(0);

    // Gamma值控制
    gammaSpinBox = new QDoubleSpinBox(this);
    gammaSpinBox->setRange(0.1, 5.0);
    gammaSpinBox->setValue(1.0);
    gammaSpinBox->setSingleStep(0.1);
    gammaSpinBox->setDecimals(2);

    controlPanel->addWidget(new QLabel("Black:"));
    controlPanel->addWidget(minSpinBox);
    controlPanel->addWidget(new QLabel("White:"));
    controlPanel->addWidget(maxSpinBox);
    controlPanel->addWidget(new QLabel("Gamma:"));
    controlPanel->addWidget(gammaSpinBox);

    layout->addWidget(plotWidget);
    layout->addLayout(controlPanel);
}

void GrayMappingWidget::onMousePress(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {

        // 获取鼠标位置
        double x = plotWidget->xAxis->pixelToCoord(event->pos().x());
        double y = plotWidget->yAxis->pixelToCoord(event->pos().y());

        // 检查是否点击到了某条线附近
        currentDraggingLine = nullptr;
        double threshold = m_maxValue * 0.02;

        if (qAbs(x - blackLine->start->key()) < threshold)
        {

            currentDraggingLine = blackLine;
        }
        else if (qAbs(x - whiteLine->start->key()) < threshold)
        {

            currentDraggingLine = whiteLine;
        }

        if (currentDraggingLine)
        {
            plotWidget->setCursor(Qt::SizeHorCursor);
        }
    }
}

void GrayMappingWidget::onMouseMove(QMouseEvent *event)
{
    if (currentDraggingLine && event->buttons() & Qt::LeftButton)
    {
        double x = plotWidget->xAxis->pixelToCoord(event->pos().x());
        x = qBound(0.0, x, (double)m_maxValue);

        // 更新线条位置
        currentDraggingLine->start->setCoords(x, 0);
        currentDraggingLine->end->setCoords(x, 255);

        // 确保黑点在白点左边
        if (currentDraggingLine == blackLine)
        {
            x = qMin(x, whiteLine->start->key() - 1);
            x = qRound(x);
            minSpinBox->setValue(x);
        }
        else if (currentDraggingLine == whiteLine)
        {
            x = qMax(x, blackLine->start->key() + 1);
            x = qRound(x);
            maxSpinBox->setValue(x);
        }

        updateLutCurve(true);
        // plotWidget->replot();
    }
}

void GrayMappingWidget::onMouseRelease(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && currentDraggingLine)
    {
        currentDraggingLine = nullptr;
        plotWidget->setCursor(Qt::ArrowCursor);

        emit lutChanged(minSpinBox->value(), maxSpinBox->value(), gammaSpinBox->value());
    }
}

void GrayMappingWidget::handleMinValueChanged()
{
    // 确保在0到max之间
    double val = minSpinBox->value();
    minSpinBox->setValue(qBound(0.0, val, maxSpinBox->value() - 1));

    updateLutCurve();

    emit lutChanged(minSpinBox->value(), maxSpinBox->value(), gammaSpinBox->value());
}

void GrayMappingWidget::handleMaxValueChanged()
{
    double val = maxSpinBox->value();
    maxSpinBox->setValue(qBound(minSpinBox->value() + 1, val, (double)m_maxValue));

    updateLutCurve();

    emit lutChanged(minSpinBox->value(), maxSpinBox->value(), gammaSpinBox->value());
}

void GrayMappingWidget::handleGammaValueChanged()
{
    emit lutChanged(minSpinBox->value(), maxSpinBox->value(), gammaSpinBox->value());
}

void GrayMappingWidget::updateLutCurve(bool fromDrag)
{

    if (!fromDrag)
    {

        if (minSpinBox->value() > maxSpinBox->value())
        {
            return;
        }

        if (minSpinBox->value() < 0)
        {
            minSpinBox->setValue(0);
        }

        if (maxSpinBox->value() > m_maxValue)
        {
            maxSpinBox->setValue(m_maxValue);
        }

        blackLine->start->setCoords(minSpinBox->value(), 0);
        blackLine->end->setCoords(minSpinBox->value(), 255);
        whiteLine->start->setCoords(maxSpinBox->value(), 0);
        whiteLine->end->setCoords(maxSpinBox->value(), 255);
    }

    double min = minSpinBox->value();
    double max = maxSpinBox->value();

    double minNorm = min / m_maxValue;
    double maxNorm = max / m_maxValue;

    double gamma = gammaSpinBox->value();

    // 计算LUT曲线
    m_lutCurve.resize(LUT_SIZE);
    for (int i = 0; i < LUT_SIZE; ++i)
    {
        double xNorm = i / 359.0;
        if (xNorm < minNorm)
        {
            m_lutCurve[i] = 0;
        }
        else if (xNorm > maxNorm)
        {
            m_lutCurve[i] = 255;
        }
        else
        {
            m_lutCurve[i] = 255 * qPow((xNorm - minNorm) / (maxNorm - minNorm), 1.0 / gamma);
        }
    }

    updatePlot();

    emit lutChanged(minSpinBox->value(), maxSpinBox->value(), gammaSpinBox->value());
}

void GrayMappingWidget::updatePlot()
{
    plotWidget->clearGraphs();

    // 绘制直方图
    {
        auto histGraph = plotWidget->addGraph();
        histGraph->setLineStyle(QCPGraph::lsLine);
        QColor histColor = QColor(31, 53, 101, 200);
        histGraph->setPen(QPen(histColor.lighter(200)));
        histGraph->setBrush(QBrush(histColor));

        if (m_histogram.size())
        {
            QVector<double> x(m_histogram.size()), y(m_histogram.size());
            for (int i = 0; i < m_histogram.size(); ++i)
            {
                x[i] = i / (m_histogram.size() - 1.0) * m_maxValue;
                y[i] = m_histogram[i];
            }
            histGraph->setData(x, y);
        }
    }

    // 添加LUT曲线

    {
        auto lutGraph = plotWidget->addGraph();

        QVector<double> x(LUT_SIZE), y(LUT_SIZE);
        for (int i = 0; i < LUT_SIZE; ++i)
        {
            x[i] = i / (LUT_SIZE - 1.0) * m_maxValue;
            y[i] = m_lutCurve[i];
        }
        lutGraph->setData(x, y);

        lutGraph->setPen(QPen(QColor(80, 134, 255), 2));
    }

    plotWidget->replot();
}
