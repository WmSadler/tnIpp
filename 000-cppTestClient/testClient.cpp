#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <iostream>
#include <sstream>
#include <syslog.h>
#include <zmq.h>

#include "../include/zhelpers.hpp"
#include "../include/INIReader.h"

using namespace zmq;
using namespace cv;
using namespace std;

int main (int argc, char **argv)
{
	int returnStatus = 0;
	int rc = 0;
	int keyPress;
	bool done = false;
	bool sendToFr;

	Mat imgRawCap;
	Mat imgPostProc;
	std::vector<uchar> imgJpeg;
	int capW, capH;

	float fps = 0.0;
	string displayFps;
	char *fpsNum;
	int frameCounter=0;

	string ippBrokerIp, ippBrokerPort, ippBrokerUri;

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
			sendToFr = iniBuffer.GetBoolean("ippClient","sendToFr",false);
			capW = iniBuffer.GetInteger("ippClient","camWidth",1280);
			capH = iniBuffer.GetInteger("ippClient","camHeight",720);
			syslog(LOG_INFO,"Conncection attempt to ippBroker at %s",ippBrokerUri.c_str());
			syslog(LOG_INFO,"Using %i,%i camera size",capW,capH);

			// connect to IPP zmq endpoint
			context_t context();

			// starting capture from camera
			VideoCapture cam(0);
			cam.set(CV_CAP_PROP_FRAME_WIDTH,capW);
			cam.set(CV_CAP_PROP_FRAME_HEIGHT,capH);

			namedWindow("VideoImage", WINDOW_AUTOSIZE|WINDOW_GUI_EXPANDED);
			namedWindow("Processed", WINDOW_AUTOSIZE);

			std::time_t tStart = std::time(0);
			while (!done) {
				cam >> imgRawCap;
				//cout << rawImgBGR.depth() << endl;
				frameCounter++;

				if (sendToFr){
					imencode(".jpeg",imgRawCap,imgJpeg);
					// send here
					imgPostProc = imdecode(imgJpeg,IMREAD_ANYCOLOR|IMREAD_ANYDEPTH);
				} else{
					imgPostProc = imgRawCap;
				}

				std::time_t tNow = std::time(0);
				fps = frameCounter / difftime(tNow,tStart);
				asprintf(&fpsNum,"%.1f",fps);
				if (sendToFr) {
					displayFps = "SENDING FPS:" + string(fpsNum);
				} else {
					displayFps = "LOCAL FPS:" + string(fpsNum);
				}
				free(fpsNum);
				displayStatusBar("VideoImage",displayFps,0);
				imshow("VideoImage", imgRawCap);
				imshow("Processed", imgPostProc);

				keyPress = waitKey(1);
				switch (keyPress) {
					case 's' : sendToFr = !sendToFr;break;
					case 'q' :
					case 27 : done = true;break;
				}
			}
		} else {
			syslog(LOG_ERR, "Ini File Exception");
			returnStatus = -1;
		}
	} catch (...){
		syslog(LOG_ERR, "terminating with exception");
		returnStatus = -1;
	}

	destroyAllWindows();
	closelog();
	return(returnStatus);
}
