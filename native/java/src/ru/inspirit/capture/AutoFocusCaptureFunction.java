package ru.inspirit.capture;

import android.util.Log;

import com.adobe.fre.FREContext;
import com.adobe.fre.FREFunction;
import com.adobe.fre.FREObject;

public class AutoFocusCaptureFunction implements FREFunction 
{
    private static final String TAG = "AutoFocusCaptureFunction";
    
    public static final String KEY = "focusAtPoint";
    
    @Override
    public FREObject call(FREContext context, FREObject[] args) 
    {

        try
        {
            CameraSurface capt = CaptureAndroidContext.cameraSurfaceHandler;

            if(capt != null)
            {
                double _x = args[1].getAsDouble();
                double _y = args[2].getAsDouble();
                capt.focusAtPoint(_x, _y);
            }
        } 
        catch(Exception e)
        {
            Log.i(TAG, "Error: " + e.toString());
        }

        return null;
    }

}
