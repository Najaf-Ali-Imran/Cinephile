#include <QLabel>
#include <QMouseEvent>

class ClickableLabel : public QLabel
{
    Q_OBJECT
public:
    using QLabel::QLabel;
signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        QLabel::mousePressEvent(event);
        if (event->button() == Qt::LeftButton) {
            emit clicked();
        }
    }
};
