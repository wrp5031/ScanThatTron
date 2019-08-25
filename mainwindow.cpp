#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QListWidget>
#include <QTreeView>
#include <QDesktopServices>
#include <QPixmap>
#include <QThread>
#include <numeric>
#include <ctime>
#include <fstream>
#include "abstractCircleOrganizer.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

    ui->setupUi(this);
    this->setWindowTitle("ScanThatTron");
    ui -> txtEditFileNames -> setReadOnly(true);
    ui -> progressBar -> setVisible(false);
    ui -> lblProgress -> setVisible(false);
    QMainWindow::show();
}

MainWindow::~MainWindow()
{
    delete ui;

}

void MainWindow::on_actionAbout_triggered(){

    QMessageBox::about(this,tr("About Scan That Tron"),"Scan That Tron, Version 4.0.0 \n"
                                                       "--Open Source \n"
                                                       "--Not For Profit");
}

void MainWindow::on_actionDevs_triggered(){

    QMessageBox::about(this,tr("Devs"),"Wade Pines \n"
                                           "Vlad Simion");
}

void MainWindow::on_actionProjectManagers_triggered(){

    QMessageBox::about(this,tr("Project Managers"),"Haley Wiskoski \n"
                                                   "Ian Smith \n"
                                                   "Gregory Nero \n");
}

void MainWindow::on_actionCode_triggered(){

    QString link = "https://www.dropbox.com/sh/ydy81wkeng7cpey/AACP2LRY71ncRa3OuoCp31eZa?dl=1";
    QDesktopServices::openUrl(QUrl(link));
}

void MainWindow::on_btnInsertBlankScanSheet_clicked()
{

  blankScanTronFilename.clear();
  ui->lblPreviewBlankScanSheet->clear();
  blankScanTronFilename = QFileDialog::getOpenFileName(this, tr("File Browser"), "", tr("Images (*.jpg *.tif *.png)"));

  if(QString::compare(blankScanTronFilename,QString())!= 0){

      std::string current_locale_text = blankScanTronFilename.toLocal8Bit().constData();

      src = cv::imread(current_locale_text,cv::IMREAD_UNCHANGED);

      QPixmap pix(blankScanTronFilename);
      int w = ui -> lblPreviewBlankScanSheet -> width();
      int h = ui -> lblPreviewBlankScanSheet -> height();
      ui -> lblPreviewBlankScanSheet -> setPixmap(pix.scaled(w,h, Qt::KeepAspectRatio,Qt::SmoothTransformation));
}
}


void MainWindow::on_btnStudentScanSheet_clicked()
{
    studentScanTronFilenames.clear();
    ui->txtEditFileNames->clear();
    studentScanTronFilenames = QFileDialog::getOpenFileNames(this,tr("File Browser"),QDir::currentPath(),tr("Images (*.jpg *.tif *.png)") );

    for (int i =0;i<studentScanTronFilenames.count();i++){
         ui->txtEditFileNames->append(studentScanTronFilenames.at(i));
       }

}

void MainWindow::on_pushButton_clicked() {

  // We only run this code if the user has entered a blank scan tron and at least 1 student scantron
  if (QString::compare(blankScanTronFilename, QString()) != 0 && studentScanTronFilenames.count() != 0) {
    string studentScanTronFilenamesIMREAD[studentScanTronFilenames.count()];
    cv::Mat src2[studentScanTronFilenames.count()]; //Declare multiDimensional MAT to hold all images
    ui->pushButton->setEnabled(false);
    ui->btnStudentScanSheet->setEnabled(false);
    ui->btnInsertBlankScanSheet->setEnabled(false);
    ui->pushButton_2->setEnabled(false);
    ui->pushButton_3->setEnabled(false);
    ui->progressBar->setVisible(true);
    ui->progressBar->setValue(0);
    ui -> lblProgress -> setVisible(true);

    // Convert to grayscale if color
    cv::Mat gray = src;
    if (src.channels() > 1) {
      cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    }

    vector<cv::Vec3f> circles;
    // Apply the Hough Transform to find the circles
    // Randomly tried a bunch of different threshold values and these seemed to work out ok
    cv::HoughCircles(gray, circles, cv::HOUGH_GRADIENT, 1, src.rows / 100, 200,50, 0,src.rows / 50);
    vector<cv::Vec3f> bubbles(1450);  // 1450 because that is the number of bubbles in our scantron
                                      //Regular Hough Circles gives us some circles that we do not care about  This vector will hold only the bubble circles
    vector<cv::Point> bubbleXY;

    int c = 0;  // Used to fill our bubbles vector in the loop
    for (size_t i = 0; i < circles.size(); i++) {
      cv::Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
      int radius = cvRound(circles[i][2]);

      if (radius != cvRound(circles[20][2])) {  // This is just so we fill our new vector properly
        c = c - 1;
      }
      if (radius == cvRound(circles[20][2])) {  // 99.99% of the time this will be the value
                                  // The radius of all the bubble circles will
                                  // be the same  given the blank scan tron
                                  // image is of extremely high resolution  If
                                  // we come across a circle that has the same
                                  // radius as the other  bubble circles lets
                                  // add that to our bubble circle vector
        bubbles[c][0] = cvRound(circles[i][0]);  // columns
        bubbles[c][1] = cvRound(circles[i][1]);  // rows
        bubbles[c][2] = cvRound(circles[i][2]);  // radius

        bubbleXY.push_back(cv::Point(cvRound(circles[i][0]), cvRound(circles[i][1])));
      }
      c++;
    }
    struct myclass {
      bool operator()(cv::Point pt1, cv::Point pt2) {
        return (pt1.x + (1000 * pt1.y)) < (pt2.x + (1000 * pt2.y));  // This is going to sort the bubble vector from top to bottom and left to right
      }
    } myobject;

    // Sort bubbles from left to right and top to bottom
    std::sort(bubbleXY.begin(), bubbleXY.end(), myobject);

    // We now need to organize these bubbles into categories so that it will be
    // easy to match them to letters.
    vector<vector<cv::Point>> LastNamesU(26);
    vector<vector<cv::Point>> FirstNamesU(26);
    vector<vector<cv::Point>> UniversityIDU(10);
    vector<vector<cv::Point>> AdditionalInfoU(10);
    vector<vector<cv::Point>> col1(25);
    vector<vector<cv::Point>> col2(25);
    vector<vector<cv::Point>> col3(25);
    vector<vector<cv::Point>> col4(25);
    vector<vector<cv::Point>> col5(25);
    vector<vector<cv::Point>> col6(25);

    // Check "abstractCircleOrganizer.h" for explantion of wtf this does
    ipcv::circleOrganizer(bubbleXY, LastNamesU, FirstNamesU, UniversityIDU, AdditionalInfoU, col1, col2, col3, col4, col5, col6);

    // I am a goof and oranized a few vectors wrong. Just gonna transpose the goofed vectors real quick
    vector<vector<cv::Point>> FirstNames(FirstNamesU[0].size(),vector<cv::Point>());
    vector<vector<cv::Point>> LastNames(LastNamesU[0].size(),vector<cv::Point>());
    vector<vector<cv::Point>> UniversityID(UniversityIDU[0].size(),vector<cv::Point>());
    vector<vector<cv::Point>> AdditionalInfo(AdditionalInfoU[0].size(),vector<cv::Point>());
    for (int i = 0; i < FirstNamesU.size(); i++) {
      for (int j = 0; j < FirstNamesU[i].size(); j++) {
        FirstNames[j].push_back(FirstNamesU[i][j]);
      }
    }
    for (int i = 0; i < LastNamesU.size(); i++) {
      for (int j = 0; j < LastNamesU[i].size(); j++) {
        LastNames[j].push_back(LastNamesU[i][j]);
      }
    }
    for (int i = 0; i < UniversityIDU.size(); i++) {
      for (int j = 0; j < UniversityIDU[i].size(); j++) {
        UniversityID[j].push_back(UniversityIDU[i][j]);
        AdditionalInfo[j].push_back(AdditionalInfoU[i][j]);
      }
    }
    // End goof fix

    /************************* MAPPING CODE!! ******************************/
    // THIS GONNA BE A BIG OL FOR LOOP because we have MULTIPLE IMAGES TO LOOK THROUGH

    int radius = cvRound(circles[20][2] - 6);  //-6 to compensate for dilation. Dilalation will widen the scan tron bubble circle so it leaks into our circle
    vector<string> stringLastNames;
    vector<string> stringFirstNames;
    vector<string> stringUniversityID;
    vector<string> stringAdditionalInfo;
    vector<string> stringAnswers;

    // Blank Sheet target circle locating
    cv::GaussianBlur(gray, gray, cv::Size(9, 9), 2, 2);
    vector<cv::Vec3f> GCP;
    cv::HoughCircles(gray, GCP, cv::HOUGH_GRADIENT, 1, src.rows / 2, 200, 100, gray.rows/100, gray.rows/25);
    for (int z = 0; z < studentScanTronFilenames.count(); z++) { //START SUPER LONG FOR LOOP
      QCoreApplication::processEvents();
      studentScanTronFilenamesIMREAD[z] = studentScanTronFilenames.at(z).toLocal8Bit().constData();
      src2[z] = cv::imread(studentScanTronFilenamesIMREAD[z], cv::IMREAD_UNCHANGED);

      // Filled Sheet target circle locating
      cv::Mat gray2 = src2[z];
      if (src2[z].channels() > 1) {
        cv::cvtColor(src2[z], gray2, cv::COLOR_BGR2GRAY);
      }
      cv::GaussianBlur(gray2, gray2, cv::Size(9, 9), 2, 2);

      vector<cv::Vec3f> GCP2;
      cv::HoughCircles(gray2, GCP2, cv::HOUGH_GRADIENT, 1, src2[z].rows / 2,200, 100, gray2.rows/50, gray2.rows/30);

      if(GCP2.size() < 3){
          cv::HoughCircles(gray2, GCP2, cv::HOUGH_GRADIENT, 1, src2[z].rows / 2,200, 100, gray2.rows/100, gray2.rows/25);
      }
      if(GCP2.size() < 3){ //This is useless. Just so it does not crash
          GCP2[0] = GCP[0];
          GCP2[1] = GCP[1];
          GCP2[1] = GCP[2];
      }

      QCoreApplication::processEvents();

      //Rotation Fix - EZ PZ
      for(int i = 0; i < GCP2.size(); i++){
          if(GCP2[i][0] < gray2.cols/8 && GCP2[i][1] < gray2.rows/8){
              cv::rotate(gray2, gray2, cv::ROTATE_180);
              cv::HoughCircles(gray2, GCP2, cv::HOUGH_GRADIENT, 1, src2[z].rows / 2,200, 100, gray2.rows/50, gray2.rows/30);
          }
          if(GCP2.size() < 3){
              cv::HoughCircles(gray2, GCP2, cv::HOUGH_GRADIENT, 1, src2[z].rows / 2,200, 100, gray2.rows/100, gray2.rows/25);
          }
          if(GCP2.size() < 3){ //This is useless. Just so it does not crash
              GCP2[0] = GCP[0];
              GCP2[1] = GCP[1];
              GCP2[1] = GCP[2];
          }
      }

      // Real juicy mapping stuff
      vector<cv::Vec3f> GCP_temp = GCP2;

      if (GCP2[2][0] < GCP2[1][0]) {
        GCP2[1] = GCP2[2];
        GCP2[2] = GCP_temp[1];
      }
      GCP_temp = GCP2;
      if (GCP2[1][0] < GCP2[0][0]) {
        GCP2[1] = GCP2[0];
        GCP2[0] = GCP_temp[1];
      }
      GCP_temp = GCP2;
      if (GCP2[1][1] < GCP2[2][1]) {
        GCP2[1] = GCP2[2];
        GCP2[2] = GCP_temp[1];
      }
      GCP_temp = GCP;
      if (GCP[2][0] < GCP[1][0]) {
        GCP[1] = GCP[2];
        GCP[2] = GCP_temp[1];
      }
      GCP_temp = GCP;
      if (GCP[1][0] < GCP[0][0]) {
        GCP[1] = GCP[0];
        GCP[0] = GCP_temp[1];
      }
      GCP_temp = GCP;
      if (GCP[1][1] < GCP[2][1]) {
        GCP[1] = GCP[2];
        GCP[2] = GCP_temp[1];
      }

      // Input Quadilateral or Image plane coordinates
      cv::Point2f inputQuad[4];
      // Output Quadilateral or World plane coordinates
      cv::Point2f outputQuad[4];

      // Lambda Matrix
      cv::Mat lambda(2, 4, CV_32FC1);

      // Set the lambda matrix the same type and size as input
      lambda = cv::Mat::zeros(gray2.rows, gray2.cols, gray2.type());

      //Slant Estimator
      double colVarGCP = abs(GCP[2][0] - GCP[1][0]); //Distance between top right and bottom centers horrizontally (Blank Scan Sheet)
      double colVarScaled = (colVarGCP * ((double)gray2.cols/gray.cols));
      double colVarGCP2 = (GCP2[2][0] - GCP2[1][0]) - (colVarScaled); //Same thing but minus colVarGCP (Student Scan Sheet)

      double rowVarGCP2 = GCP2[1][1] - GCP2[0][1]; //Distance between bottom left and right targets vertically (Student Scan Sheet)

      // The 4 points that select quadilateral on the input , from top-left in
      // clockwise order These four pts are the sides of the rect box used as
      // input
      inputQuad[0] = cv::Point2f(cvRound(GCP2[0][0] + colVarGCP2), cvRound(GCP2[2][1] - rowVarGCP2));
      inputQuad[1] = cv::Point2f(cvRound(GCP2[0][0]), cvRound(GCP2[0][1]));
      inputQuad[2] = cv::Point2f(cvRound((GCP2[1][0] + colVarScaled)), cvRound(GCP2[1][1]));
      inputQuad[3] = cv::Point2f(cvRound(GCP2[2][0]), cvRound(GCP2[2][1]));
      // The 4 points where the mapping is to be done , from top-left in
      // clockwise order
      outputQuad[0] = cv::Point2f(cvRound(GCP[0][0]), cvRound(GCP[2][1]));
      outputQuad[1] = cv::Point2f(cvRound(GCP[0][0]), cvRound(GCP[0][1]));
      outputQuad[2] = cv::Point2f(cvRound(GCP[1][0] + colVarGCP), cvRound(GCP[1][1]));
      outputQuad[3] = cv::Point2f(cvRound(GCP[2][0]), cvRound(GCP[2][1]));

      // Get the Perspective Transform Matrix i.e. lambda
      lambda = cv::getPerspectiveTransform(inputQuad, outputQuad);
      QCoreApplication::processEvents();
      // Apply the Perspective Transform just found to the src image
      cv::warpPerspective(gray2, gray2, lambda, gray.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT, 22);

      QCoreApplication::processEvents();   

      /********* Now lets do a little image pre-processing to stengthen the fillings ***************/


      QCoreApplication::processEvents();// Inverse is taken so it works with morphology
      cv::Mat stdDev;
      cv::Mat mean;
      cv::meanStdDev(gray2, mean, stdDev);
      QCoreApplication::processEvents();
      double T = 255 - stdDev.at<double>(0, 0);  // Clip/threshold value (This threshold value is experimental)

      cv::threshold(gray2, gray2, T, 255, cv::THRESH_BINARY_INV);

      // Erosion (Removes noise)
      cv::Mat dilate;

      cv::Mat kernelDilate = cv::getStructuringElement(cv::MORPH_ERODE, cv::Point(5, 5));
      cv::erode(gray2, dilate, kernelDilate);
      QCoreApplication::processEvents();
      // Dilation (Strengthens the bubbled circles)
      cv::Mat erode;

      cv::Mat kernelErosion = cv::getStructuringElement(cv::MORPH_DILATE, cv::Point(11, 11));
      cv::dilate(dilate, erode, kernelErosion);
      QCoreApplication::processEvents();

      erode = ~erode;  // Invert the image

      /*********************************************************************************************/

      // CODE TO FIND THE MEAN OF THE INSIDE OF A CIRCLE
      vector<char> charLastNames;
      vector<char> charFirstNames;
      vector<char> charUniversityID;
      vector<char> charAdditionalInfo;
      vector<char> charAnswers;

      cv::resize(erode, erode, cv::Size(), 0.25, 0.25); //This next process is very time consuming
                                                        //Resizing image makes it MUCH faster :D

      // Last Names********************
      vector<char> alphabet(26);
      iota(alphabet.begin(), alphabet.end(), 'A');
      for (int r = 0; r < LastNames.size(); r++) {
        QCoreApplication::processEvents();
        for (int c = 0; c < LastNames[0].size(); c++) {
          cv::Point center(cvRound(LastNames[r][c].x/4),cvRound(LastNames[r][c].y/4));
          cv::Mat bubbleMask = cv::Mat::zeros(erode.size(), CV_8UC1);  // Make a mask of the image
          circle(bubbleMask, center, radius/4, cv::Scalar(255, 255, 255),-1);  // Draw circle on the mask
          cv::Scalar LastNamesMean = cv::mean(erode, bubbleMask);  // Find mean of cirle

          if (LastNamesMean[0] < 100) {
            charLastNames.push_back(alphabet[c]);
            break;
          }
          QCoreApplication::processEvents();
        }
      }
      std::string strLastName(charLastNames.begin(), charLastNames.end());
      stringLastNames.push_back(strLastName); //This will hold all of the last names in all images

      // First Names********************
      for (int r = 0; r < FirstNames.size(); r++) {
        QCoreApplication::processEvents();
        for (int c = 0; c < FirstNames[0].size(); c++) {
          cv::Point center(cvRound(FirstNames[r][c].x/4),cvRound(FirstNames[r][c].y/4));
          cv::Mat bubbleMask = cv::Mat::zeros(erode.size(), CV_8UC1);  // Make a mask of the image
          circle(bubbleMask, center, radius/4, cv::Scalar(255, 255, 255),-1);  // Draw circle on the mask
          cv::Scalar FirstNamesMean =cv::mean(erode, bubbleMask);  // Find mean of cirle

          if (FirstNamesMean[0] < 100) {
            charFirstNames.push_back(alphabet[c]);
            break;
          }
          QCoreApplication::processEvents();
        }
      }
      std::string strFirstName(charFirstNames.begin(), charFirstNames.end());
      stringFirstNames.push_back(strFirstName); //This will hold all of the first names in all images

      // UniversityID********************
      vector<char> nums(10);
      iota(nums.begin(), nums.end(), '0');
      for (int r = 0; r < UniversityID.size(); r++) {
        QCoreApplication::processEvents();
        for (int c = 0; c < UniversityID[0].size(); c++) {
          cv::Point center(cvRound(UniversityID[r][c].x/4),cvRound(UniversityID[r][c].y/4));
          cv::Mat bubbleMask = cv::Mat::zeros(erode.size(), CV_8UC1);  // Make a mask of the image
          circle(bubbleMask, center, radius/4, cv::Scalar(255, 255, 255),-1);  // Draw circle on the mask
          cv::Scalar UniversityIDMean = cv::mean(erode, bubbleMask);  // Find mean of cirle

          if (UniversityIDMean[0] < 100) {
            charUniversityID.push_back(nums[c]);
            break;
          }
          QCoreApplication::processEvents();
        }
      }
      std::string strUniversityID(charUniversityID.begin(),charUniversityID.end());
      stringUniversityID.push_back(strUniversityID);
      // AdditionalInfo********************
      for (int r = 0; r < AdditionalInfo.size(); r++) {
        QCoreApplication::processEvents();
        for (int c = 0; c < AdditionalInfo[0].size(); c++) {
          cv::Point center(cvRound(AdditionalInfo[r][c].x/4),cvRound(AdditionalInfo[r][c].y/4));
          cv::Mat bubbleMask = cv::Mat::zeros(erode.size(), CV_8UC1);  // Make a mask of the image
          circle(bubbleMask, center, radius/4, cv::Scalar(255, 255, 255),-1);  // Draw circle on the mask
          cv::Scalar AdditionalInfoMean = cv::mean(erode, bubbleMask);  // Find mean of cirle

          if (AdditionalInfoMean[0] < 100) {
            charAdditionalInfo.push_back(nums[c]);
            break;
          }
          QCoreApplication::processEvents();
        }
      }
      std::string strAdditionalInfo(charAdditionalInfo.begin(),charAdditionalInfo.end());
      stringAdditionalInfo.push_back(strAdditionalInfo);
      // Answers********************
      vector<char> alpha(5);
      iota(alpha.begin(), alpha.end(), 'A');
      for (int r = 0; r < col1.size(); r++) {
        QCoreApplication::processEvents();
        int omit = 0; //What if student leaves bubble blank?
        for (int c = 0; c < col1[0].size(); c++) {
          cv::Point center(cvRound(col1[r][c].x/4), cvRound(col1[r][c].y/4));
          cv::Mat bubbleMask = cv::Mat::zeros(erode.size(), CV_8UC1);  // Make a mask of the image
          circle(bubbleMask, center, radius/4, cv::Scalar(255, 255, 255),-1);  // Draw circle on the mask
          cv::Scalar answersMean = cv::mean(erode, bubbleMask);  // Find mean of cirle

          if (answersMean[0] < 100) {
            charAnswers.push_back(alpha[c]);
            omit++;
            break;
          }
          QCoreApplication::processEvents();
        }
        if(omit == 0){
            charAnswers.push_back('X');
        }
      }
      for (int r = 0; r < col2.size(); r++) {
        QCoreApplication::processEvents();
        int omit = 0; //What if student leaves bubble blank?
        for (int c = 0; c < col2[0].size(); c++) {
          cv::Point center(cvRound(col2[r][c].x/4), cvRound(col2[r][c].y/4));
          cv::Mat bubbleMask = cv::Mat::zeros(erode.size(), CV_8UC1);  // Make a mask of the image
          circle(bubbleMask, center, radius/4, cv::Scalar(255, 255, 255),-1);  // Draw circle on the mask
          cv::Scalar answersMean =cv::mean(erode, bubbleMask);  // Find mean of cirle

          if (answersMean[0] < 100) {
            charAnswers.push_back(alpha[c]);
            omit++;
            break;
          }
          QCoreApplication::processEvents();
        }
        if(omit == 0){
            charAnswers.push_back('X');
        }
      }
      for (int r = 0; r < col3.size(); r++) {
        QCoreApplication::processEvents();
        int omit = 0; //What if student leaves bubble blank?
        for (int c = 0; c < col3[0].size(); c++) {
          cv::Point center(cvRound(col3[r][c].x/4), cvRound(col3[r][c].y/4));
          cv::Mat bubbleMask = cv::Mat::zeros(erode.size(), CV_8UC1);  // Make a mask of the image
          circle(bubbleMask, center, radius/4, cv::Scalar(255, 255, 255),-1);  // Draw circle on the mask
          cv::Scalar answersMean =cv::mean(erode, bubbleMask);  // Find mean of cirle

          if (answersMean[0] < 100) {
            charAnswers.push_back(alpha[c]);
            omit++;
            break;
          }
          QCoreApplication::processEvents();
        }
        if(omit == 0){
            charAnswers.push_back('X');
        }
      }
      for (int r = 0; r < col4.size(); r++) {
        QCoreApplication::processEvents();
        int omit = 0; //What if student leaves bubble blank?
        for (int c = 0; c < col4[0].size(); c++) {
          cv::Point center(cvRound(col4[r][c].x/4), cvRound(col4[r][c].y/4));
          cv::Mat bubbleMask = cv::Mat::zeros(erode.size(), CV_8UC1);  // Make a mask of the image
          circle(bubbleMask, center, radius/4, cv::Scalar(255, 255, 255),-1);  // Draw circle on the mask
          cv::Scalar answersMean =cv::mean(erode, bubbleMask);  // Find mean of cirle

          if (answersMean[0] < 100) {
            charAnswers.push_back(alpha[c]);
            omit++;
            break;
          }
          QCoreApplication::processEvents();
        }
        if(omit == 0){
            charAnswers.push_back('X');
        }
      }
      for (int r = 0; r < col5.size(); r++) {
        QCoreApplication::processEvents();
        int omit = 0; //What if student leaves bubble blank?
        for (int c = 0; c < col5[0].size(); c++) {
          cv::Point center(cvRound(col5[r][c].x/4), cvRound(col5[r][c].y/4));
          cv::Mat bubbleMask = cv::Mat::zeros(erode.size(), CV_8UC1);  // Make a mask of the image
          circle(bubbleMask, center, radius/4, cv::Scalar(255, 255, 255),-1);  // Draw circle on the mask
          cv::Scalar answersMean =cv::mean(erode, bubbleMask);  // Find mean of cirle

          if (answersMean[0] < 100) {
            charAnswers.push_back(alpha[c]);
            omit++;
            break;
          }
          QCoreApplication::processEvents();
        }
        if(omit == 0){
            charAnswers.push_back('X');
        }
      }
      for (int r = 0; r < col6.size(); r++) {
        QCoreApplication::processEvents();
        int omit = 0; //What if student leaves bubble blank?
        for (int c = 0; c < col6[0].size(); c++) {
          cv::Point center(cvRound(col6[r][c].x/4), cvRound(col6[r][c].y/4));
          cv::Mat bubbleMask = cv::Mat::zeros(erode.size(), CV_8UC1);  // Make a mask of the image
          circle(bubbleMask, center, radius/4, cv::Scalar(255, 255, 255),-1);  // Draw circle on the mask
          cv::Scalar answersMean =cv::mean(erode, bubbleMask);  // Find mean of cirle

          if (answersMean[0] < 100) {
            charAnswers.push_back(alpha[c]);
            omit++;
            break;
          }
          QCoreApplication::processEvents();
        }
        if(omit == 0){
            charAnswers.push_back('X');
        }
      }
      std::string strAnswers(charAnswers.begin(), charAnswers.end());
      stringAnswers.push_back(strAnswers);

      ui->progressBar->setValue(((double)(z + 1) / studentScanTronFilenames.count()) * 100);
      src2[z].release();
      charAnswers.clear();
      charLastNames.clear();
      charFirstNames.clear();
      charUniversityID.clear();
      charAdditionalInfo.clear();
      studentScanTronFilenamesIMREAD[z].clear();
      GCP2.clear();
      gray2.release();
      GCP_temp.clear();
      lambda.release();
      QCoreApplication::processEvents();
    } //END LONG ASS FOR LOOP, WEEEEEE OUTYYYYYY***********************

    // Lets find the key and its answers :D
    string correctAnswers;
    for (int i = 0; i < studentScanTronFilenames.count(); i++) {
      if (stringLastNames[i] == "KEY") {
        correctAnswers = stringAnswers[i];
      }
    }

    // We want to convert our string answers to vector of char for easy ouput to csv file
    vector<char> csvKeyAnswers(correctAnswers.begin(), correctAnswers.end());

    // Calculate the student's score
    vector<int> scoreTracker;
    for (int i = 0; i < studentScanTronFilenames.count(); i++) {
      int scoreCount = 0;
      vector<char> charStudentAnswers(stringAnswers[i].begin(),stringAnswers[i].end());
      for (c = 0; c < csvKeyAnswers.size(); c++) {
        if (csvKeyAnswers[c] == charStudentAnswers[c] && csvKeyAnswers[c] != 'X') {
          scoreCount++;
        }
      }
      scoreTracker.push_back(scoreCount);
    }
    double maxScore = 0;
    maxScore = *std::max_element(scoreTracker.begin(),scoreTracker.end());  //Find highest score. Key is included, which will be 100%
                                                                            //Max score allows for me to compute other student scores :D

    //************* Pipe out the results to .csv(Excel File) ********************

    QFileInfo fi(studentScanTronFilenames[0]);
    QString folder_name = "GradedScanTrons";

    QDir tmp_dir(fi.dir());
    tmp_dir.mkdir(folder_name);
    std::string locale_folder = folder_name.toLocal8Bit().constData();

    QDir d = QFileInfo(studentScanTronFilenames[0]).absoluteDir();
    QString absolute = d.absolutePath();
    std::string current_locale_text = absolute.toLocal8Bit().constData();

    //Send out warning messages if score is super low, or first and last names are not present
    //These things could be signs of scanning errors!
    vector<string> warning;
    string errorStatement;
    for(int i = 0; i < studentScanTronFilenames.count(); i++){
        std::string scanTronError = studentScanTronFilenames[i].toLocal8Bit().constData();
        double studScore = ((double)scoreTracker[i] / maxScore) * 100;
        if((stringFirstNames[i].empty() && stringLastNames[i] != "KEY") ||
           (stringLastNames[i].empty()) || studScore <= 30){
            warning.push_back(scanTronError); //Error
            errorStatement += scanTronError + "\n";
            stringLastNames [i] += "_ERROR_REVISE_ME";
        }
    }

    for (int i = 0; i < studentScanTronFilenames.count(); i++) {
      vector<char> csvStudentAnswers(stringAnswers[i].begin(),stringAnswers[i].end());

      double percentGrade = ((double)scoreTracker[i] / maxScore) * 100;

      ofstream myfile;
      myfile.open(current_locale_text + "\\" + locale_folder + "\\" + stringLastNames[i] + "_" + to_string(i) + "(" + to_string(percentGrade) + "%)" + ".csv");
      myfile << "Last Name, First Name, University ID, Additional Information, "
                "Question Number, Correct Answers, Student Answers, SCORE\n";
      myfile << stringLastNames[i] << ", " << stringFirstNames[i] << ", '"
             << stringUniversityID[i] << "', " << stringAdditionalInfo[i]
             << ", , , ,"
             << percentGrade << "%\n";

      for (int y = 0; y < maxScore; y++) {
        myfile << ", , , ," << y + 1 << ", " << csvKeyAnswers[y] << ", "
               << csvStudentAnswers[y] << "\n";
      }
      myfile.close();
    }

    //Lets make another file that includes all student grades and what not
    ofstream myfile;
    myfile.open(current_locale_text + "\\" + locale_folder + "\\" + "ALLSTUDENTGRADES" + ".csv");
    myfile << "Last Name, First Name, University ID, Additional Information, SCORE\n";

    for(int i = 0; i < studentScanTronFilenames.count(); i++){

        vector<char> csvStudentAnswers(stringAnswers[i].begin(), stringAnswers[i].end());

        double percentGrade = ((double)scoreTracker[i] / maxScore) * 100;

        myfile << stringLastNames[i] << ", " << stringFirstNames[i] << ", '"
               << stringUniversityID[i] << "', " << stringAdditionalInfo[i] << "," << percentGrade << "%\n";

    }
        myfile.close();


    ui->progressBar->setVisible(false);
    ui->progressBar->setValue(0);
    ui -> lblProgress -> setVisible(false);
    ui->txtEditFileNames->QTextEdit::clear();
    studentScanTronFilenames = QStringList();
    blankScanTronFilename = QString();
    ui->lblPreviewBlankScanSheet->clear();
    src.release();
    ui->pushButton_3->setEnabled(true);

    QMessageBox::information(this, tr("Attention!"), "Graded Scan Trons have been saved to \n" +
                                                      absolute + "\n in a newly created folder named \n" +
                                                        folder_name);

    if(warning.empty()){

    } else{
    QString errorFileNames = QString::fromStdString(errorStatement);
    QMessageBox::information(this, tr("Warning!"), "At least one of the Graded Scan Trons were missing First/Last Names or had scores below 30% \n"
                                                   "This could be a product of the program malfunctioning due to a scanning issue on your end \n"
                                                   "Here is a list of the scan trons that should be revised \n" + errorFileNames);
    }

    ui->pushButton->setEnabled(true);
    ui->btnStudentScanSheet->setEnabled(true);
    ui->btnInsertBlankScanSheet->setEnabled(true);
    ui->pushButton_2->setEnabled(true);
    ui->txtEditFileNames->QTextEdit::clear();
    blankScanTronFilename = QString();
    ui -> lblPreviewBlankScanSheet -> clear();
    studentScanTronFilenames.clear();
    ui->txtEditFileNames->clear();
    src.release();

  } else {
    QMessageBox::information(this, tr("Warning!"), "You must enter 1 blank scantron image \n"
                                                   "as well as at least 1 student scantron image");
  }
}

void MainWindow::on_pushButton_2_clicked()
{
    ui->txtEditFileNames->QTextEdit::clear();
    studentScanTronFilenames = QStringList();
}

void MainWindow::on_pushButton_3_clicked()
{
    ui->pushButton->setEnabled(true);
    ui->btnStudentScanSheet->setEnabled(true);
    ui->btnInsertBlankScanSheet->setEnabled(true);
    ui->pushButton_2->setEnabled(true);
    ui->txtEditFileNames->QTextEdit::clear();
    studentScanTronFilenames = QStringList();
    blankScanTronFilename = QString();
    ui -> lblPreviewBlankScanSheet -> clear();
    src.release();
}
