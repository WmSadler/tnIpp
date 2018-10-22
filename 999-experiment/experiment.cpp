#include <opencv2/opencv.hpp>

using namespace cv;

int main ()
{
    VideoCapture cam(0);
    cam.set(CV_CAP_PROP_FRAME_WIDTH,640);
    cam.set(CV_CAP_PROP_FRAME_HEIGHT,480);

    namedWindow("CapturedImage", WINDOW_AUTOSIZE);
    namedWindow("ProcessedImage", WINDOW_AUTOSIZE);

    int keyPress;
    Mat imgRawCap(480,640,CV_8UC4);
    Mat imgPostProc(480,640,CV_8UC4);
    std::vector<uchar> imgJpeg;

    bool done = false;
    while (!done) {
        cam >> imgRawCap;
        imwrite("imgRawCap.jpg",imgRawCap);
        imencode(".jpeg",imgRawCap,imgJpeg);
        imdecode(imgJpeg,0,&imgPostProc);
        imwrite("imgPostProc.jpg",imgPostProc);
        imshow("CapturedImage", imgRawCap);
        imshow("ProcessedImage",imgPostProc);
        keyPress = waitKey(30);
        done = (keyPress==27);
    }
}
