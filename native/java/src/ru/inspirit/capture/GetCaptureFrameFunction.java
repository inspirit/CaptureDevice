package ru.inspirit.capture;

import android.util.Log;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import com.adobe.fre.FREContext;
import com.adobe.fre.FREFunction;
import com.adobe.fre.FREObject;
import com.adobe.fre.FREByteArray;
import com.adobe.fre.FREBitmapData;

public class GetCaptureFrameFunction implements FREFunction 
{
    private static final int GET_FRAME_BITMAP = 2;// 1 << 1;
    private static final int GET_FRAME_RAW_BYTES = 4;// 1 << 2;
    private static final int GET_POWER_OF_2_FRAME_BGRA_BYTES = 8;// 1 << 3;

    private static final String TAG = "GetCaptureFrameFunction";
    
    public static final String KEY ="getCaptureFrame";
    
    @Override
    public FREObject call(FREContext context, FREObject[] args) 
    {
        FREObject result = null;
        int _id = -1;
        int _opt = 0;

        FREByteArray _ba = null;
        ByteBuffer bytes = null;
        int res = 0;

        int w2 = 0, h2 = 0;

        try{

            _id = args[0].getAsInt();
            // read options
            _ba = (FREByteArray)args[1];
            _ba.acquire();

            bytes = _ba.getBytes();
            bytes.order(ByteOrder.LITTLE_ENDIAN);

            _opt = bytes.getInt(0);
            w2 = bytes.getInt(4);
            h2 = bytes.getInt(8);

            _ba.release();
        }
        catch(Exception e0)
        {
            Log.i(TAG, "GET ARGS Error: " + e0.toString());

            try
            {
                result = FREObject.newObject( res );
            }
            catch(Exception e3)
            {
                Log.i(TAG, "Construct Result Object Error: " + e3.toString());
            }

            return result;
        }

        CameraSurface capt = CaptureAndroidContext.cameraSurfaceHandler;

        if(capt.isNewFrame)
        {
            res = 1;
            capt.isNewFrame = false;

            try
            {

                if((_opt & GET_FRAME_BITMAP) != 0)
                {
                   
                    FREBitmapData _bmp = (FREBitmapData)args[2];
                    _bmp.acquire();

                    bytes = _bmp.getBits();
                    bytes.order(ByteOrder.LITTLE_ENDIAN);
                    capt.grabFrame(bytes);

                    _bmp.release();
                    
                }

                if((_opt & GET_FRAME_RAW_BYTES) != 0)
                {
                    _ba = (FREByteArray)args[3];
                    _ba.acquire();

                    bytes = _ba.getBytes();
                    bytes.order(ByteOrder.LITTLE_ENDIAN);
                    capt.grabRawFrame(bytes);

                    _ba.release();
                }

                if((_opt & GET_POWER_OF_2_FRAME_BGRA_BYTES) != 0)
                {
                    _ba = (FREByteArray)args[4];
                    _ba.acquire();

                    bytes = _ba.getBytes();
                    bytes.order(ByteOrder.LITTLE_ENDIAN);
                    capt.grabP2Frame(bytes, w2, h2);

                    _ba.release();
                }

            }
            catch(Exception e1)
            {
                Log.i(TAG, "Error: " + e1.toString());
            }
        }

        try
        {
            result = FREObject.newObject( res );
        }
        catch(Exception e3)
        {
            Log.i(TAG, "Construct Result Object Error: " + e3.toString());
        }

        return result;
    }

}
