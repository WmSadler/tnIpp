#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <iostream>
#include <sstream>
#include <syslog.h>
#include <zmq.h>

#include <chrono>

#include "../include/zhelpers.hpp"
#include "../include/INIReader.h"

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
			zmq::context_t context(1);
			zmq::socket_t sendQ(context,ZMQ_REQ);
			sendQ.connect(ippBrokerUri);

			// starting capture from camera
			VideoCapture cam(0);
			cam.set(CV_CAP_PROP_FRAME_WIDTH,capW);
			cam.set(CV_CAP_PROP_FRAME_HEIGHT,capH);
            //cam.set(CV_CAP_PROP_FPS,120);

			namedWindow("VideoImage", WINDOW_AUTOSIZE|WINDOW_GUI_EXPANDED);
			namedWindow("Processed", WINDOW_AUTOSIZE|WINDOW_GUI_EXPANDED);
			moveWindow("VideoImage",0,0);
			moveWindow("Processed",capW+2,0);

			time_t tNow;
			time_t tStart = time(0);
			string lastProcStatus = "";

			int jpgLen = 0;

			while (!done) {
				cam >> imgRawCap;
				frameCounter++;
				tNow = time(0);
				fps = frameCounter / difftime(tNow,tStart);

				if (sendToFr){
					auto procStart = chrono::high_resolution_clock::now();

					// convert captured image to jpeg
					imencode(".jpeg",imgRawCap,imgJpeg);

					// send to ipp here
					jpgLen = imgJpeg.size();
					zmq::message_t msg(jpgLen);
					memcpy(msg.data(), imgJpeg.data(),jpgLen);
					sendQ.send(msg);

					// reply from IPP
					zmq::message_t imgProcMsg;
					sendQ.recv(&imgProcMsg);
					std::vector<uchar> buf(imgProcMsg.size());
					memcpy(&buf, imgProcMsg.data(),imgProcMsg.size());

					imgPostProc = imdecode(buf,IMREAD_ANYCOLOR|IMREAD_ANYDEPTH);
					auto procEnd = chrono::high_resolution_clock::now();
					milliseconds ns = duration_cast<milliseconds>(procEnd - procStart);
					imshow("Processed", imgPostProc);
					lastProcStatus = "Proc Time Last Image: "+to_string(ns.count())+" milliseconds";
					sendToFr = false;
				}

				// build status bar info
				asprintf(&fpsNum,"%s %.1f %s",sendToFr?"SENDING":"LOCAL",fps,lastProcStatus.c_str());
				displayFps = string(fpsNum);
				free(fpsNum);
				displayStatusBar("VideoImage",displayFps);

				// menu items
				if (showMenu) {
					displayOverlay("VideoImage","\nMENU <Enter to Collapse>\n\n"
												"C or c - Capture and Send\n"
												"ESC or Q - Exit\n",1);
				} else {
					displayOverlay("VideoImage","MENU <Enter to Expand>",1);
				}
				imshow("VideoImage", imgRawCap);

				keyPress = waitKey(1);
				switch (keyPress) {
					case 'C' :
					case 'c' : sendToFr = !sendToFr;break;
					case 13 : showMenu=!showMenu;break;
					case 'q' :
					case 27 : done = true;break;
				}
			}
			cam.release();
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
