// NeonButton.h
#pragma once
#include <QPushButton>
#include <QPropertyAnimation>

class NeonButton : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(qreal glowIntensity READ glowIntensity WRITE setGlowIntensity)

public:
    explicit NeonButton(QWidget *parent = nullptr);

    void startGlowing();
    void stopGlowing();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    qreal glowIntensity() const { return m_glowIntensity; }
    void setGlowIntensity(qreal intensity);

    QPropertyAnimation *m_glowAnimation;
    qreal m_glowIntensity = 0.0;
    bool m_isGlowing = false;
};
