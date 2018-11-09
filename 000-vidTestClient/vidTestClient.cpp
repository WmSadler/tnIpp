#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <iostream>
#include <sstream>
#include <syslog.h>

#include <chrono>

using namespace cv;
using namespace std;
using chrono::milliseconds;
using chrono::duration_cast;

int main (int argc, char **argv)
{
	int returnStatus = 0;
	int rc = 0;
	int keyPress;
	bool done = false;
	bool sendToFr;
	bool showMenu = false;

	Mat imgRawCap;
    int capW=1280;
    int capH=1024;
    int capFps=60;

	float fps = 0.0;
	string displayFps;
	char *fpsNum;
	int frameCounter=0;

	setlogmask(LOG_UPTO (LOG_INFO));
    openlog("simpleVidTestClient",LOG_PERROR | LOG_CONS | LOG_PID, LOG_DAEMON);

    syslog(LOG_INFO,"simple Vid Test Client Start");

    // starting capture from camera
    VideoCapture cam(0);
    cam.set(CV_CAP_PROP_FRAME_WIDTH,capW);
    cam.set(CV_CAP_PROP_FRAME_HEIGHT,capH);
    cam.set(CV_CAP_PROP_FPS,capFps);

    namedWindow("Simple Capture", WINDOW_AUTOSIZE|WINDOW_GUI_EXPANDED);
    moveWindow("Simple Capture",0,0);

    time_t tNow;
    time_t tStart = time(0);
    string lastProcStatus = "";

    while (!done) {
        cam >> imgRawCap;
        frameCounter++;
        tNow = time(0);
        fps = frameCounter / difftime(tNow,tStart);

        // build status bar info
        asprintf(&fpsNum,"%s %.1f %s",sendToFr?"SENDING":"LOCAL",fps,lastProcStatus.c_str());
        displayFps = string(fpsNum);
        free(fpsNum);
        displayStatusBar("VideoImage",displayFps);

        imshow("Simple Capture", imgRawCap);

        keyPress = waitKey(10);
        switch (keyPress) {
            case 27 : done = true;break;
        }
    }
    cam.release();

    destroyAllWindows();
    closelog();
    return(returnStatus);
}
