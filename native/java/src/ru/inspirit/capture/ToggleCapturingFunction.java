package ru.inspirit.capture;

import android.util.Log;

import com.adobe.fre.FREContext;
import com.adobe.fre.FREFunction;
import com.adobe.fre.FREObject;

public class ToggleCapturingFunction implements FREFunction 
{
    private static final String TAG = "ToggleCapturingFunction";
    
    public static final String KEY = "toggleCapturing";
    
    @Override
    public FREObject call(FREContext context, FREObject[] args) 
    {
        int _opt = 0;

        try
        {
            _opt = args[1].getAsInt();

            CameraSurface capt = CaptureAndroidContext.cameraSurfaceHandler;

            if(_opt == 1 && capt != null)
            {
                capt.startPreview();
            }
            else if(_opt == 0 && capt != null)
            {
                capt.stopPreview();
            }
        } 
        catch(Exception e)
        {
            Log.i(TAG, "Error: " + e.toString());
        }

        return null;
    }

}
