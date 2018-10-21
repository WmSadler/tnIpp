import java.io.RandomAccessFile; 
import org.zeromq.ZMQ;

public class jClient{
	public static void main(String[] args) {
		// invocation args: ipOfEndpt portOfEndpt imageFilename
	    ZMQ.Context context = ZMQ.context(1);
	    ZMQ.Socket ippQ = context.socket(ZMQ.REQ);

	    String bindEndpt = "tcp://" + args[0] + ":" + args[1];
	    System.out.println("to ["+args[0]+"] sending :"+args[1]);
	    ippQ.connect(bindEndpt);
	    
	    try {
	    	RandomAccessFile rf = new RandomAccessFile(args[2], "r");
	    	byte[] imgBuf = new byte[(int)rf.length()];
	    	rf.readFully(imgBuf);
	    	
			ippQ.send(imgBuf, 0);
			byte[] jsonMsg = ippQ.recv(0);
			System.out.println(new String(jsonMsg));
			rf.close();
	    } catch (Exception e) {
	    	e.printStackTrace();
	    } finally {
	    	try {
	 	       ippQ.close();
	    	} catch (Throwable e) {}
	    	try {
	    		context.term();
	    	} catch (Throwable e) {}
	    }
	}
}
