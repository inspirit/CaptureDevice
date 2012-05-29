package ru.inspirit.capture;

import android.util.Log;

import com.adobe.fre.FREContext;
import com.adobe.fre.FREFunction;
import com.adobe.fre.FREObject;

public class CaptureStillImageFunction implements FREFunction 
{
    private static final String TAG = "CaptureStillImageFunction";
    
    public static final String KEY = "camShot";
    
    @Override
    public FREObject call(FREContext context, FREObject[] args) 
    {

        try
        {
            CameraSurface capt = CaptureAndroidContext.cameraSurfaceHandler;

            if(capt != null)
            {
                //capt.startTakePicture();
                capt.takePicture();
            }
        } 
        catch(Exception e)
        {
            Log.i(TAG, "Error: " + e.toString());
        }

        return null;
    }

}
