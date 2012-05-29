package ru.inspirit.capture;

import android.util.Log;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import com.adobe.fre.FREContext;
import com.adobe.fre.FREFunction;
import com.adobe.fre.FREObject;
import com.adobe.fre.FREByteArray;

public class CreateCaptureFunction implements FREFunction 
{
	private static final String TAG = "CreateCaptureFunction";
	
	public static final String KEY ="getCapture";
	
	@Override
	public FREObject call(FREContext context, FREObject[] args) 
	{
        String _name;
        int _width;
        int _height;
        int _fps;
        int _pictureQuality;

        FREByteArray _info;
        ByteBuffer bytes;
        CameraSurface capt = null;

		try
        {
            _name = args[0].getAsString();
            _width = args[1].getAsInt();
            _height = args[2].getAsInt();
            _fps = args[3].getAsInt();
            _pictureQuality = args[5].getAsInt();

			CameraSurface.captureContext = context;

            Log.i(TAG, "Try Open Device: " + _name + ", " + _width + ", " + _height);

            CaptureAndroidContext.cameraSurfaceHandler = new CameraSurface(context.getActivity());
            capt = CaptureAndroidContext.cameraSurfaceHandler;
            if(_name.compareToIgnoreCase("Back Camera") == 0)
            { 
                capt.createCameraSurface(0);
            } else {
                capt.createCameraSurface(1);
            }
            capt.setupCameraSurface(_width, _height, _fps, _pictureQuality);

            _info = (FREByteArray)args[4];
            _info.acquire();
            bytes = _info.getBytes();
            bytes.order(ByteOrder.LITTLE_ENDIAN);

            bytes.putInt(0);
            bytes.putInt(capt.getFrameWidth());
            bytes.putInt(capt.getFrameHeight());

            _info.release();

            Log.i(TAG, "Inited Device: " + _name + ", " + capt.getFrameWidth() + ", " + capt.getFrameHeight());
        } 
        catch(Exception e)
        {
            Log.i(TAG, "Opening Device Error: " + e.toString());

            if(capt != null)
            {
                capt.destroyCameraSurface();
                CameraSurface.captureContext = null;
                CaptureAndroidContext.cameraSurfaceHandler = null;
            }

            _info = (FREByteArray)args[4];
            try{
                _info.acquire();
                bytes = _info.getBytes();
                bytes.order(ByteOrder.LITTLE_ENDIAN);
                bytes.putInt(-1);
                _info.release();
            }
            catch(Exception e2)
            {
                Log.i(TAG, "Write Result Error: " + e2.toString());
            }
        }

		return null;
	}

}
