#include <zmq.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/stat.h>

int main (int argc, char **argv)
{
	int rc;

	if (argc == 4) {
		void *context = zmq_ctx_new ();
		void *ippQ = zmq_socket (context, ZMQ_REQ);
		char endPt[28];

		sprintf(endPt,"tcp://%s:%s", (char*)argv[1], (char*)argv[2]);
		rc = zmq_connect (ippQ, endPt);
		assert(rc == 0);
		zmq_msg_t ippRequestMsg;
		zmq_msg_t ippResponseMsg;

		struct stat st;
		stat(argv[3], &st);
		size_t fSize = st.st_size;
		rc = zmq_msg_init_size(&ippRequestMsg, fSize);
		assert (rc == 0);

		FILE *fp = fopen(argv[3], "rb");
		assert (fp);
		fread(zmq_msg_data(&ippRequestMsg), sizeof(char), fSize, fp);

		rc = zmq_msg_init(&ippResponseMsg);
		assert(rc == 0);

		// note - client is NOT responsible to free a request
		zmq_msg_send(&ippRequestMsg,ippQ, 0);

		zmq_msg_recv(&ippResponseMsg, ippQ, 0);
		printf("received message = %s\n",(char*)zmq_msg_data(&ippResponseMsg));

		// you are responsible to close the message
		zmq_msg_close(&ippResponseMsg);

		// graceful shutdown requres the queue and context be closed
		zmq_close (ippQ);
		zmq_ctx_destroy (context);
		return(0);
	} else {
		printf("usage: cClient ippQAddr ippQPort imageFileName\n");
		return(-1);
	}
}
