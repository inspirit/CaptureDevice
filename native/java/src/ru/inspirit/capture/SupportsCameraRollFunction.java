package ru.inspirit.capture;

import android.util.Log;

import com.adobe.fre.FREContext;
import com.adobe.fre.FREFunction;
import com.adobe.fre.FREObject;

public class SupportsCameraRollFunction implements FREFunction 
{
    private static final String TAG = "SupportsCameraRollFunction";
    
    public static final String KEY ="supportsSaveToCameraRoll";
    
    @Override
    public FREObject call(FREContext context, FREObject[] args) 
    {
        FREObject result = null;

        try
        {
            result = FREObject.newObject( 1 );
        }
        catch(Exception e)
        {
            Log.i(TAG, "Construct Result Object Error: " + e.toString());
        }

        return result;
    }

}