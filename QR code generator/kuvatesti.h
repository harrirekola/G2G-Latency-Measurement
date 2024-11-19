#ifndef KUVATESTI_H
#define KUVATESTI_H

#include <QMainWindow>
#include <QSvgRenderer>
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
#include <QList>

QT_BEGIN_NAMESPACE
namespace Ui {
class kuvatesti;
}
QT_END_NAMESPACE

class kuvatesti : public QMainWindow
{
    Q_OBJECT

public:
    kuvatesti(QWidget *parent = nullptr);
    ~kuvatesti();

private slots:
    void updateQRCode();
    void saveBufferedDataToJson();

private:
    Ui::kuvatesti *ui;

    QLabel *svgLabel;
    QTimer *timer;
    QTimer *simulatedLoading;
    int frameCounter;
    double interval;
    QList<QJsonObject> frameBuffer;
    const int bufferSize = 100;
};

#endif // KUVATESTI_H
