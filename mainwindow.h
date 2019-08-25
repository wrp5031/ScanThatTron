#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <QThread>

namespace Ui {
class MainWindow;
}
//TESTING

//END TESTING
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QString blankScanTronFilename;
    QStringList studentScanTronFilenames;
    cv::Mat src;

private slots:
    void on_actionAbout_triggered();

    void on_actionDevs_triggered();

    void on_actionProjectManagers_triggered();

    void on_actionCode_triggered();

    void on_btnInsertBlankScanSheet_clicked();

    void on_btnStudentScanSheet_clicked();

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
