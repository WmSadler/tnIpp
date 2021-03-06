#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <iostream>
#include <sstream>
#include <syslog.h>
#include <zmq.h>

#include "../include/INIReader.h"

using namespace cv;
using std::string;
using std::cout;using std::cerr;using std::cerr;
using std::exception;

int main (int argc, char **argv)
{
    int returnStatus = 0;

    string ippBrokerIp, ippBrokerPort, ippBrokerUri;
    int capW, capH;

    setlogmask(LOG_UPTO (LOG_INFO));
    openlog("tnIppTestClient",LOG_PERROR | LOG_CONS | LOG_PID, LOG_DAEMON);

    string iniFileName="default.ini";

    if (argc == 2) {
        iniFileName = argv[1];
    }
    syslog(LOG_INFO,"Test Client start with %s", iniFileName.c_str());

    try {
        INIReader iniBuffer("../iniFiles/"+iniFileName);
        if (!iniBuffer.ParseError()) {
            ippBrokerIp = iniBuffer.Get("ippBroker","ippBrokerIp","");
            ippBrokerPort = iniBuffer.Get("ippBroker","ippBrokerPort","");
            ippBrokerUri = "tcp://"+ippBrokerIp+":"+ippBrokerPort;
            capW = iniBuffer.GetInteger("ippClient","camWidth",1280);
            capH = iniBuffer.GetInteger("ippClient","camHeight",720);
            syslog(LOG_INFO,"Conncection attempt to ippBroker at %s",ippBrokerUri.c_str());

            // starting capture from camera
            VideoCapture cam(0);
            cam.set(CV_CAP_PROP_FRAME_WIDTH,capW);
            cam.set(CV_CAP_PROP_FRAME_HEIGHT,capH);

            namedWindow("RawImage", WINDOW_AUTOSIZE | WINDOW_OPENGL);
            namedWindow("IppReturn", WINDOW_AUTOSIZE | WINDOW_OPENGL);

            int keyPress;
            bool done = false;
            Mat rawImage, ippImage;
            float fps = 0.0;
            int frameCounter=0;

            std::time_t tStart = std::time(0);
            while (!done) {
                cam >> rawImage;
                frameCounter++;

                // send to IPP Here
                rawImage.copyTo(ippImage);

                std::time_t tNow = std::time(0);
                fps = frameCounter / difftime(tNow,tStart);
                putText(rawImage, cv::format("Avg FPS=%3.1f",fps),
                        cv::Point(30,30),
                        cv::FONT_HERSHEY_SIMPLEX,1.0,
                        cv::Scalar(0,0,0));

                imshow("RawImage", rawImage);
                imshow("IppReturn", ippImage);
                keyPress = waitKey(10);
                done = (keyPress==27);
            }
        } else {
            syslog(LOG_ERR, "Ini File Exception");
            returnStatus = -1;
        }
    } catch (...){
        syslog(LOG_ERR, "terminating with exception");
        returnStatus = -1;
    }

    closelog();
    return(returnStatus);
}
