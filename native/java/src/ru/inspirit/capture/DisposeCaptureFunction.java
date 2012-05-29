package ru.inspirit.capture;

import android.util.Log;

import com.adobe.fre.FREContext;
import com.adobe.fre.FREFunction;
import com.adobe.fre.FREObject;

public class DisposeCaptureFunction implements FREFunction 
{
    private static final String TAG = "DisposeCaptureFunction";
    
    public static final String KEY ="disposeANE";
    
    @Override
    public FREObject call(FREContext context, FREObject[] args) 
    {
        Log.i(TAG, "disposing extension");

        CameraSurface capt = CaptureAndroidContext.cameraSurfaceHandler;
        if(capt != null)
        {
            capt.destroyCameraSurface();
            CameraSurface.captureContext = null;
            CaptureAndroidContext.cameraSurfaceHandler = null;
        }
        return null;
    }

}
