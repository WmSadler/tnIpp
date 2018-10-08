#include <opencv2/opencv.hpp>
#include <opencv2/xphoto.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <zmq.h>
#include <syslog.h>

using namespace std;
using namespace cv;

static float widthFactor = 1.4;
static float heightFactor = widthFactor * 1.3;
String haarLocation = "/usr/local/share/opencv4/haarcascades/";
String faceCName = haarLocation+"haarcascade_frontalface_alt.xml";
CascadeClassifier faceC;
static int K = 2;

int main(int argc, char **argv)
{
	setlogmask (LOG_UPTO (LOG_INFO));
	openlog ("experiment", LOG_CONS | LOG_PID, LOG_DAEMON);

	// load setups
	syslog(LOG_INFO, "Parsing ini file");
	int cameraId = 0;
	int nCols = 1280;
	int nRows = 1024;
	int MaxSize = 1000000;
	int MinSize = 250000;

	// initialize from setup
	VideoCapture eye(cameraId);
    eye.set(CAP_PROP_FRAME_WIDTH,nCols);
    eye.set(CAP_PROP_FRAME_HEIGHT,nRows);

	faceC.load(faceCName);

	int deltaMaskX, deltaMaskY;

	// never ending video loop
	bool done = false;
	while(!done) {

		Mat img, imgCap;
		Mat imgHSV, hsvNormalLum, imgNorm;

		if (argc > 1) {	// get image from file
            imgCap = imread(argv[1],IMREAD_UNCHANGED);
		}else {
			// capture video feed image
			eye >> imgCap;
		}

		try {
			// adjust image size for processing
			float sWidth = imgCap.cols;
			float sHeight = imgCap.rows;
			while (sWidth*sHeight > MaxSize) {
				sWidth = sWidth * 0.99;
				sHeight = sHeight * 0.99;
			}
			while (sWidth*sHeight < MinSize) {
				sWidth = sWidth * 1.01;
				sHeight = sHeight * 1.01;
			}
			if (sWidth*sHeight < imgCap.cols*imgCap.rows) {	// shrink image
				resize(imgCap, imgCap, Size(sWidth,sHeight),0,0,INTER_AREA);
			} else if (sWidth*sHeight > imgCap.cols*imgCap.rows) {	// increase image
				resize(imgCap, imgCap, Size(sWidth,sHeight), 0, 0, INTER_LINEAR);
			}
			// clone original resized image so we have it to compare and use for baseline
			img = imgCap.clone();

			// detect faces on original image
			std::vector<Rect> faces;
			faceC.detectMultiScale(img, faces, 1.1, 2, 0, Size(50,50));

			// get biggest face if more than 1
			int faceIdx = 0;	// zero is good for one face - if more find biggest one
			if (faces.size() > 1) {
				long size = 0;
				long max = 0;
				for (int i = 0; i < faces.size(); i++) {
					size = faces[i].height*faces[i].width;
					if (size > max) {
						max = size;
						faceIdx = i;
					}
				}
			}

			if (faces.size() >= 1) {	// if a face is found further processing
                //white balance the image
                Ptr<xphoto::WhiteBalancer> wb;
                wb = xphoto::createSimpleWB();
                wb->balanceWhite(img, img);

				// Normalize Illumination
				// convert to HSV
                cvtColor(img, imgHSV, COLOR_BGR2HSV_FULL);
                vector<Mat> hsvChannels(3);
				split(imgHSV,hsvChannels);
				Ptr<CLAHE> clahe = createCLAHE(4);
				clahe->apply(hsvChannels[2], hsvNormalLum);
				hsvNormalLum.copyTo(hsvChannels[2]);
				merge(hsvChannels, imgNorm);
				cvtColor(imgNorm, imgNorm, COLOR_HSV2BGR_FULL);

				// want a larger rectangle than faces normally returns
				Rect faceI = faces[faceIdx];
				Rect rMax = Rect(0,0,img.cols,img.rows);
				Size deltaFace(faceI.width * 0.0f, faceI.height*0.3f);
				Point offset(deltaFace.width / 2, deltaFace.height/2);
				faceI += deltaFace;
				faceI -= offset;
				faceI = faceI & rMax;
				int ellipseWidth = round(faceI.width/2);
				int ellipseHeight = round(ellipseWidth*1.618/2);

				// compute largest fitting aspect ratio rectangle centered on face
				float w2h = (float)img.cols / (float)img.rows;	// aspect ration target width over height original image
				float w = faceI.width;		// starting width is width of face
				float h = w / w2h;				// compute height for aspect ratio to start
				Point ctr = Point(faceI.x+faceI.width/2, faceI.y + faceI.height/2);	// center of detected face
				Point tl, br;
				Rect pportI;
				for (int i = 1; i < (img.cols/2+1); i++) {	// cannot go beyond cols / 2 + 1
					int tlx, tly, brx, bry;
					// figure new corners at aspect ratio
					tlx = ctr.x - w/2 - i;
					tly = ctr.y - h/2 - i;
					brx = ctr.x + w/2 + i;
					bry = ctr.y + h/2 + i;
					tl = Point(tlx, tly); br = Point(brx,bry);
					if (tlx == 0 || tly == 0) break;
					if (brx >= img.cols || bry >= img.rows) break;
				}
				pportI = Rect(tl, br);
				deltaMaskX = faceI.x - pportI.x; deltaMaskY = faceI.y - pportI.y;

				//draw face detected on original image bound
				rectangle(imgCap, faceI, CV_RGB(0,0,255),4);
				circle(imgCap, ctr ,1,CV_RGB(255,0,0),8);
				ellipse(imgCap, ctr, Size(ellipseHeight, ellipseWidth), 0.0, 0.0, 360.0, Scalar(0,255,0), 4);
				//draw passport roi on original image
				rectangle(imgCap, pportI, CV_RGB(255,0,0),4);

				// pick face for Kmeans
				Mat imgK = img(faceI).clone();	// subregion of the img consisting of the face
				Mat imgPport = img(pportI).clone();

				// kmeans on an image - All kmeans are done on imgK - pipe stuff into that
				Mat Z = imgK.reshape(1,imgK.rows*imgK.cols);
				Z.convertTo(Z, CV_32F);
				Mat labels(Z.rows, 1, CV_8U);
				Mat centers(K, 1, CV_32F);
                kmeans(Z, K, labels, TermCriteria(TermCriteria::COUNT|TermCriteria::EPS, 10, 0.05), 10, KMEANS_RANDOM_CENTERS, centers);

				// find most significant cluster
				int max = 0;
				int indx = 0;
				int id = 0;
				int clusters[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};	// assums K < 16
				for (int i = 0; i < labels.rows; i++){
					id = labels.at<int>(i,0);
					clusters[id]++;
					if (clusters[id] > max) {
						max = clusters[id];
						indx = id;
					}
				}
				int B = round(centers.at<float>(indx,0));
				int G = round(centers.at<float>(indx,1));
				int R = round(centers.at<float>(indx,2));
				Mat swatch(250, 250, img.type(),Vec3b(B,G,R));

				// create masks init black
				Mat imgMaskNoSkin(imgPport.size(), CV_8UC1);
				Mat imgMaskSkinColor(imgPport.size(), CV_8UC3, CV_RGB(0,0,0));

				// reshape indicies to make indexing easier
				Mat labelReshape = labels.reshape(1,imgK.rows);	// makes array indexing easier

				int clIdx = 0;
				Vec3b swatchColor(B,G,R);
				Vec3b zeros(0,0,0);
				Vec3b white(255,255,255);
				// replace every pixel in the masks with zero or the swatch color
				for(int y = 0; y < imgK.rows; y++) {
					for (int x = 0; x < imgK.cols; x++) {
						clIdx = labelReshape.at<int>(y,x);
						if (clIdx == indx) {
							imgMaskSkinColor.at<Vec3b>(y+deltaMaskY,x+deltaMaskX) = swatchColor;
						} else {
							imgMaskSkinColor.at<Vec3b>(y+deltaMaskY,x+deltaMaskX) = zeros;
						}
					}
				}

				// binary mask for everywhere there is skin color
				bitwise_not(imgMaskSkinColor,imgMaskNoSkin);
				imgMaskNoSkin = imgMaskNoSkin == 255;

				// render coverage here
				float baseImgFactor[3] = {1.1,1.1,1.1};	// how much base shows through [0] is upper row [1] is middle [2] is bottom
				float productFactor[3] = {0.005,0.010,0.015};	// heaviness of product [0] is left column [1] is middle [2] is right
				float levelFactor[3][3] = {{1.0,1.0,1.0},	// blend gamma - changes overall glossy - (b+p) normall = 1
													{1.0,1.0,1.0},
													{1.0,1.0,1.0}};

				Mat render[3][3];
				Mat renderFinal[3][3];
				for (int y = 0; y < 3; y++) {
					for (int x = 0; x < 3; x++) {
						float gamma;
						gamma = levelFactor[y][x] - (baseImgFactor[y]+productFactor[x]);
						imgPport.copyTo(render[x][y]);
						addWeighted(render[x][y], baseImgFactor[y], (imgMaskSkinColor), productFactor[x], gamma, render[x][y]);
						bilateralFilter(render[x][y], renderFinal[x][y], -1, 20, 2);
					}
				}

				// make a contact sheet of the nine results
				int csW = renderFinal[0][0].cols;	// cols is x axis
				int csH = renderFinal[0][0].rows;	// rows is y axis
				Mat sheet(csH*3, csW*3, renderFinal[0][0].type(),CV_RGB(0,0,0));	// contact sheet initialized
				Rect roi;
				roi.width = csW; roi.height = csH;
				for (int y = 0; y < 3; y++) {
					for (int x = 0; x < 3; x++) {
						roi.x = x*csW; roi.y = y*csH;
						renderFinal[x][y].copyTo(sheet(roi));
					}
				}

				// show steps
				imshow("Original Image with ROIs",imgCap);
				if (!imgMaskSkinColor.empty()) imshow("Mask to remove skin", imgMaskSkinColor);
				if (!sheet.empty()) imshow("contact sheet", sheet);
				if (!swatch.empty()) imshow("Swatch",swatch);
			}	// face is found

			if (argc > 1) {
				if (waitKey(0)==27) done = true;
			} else if (waitKey(10)==27) done = true;
		} catch (...) {
			syslog(LOG_ERR, "error");
		}
	}
	syslog(LOG_INFO,"Exiting");
	closelog();
	return(0);
}
