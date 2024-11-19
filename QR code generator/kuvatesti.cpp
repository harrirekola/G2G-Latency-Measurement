#include "kuvatesti.h"
#include "ui_kuvatesti.h"
#include "qrcodegen.hpp"
#include <QPixmap>
#include <QStandardPaths>
#include <QDebug>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <QByteArray>
#include <QSvgRenderer>
#include <QPainter>
#include <QVBoxLayout>
#include <QDateTime>
#include <QTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <QImage>

using std::uint8_t;
using qrcodegen::QrCode;
using qrcodegen::QrSegment;

static QString toSvgString(const QrCode &qr, int border) {
    if (border < 0)
        throw std::domain_error("Border must be non-negative");
    if (border > INT_MAX / 2 || border * 2 > INT_MAX - qr.getSize())
        throw std::overflow_error("Border too large");

    std::ostringstream sb;
    sb << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    sb << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n";
    sb << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" viewBox=\"0 0 ";
    sb << (qr.getSize() + border * 2) << " " << (qr.getSize() + border * 2) << "\" stroke=\"none\">\n";
    sb << "\t<rect width=\"100%\" height=\"100%\" fill=\"#FFFFFF\"/>\n";
    sb << "\t<path d=\"";
    for (int y = 0; y < qr.getSize(); y++) {
        for (int x = 0; x < qr.getSize(); x++) {
            if (qr.getModule(x, y)) {
                if (x != 0 || y != 0)
                    sb << " ";
                sb << "M" << (x + border) << "," << (y + border) << "h1v1h-1z";
            }
        }
    }
    sb << "\" fill=\"#000000\"/>\n";
    sb << "</svg>\n";
    QString qstr = QString::fromStdString(sb.str());
    return qstr;
}

kuvatesti::kuvatesti(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::kuvatesti)
    , interval(double(1000) / double(24))
{
    ui->setupUi(this);

    // Create a QLabel to display the QR code
    svgLabel = new QLabel(this);
    svgLabel->setAlignment(Qt::AlignCenter);

    // Set up the layout to add the QLabel to the main window
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setAlignment(Qt::AlignCenter);
    layout->addWidget(svgLabel);

    // Create a central widget and set the layout
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);

    frameCounter = 1;

    // Start the QR code update logic immediately or after a delay
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &kuvatesti::updateQRCode);
    timer->start(interval);
}

void kuvatesti::updateQRCode() {
    QElapsedTimer timer;
    timer.start();
    // Get the current timestamp
    QDateTime currentTime = QDateTime::currentDateTime();
    QString timestamp = currentTime.toString("yyyy-MM-dd HH:mm:ss.zzzzz");

    // Append the timestamp and frame number to the text to encode   
    QString textToEncode = timestamp + ",Frame:" + QString::number(frameCounter);
    qDebug() << "Text to encode: " << textToEncode;

    // Encode the text into the QR code
    const QrCode::Ecc errCorLvl = QrCode::Ecc::HIGH;
    const QrCode qr = QrCode::encodeText(textToEncode.toUtf8().constData(), errCorLvl);

    // Render the QR code directly onto a QImage
    int qrSize = qr.getSize();
    int scale = 20;
    int border = 4;
    int imageSize = (qrSize + border * 2) * scale;

    QImage image(imageSize, imageSize, QImage::Format_RGB32);
    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setBrush(Qt::black);
    painter.setPen(Qt::NoPen);

    // Draw the QR code onto the image
    for (int y = -border; y < qrSize + border; y++) {
        for (int x = -border; x < qrSize + border; x++) {
            if (qr.getModule(x, y)) {
                QRect rect((x + border) * scale, (y + border) * scale, scale, scale);
                painter.drawRect(rect);
            }
        }
    }
    painter.end();

    qint64 elapsedTime = timer.elapsed();

    // Display the QR code image
    svgLabel->setPixmap(QPixmap::fromImage(image));

    // Store frame data in the buffer
    QJsonObject frameObject;
    frameObject["timestamp"] = timestamp;
    frameObject["frame_number"] = frameCounter;
    frameObject["qr_generation_time_ms"] = elapsedTime;  // Record QR code generation time
    frameBuffer.append(frameObject);

    // Write to file when buffer reaches a certain size
    if (frameBuffer.size() >= bufferSize) {
        saveBufferedDataToJson();
    }

    // Increment the frame counter for the next frame
    frameCounter++;
}

void kuvatesti::saveBufferedDataToJson() {
    // Set the path for the JSON file
    QString documentsFolder = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString filePath = documentsFolder + "/qr_data.json";

    QFile file(filePath);
    QJsonDocument jsonDoc;
    QJsonArray framesArray;

    if (file.exists()) {
        if (file.open(QIODevice::ReadOnly)) {
            // Load the existing content of the file
            QByteArray fileData = file.readAll();
            jsonDoc = QJsonDocument::fromJson(fileData);
            file.close();
        }

        if (!jsonDoc.isNull()) {
            if (jsonDoc.isArray()) {
                // Get the existing array of frames
                framesArray = jsonDoc.array();
            } else if (jsonDoc.isObject()) {
                // If the existing JSON is an object, convert it to an array
                framesArray.append(jsonDoc.object());
            }
        }
    }

    // Append buffered frames to the array
    for (const QJsonObject &frameObject : frameBuffer) {
        framesArray.append(frameObject);
    }

    // Create a JSON document from the updated array
    jsonDoc = QJsonDocument(framesArray);

    // Open the file for writing (overwrite with new data)
    if (file.open(QIODevice::WriteOnly)) {
        // Write the updated JSON document to the file
        file.write(jsonDoc.toJson(QJsonDocument::Indented));
        file.close();
    }

    // Clear the buffer after saving
    frameBuffer.clear();
}

kuvatesti::~kuvatesti()
{
    // Save any remaining buffered data before exiting
    if (!frameBuffer.isEmpty()) {
        saveBufferedDataToJson();
    }
    delete ui;
}
