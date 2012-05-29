package ru.inspirit.capture;

import android.util.Log;
import android.hardware.Camera;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.CharBuffer;
import java.nio.charset.CharsetEncoder;
import java.nio.charset.Charset;

import com.adobe.fre.FREContext;
import com.adobe.fre.FREFunction;
import com.adobe.fre.FREObject;
import com.adobe.fre.FREByteArray;

public class ListCaptureDevicesFunction implements FREFunction 
{
    private static final String TAG = "ListCaptureDevicesFunction";
    
    public static final String KEY = "listDevices";
    public static Charset charset = Charset.forName("UTF-8");
    public static CharsetEncoder encoder = charset.newEncoder();
    
    @Override
    public FREObject call(FREContext context, FREObject[] args) 
    {
        FREByteArray _info;
        ByteBuffer bytes;

        try
        {
            _info = (FREByteArray)args[1];
            _info.acquire();
            bytes = _info.getBytes();
            bytes.order(ByteOrder.LITTLE_ENDIAN);

            ByteBuffer str0 = str_to_bb("Back Camera");

            int num = Camera.getNumberOfCameras();

            bytes.putInt(num);

            bytes.putInt(11);
            bytes.put(str0);
            bytes.putInt(1);
            bytes.putInt(1);

            if(num > 1)
            {
                ByteBuffer str1 = str_to_bb("Front Camera");

                bytes.putInt(12);
                bytes.put(str1);
                bytes.putInt(1);
                bytes.putInt(1);
            }

            _info.release();
        } 
        catch(Exception e)
        {
            Log.i(TAG, "Error: " + e.toString());
        }

        return null;
    }

    public static ByteBuffer str_to_bb(String msg)
    {
        try{
            return encoder.encode(CharBuffer.wrap(msg));
        }catch(Exception e){e.printStackTrace();}

        return null;
    }

}
