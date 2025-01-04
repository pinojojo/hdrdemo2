// NeonButton.cpp
#include "NeonButton.h"
#include <QPainter>

NeonButton::NeonButton(QWidget *parent) : QPushButton(parent)
{
    m_glowAnimation = new QPropertyAnimation(this, "glowIntensity", this);
    m_glowAnimation->setDuration(2000);
    m_glowAnimation->setLoopCount(-1);
    m_glowAnimation->setStartValue(0.0);
    m_glowAnimation->setEndValue(1.0);
    m_glowAnimation->setEasingCurve(QEasingCurve::InOutBounce);
}

void NeonButton::startGlowing()
{
    m_isGlowing = true;
    m_glowAnimation->start();
}

void NeonButton::stopGlowing()
{
    m_isGlowing = false;
    m_glowAnimation->stop();
    m_glowIntensity = 0.0;
    update();
}

void NeonButton::setGlowIntensity(qreal intensity)
{
    m_glowIntensity = intensity;
    update();
}

void NeonButton::paintEvent(QPaintEvent *event)
{
    QPushButton::paintEvent(event);

    if (m_isGlowing)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // 创建红色霓虹效果
        QColor neonColor(255, 0, 0, int(120 * m_glowIntensity));

        // 绘制外发光

        painter.setPen(QPen(neonColor, 2));

        // with rounded rect
        painter.drawRoundedRect(rect().adjusted(1, 1, -(1), -(1)), 1, 1);

        // without rounded rect
        // painter.drawRect(rect().adjusted(i + 1, i + 1, -(i + 1), -(i + 1)));
    }
}