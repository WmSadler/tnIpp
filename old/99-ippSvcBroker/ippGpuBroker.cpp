#include <iostream>
#include <syslog.h>
#include <thread>
#include <czmq.h>
#include <sstream>
#include "../include/ippIni.h"
#include "../include/INIReader.h"

using std::string;
using std::cout;using std::cerr;using std::endl;
using std::ifstream;using std::stringstream;
using std::exception;

// global ini vars
bool sendToS3Upload;

static void gpuWorker(zsock_t *pipe, void *args) {
	// init stuff
	zsock_t *s3Upload;
	int rc;
	// get thread id for loggin purposes
	std::ostringstream ss;
	ss << std::this_thread::get_id(); string id = ss.str();

	s3Upload = zsock_new_push((char *)args);
	assert (s3Upload);	// this must work or we be done
	assert(zsock_resolve(s3Upload)!=s3Upload);
	assert(streq(zsock_type_str(s3Upload),"PUSH"));
	syslog(LOG_INFO, "Thread: %s Successful Backend Connect via %s to %s",id.c_str(),zsock_type_str(s3Upload),(char*)args);

	// one time start of pipe
	zsock_signal(pipe,0);
	// first say you're ready
	rc=zsock_send(pipe,"s","$RDY");
	assert(rc==0);

	// process incoming messages
	bool done = false;
	while (!done) {
		zmsg_t *msg;
		// poll for your message
		msg = zmsg_recv_nowait(pipe);
		if (msg) {
			// parse payload for processing
			zframe_t *msgId = zmsg_pop(msg);
			zframe_t *delimiter = zmsg_pop(msg); zframe_destroy(&delimiter);
			zframe_t *payload = zmsg_pop(msg);
			zframe_t *count = zmsg_pop(msg);
			// done with this image - git its frames
			zmsg_destroy(&msg);

			char *buffer = (char *)zframe_data(payload);
			size_t bufLen= zframe_size(payload);
			size_t *msgCount = (size_t *)zframe_data(count);

			// zactor must check for $TERM
			if (streq(buffer,"$TERM")) {
				// killall parsed frames since none are being used anymore
				zframe_destroy(&msgId);zframe_destroy(&payload);zframe_destroy(&count);
				syslog(LOG_INFO, "Thread: %s terminating", id.c_str());
				done = true;
			} else {	// non terminate message - process image

				/////////////////
				// GPU work here
				string json = "JSON Response["+std::to_string(*msgCount)+"]";
				/////////////////

				// send rendered images to uploader - img and filename
				if (sendToS3Upload) {
					// use msgCount to create filename
					string s3FileName = "img." + std::to_string(*msgCount) + ".jpg";
					// msg to uploader is name and payload - both frames are now owned by uploader
					rc = zsock_send(s3Upload,"sf",s3FileName.c_str(),payload);
					if (rc != 0) syslog(LOG_ERR, "GPU thread %s send image in the blind to S3 failed",id.c_str());
				}
				// relese payload
				zframe_destroy(&payload);

				// return incoming json response ID frame is owned by client
				rc = zsock_send(pipe,"fzs",msgId,json.c_str());
				assert(rc==0);

				// always kill count we never send it on
				zframe_destroy(&count);
			}
		} else usleep(100);	// breathe
	}
	zsock_destroy(&s3Upload);
}

int main(int argc, char** argv)
{
	int returnStatus = 0;
	int rc;

	// from ini file
	size_t reportEveryNth;
	float procThreadFactor;
	string ippGpuEndPt, ippS3uploadEndPt;
	zlist_t *workerThreadList = zlist_new();
	zsock_t *imgFrontEndQ;

	// logging
	setlogmask (LOG_UPTO (LOG_INFO));
	openlog ("ippGpuBroker", LOG_PERROR | LOG_CONS | LOG_PID, LOG_DAEMON);

	string iniFileName;
	if(argc == 2){
		syslog(LOG_INFO, "Starting with non-default ini file %s", argv[1]);
		iniFileName = argv[1];
	} else {
		syslog(LOG_INFO, "Starting with default ini file %s", iniFile);
		iniFileName = iniFile;
	}
	try {	// overall program - if we die here, tell us something if you can...
		try {	// ini file parse - this fails we're done
			INIReader iniBuffer(iniFileName);
			ippGpuEndPt = "tcp://" + iniBuffer.Get("ippGpuBroker","ippGpuFrontQIP","") + ":" +
												iniBuffer.Get("ippGpuBroker","ippGpuFrontQPort","");
			ippS3uploadEndPt = "tcp://" + iniBuffer.Get("ippS3upload","ippS3uploadQIP","") + ":" +
												iniBuffer.Get("ippS3upload","ippS3uploadQPort","");
			procThreadFactor = iniBuffer.GetReal("ippGpuBroker","ippGpuThreadProcFactor",1.0);
			sendToS3Upload = iniBuffer.GetBoolean("ippGpuBroker","sendToS3Upload",true);
			reportEveryNth = iniBuffer.GetInteger("ippGpuBroker", "logEveryNthMsg",10);
			syslog(LOG_INFO, "GPU Endpoint = %s, S3 Upload Endpoint = %s, thread proc factor = %4.2f", ippGpuEndPt.c_str(),ippS3uploadEndPt.c_str(), procThreadFactor);
		}
		catch (exception e) {
			syslog(LOG_ERR, "Ini File Exception - Caught [%s] - terminating", e.what());
			cerr << e.what() << endl;
			returnStatus = -1;
			throw e;
		}
		try {	// setup frontend Q - input message handling must setup correctly or we're done
			imgFrontEndQ = zsock_new_router(ippGpuEndPt.c_str());
			assert (imgFrontEndQ);	// this must work or we be done
			assert(zsock_resolve(imgFrontEndQ)!=imgFrontEndQ);
			assert(streq(zsock_type_str(imgFrontEndQ),"ROUTER"));
			zsock_flush(imgFrontEndQ);
			syslog(LOG_INFO, "Successful Frontend Connect via %s to %s",zsock_type_str(imgFrontEndQ),zsock_endpoint(imgFrontEndQ));
		}
		catch (exception e) {
			syslog(LOG_ERR, "ZMQ Incoming image Q Exception - \"%s\" - terminating", e.what());
			returnStatus = -1;
			throw e;
		}
		try {	// available worker thread pool must work or we're done
			int nProc = (sysconf(_SC_NPROCESSORS_ONLN)*procThreadFactor);
			if (nProc < 2) nProc = 2;	// gotta have a couple
			for (int i = 0; i < nProc; i++) {
				// kick off worker threads
				zlist_append(workerThreadList, zactor_new(gpuWorker,(void *)ippS3uploadEndPt.c_str()));
			}
			syslog(LOG_INFO, "GPU PL Incoming Image Thread Pool Size = %lu", zlist_size(workerThreadList));
		}
		catch (exception e) {
			syslog(LOG_ERR, "Thread Initialization exception - \"%s\" - terminating", e.what());
			returnStatus = -1;
			throw e;
		}
		try { // main processing block
			bool done = false;
			zlist_t *freeWorkers = zlist_new();
			// main processing loop
			size_t msgImgInCount=0;
			size_t msgRspOutCount=0;
			while (!done) {
				//  poll and build list of ready worker threads
				zactor_t * thrQ = (zactor_t *)zlist_first(workerThreadList);
				while (thrQ!=NULL) {
					// check actors for message - if there is one, its done and free to send
					zmsg_t *msg;
					msg = zmsg_recv_nowait(thrQ);
					if (msg) {	// if there is a message thread is ready
						zframe_t *payload = zmsg_last(msg);
						char *json = (char *)zframe_data(payload); // data
						if (streq(json,"$RDY"))	{	// first time thread is ready add to freelist
							// nothing to send on this is the end of this message
							zmsg_destroy(&msg);
						} else {	// GPU is finished and has returned the json to frontend Q
							// send response from worker
							rc=zmsg_send(&msg, imgFrontEndQ);
							assert(rc==0);
							msgRspOutCount++;
							if (msgRspOutCount % reportEveryNth == 0) {
								syslog(LOG_INFO, "Response Sent Count = %lu",msgRspOutCount);
							}
						}
						// doesn't matter if its json or $RDY, thread is ready
						zlist_append(freeWorkers,thrQ);
					}
					// goto next thread in list
					thrQ = (zactor_t *)zlist_next(workerThreadList);
				}	// end found ready worker pool

				// query front end only if there is a free worker
				if (zlist_size(freeWorkers) > 0) {
					zmsg_t *msg;
					msg = zmsg_recv_nowait(imgFrontEndQ);
					if (msg) {	// we have a message
						// count it
						msgImgInCount++;
						// send it as the last frame of the message to the thread worker
						zframe_t *msgCount = zframe_new(&msgImgInCount,sizeof(msgImgInCount));
						zmsg_append(msg,&msgCount);
						// pop the next free worker off the list - it'll be busy for a bit
						zactor_t *thrQ = (zactor_t *)zlist_pop(freeWorkers);
						// send the message to the free worker for processing
						rc=zmsg_send(&msg, thrQ);
						assert(rc==0);
						if (msgImgInCount % reportEveryNth == 0 && msgImgInCount != 0) {
							syslog(LOG_INFO, "Input Image Count = %lu",msgImgInCount);
						}
					}
				}	// check for freeworkers
			}	// end main processing loop
		}
		catch (exception e) {
			syslog(LOG_ERR, "Message processing loop exception - Caught \"%s\" - terminating", e.what());
			returnStatus = -1;
			throw e;
		}

		// clean up main Queues
		zsock_destroy(&imgFrontEndQ);

		// release threads
		while (zlist_size(workerThreadList)) {
			zactor_t *a = (zactor_t *)zlist_pop(workerThreadList);
			zactor_destroy(&a);
		}
		zlist_destroy(&workerThreadList);
	}
	catch (...) {
		syslog(LOG_ERR, "Unexpected Exception");
		cerr << "Unexpected Exception" << endl;
		returnStatus = -1;
	}

	closelog();
	return (returnStatus);
}
