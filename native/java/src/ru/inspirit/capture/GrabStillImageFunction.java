package ru.inspirit.capture;

import android.util.Log;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import com.adobe.fre.FREContext;
import com.adobe.fre.FREFunction;
import com.adobe.fre.FREObject;
import com.adobe.fre.FREByteArray;
import com.adobe.fre.FREBitmapData;

public class GrabStillImageFunction implements FREFunction 
{
    private static final String TAG = "GrabStillImageFunction";
    
    public static final String KEY = "grabCamShot";
    
    @Override
    public FREObject call(FREContext context, FREObject[] args) 
    {

        try
        {
            CameraSurface capt = CaptureAndroidContext.cameraSurfaceHandler;

            FREByteArray _ba = null;
            ByteBuffer bytes = null;

            if(capt != null)
            {
                args[0].setProperty( "length", FREObject.newObject( capt.getJpegDataSize() ) );
                _ba = (FREByteArray)args[0];
                _ba.acquire();

                bytes = _ba.getBytes();
                bytes.order(ByteOrder.LITTLE_ENDIAN);

                capt.grabJpegFrame(bytes);

                _ba.release();
            }
        } 
        catch(Exception e)
        {
            Log.i(TAG, "Error: " + e.toString());
        }

        return null;
    }

}
