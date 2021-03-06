/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package logformat.clog2;

import java.io.*;

// Class corresponds to CLOG_Rec_Header
public class RecHeader
{
    private static final int BYTESIZE = 8 + 2 * 4;
    public         double timestamp;
    public         int    rectype;
    public         int    taskID;   // i.e. procid, = rank in MPI_COMM_WORLD

    public RecHeader()
    {
        timestamp  = Const.INVALID_double;
        rectype    = Const.INVALID_int;
        taskID     = Const.INVALID_int;
    }

    public RecHeader( DataInputStream istm )
    {
        this.readFromDataStream( istm );
    }

    public int readFromDataStream( DataInputStream istm )
    {
        try {
            timestamp  = istm.readDouble();
            rectype    = istm.readInt();
            taskID     = istm.readInt();
        } catch ( IOException ioerr ) {
            ioerr.printStackTrace();
            return 0;
        }

        return BYTESIZE;
    }

    public int skipBytesFromDataStream( DataInputStream in )
    {
        try {
            in.skipBytes( BYTESIZE );
        } catch ( IOException ioerr ) {
            ioerr.printStackTrace();
            return 0;
        }

        return BYTESIZE;
    }

/*
    //Copy Constructor
    public RecHeader copy()
    {
        RecHeader cp  = new RecHeader();
        cp.timestamp  = this.timestamp;
        cp.rectype    = this.rectype;
        cp.taskID     = this.taskID;
        return cp;
    }
*/

    public int getRecType()
    {
        return this.rectype;
    }

    public String toString()
    {
        return ( "RecHeader"
               + "[ timestamp=" + timestamp
               + ", rectype=" + rectype
               + ", taskID=" + taskID
               // + ", BYTESIZE=" + BYTESIZE
               + " ]" );
    } 
}
