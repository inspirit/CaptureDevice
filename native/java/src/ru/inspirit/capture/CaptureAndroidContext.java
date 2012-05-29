package ru.inspirit.capture;

import java.util.HashMap;
import java.util.Map;

import com.adobe.fre.FREContext;
import com.adobe.fre.FREFunction;

public class CaptureAndroidContext extends FREContext 
{
	
	public static CameraSurface cameraSurfaceHandler=null;

	@Override
	public void dispose() {
		// TODO Auto-generated method stub
		CameraSurface capt = CaptureAndroidContext.cameraSurfaceHandler;
		if(capt != null)
        {
        	capt.destroyCameraSurface();
        	CameraSurface.captureContext = null;
        	CaptureAndroidContext.cameraSurfaceHandler = null;
        }
	}

	@Override
	public Map<String, FREFunction> getFunctions() {
		Map<String, FREFunction> map = new HashMap<String, FREFunction>();
		
		map.put(CreateCaptureFunction.KEY, new CreateCaptureFunction());
		map.put(DisposeCaptureFunction.KEY, new DisposeCaptureFunction());
		map.put("releaseCapture", new DisposeCaptureFunction());
		map.put(GetCaptureFrameFunction.KEY, new GetCaptureFrameFunction());
		map.put(ListCaptureDevicesFunction.KEY, new ListCaptureDevicesFunction());
		map.put(ToggleCapturingFunction.KEY, new ToggleCapturingFunction());
		map.put(SupportsCameraRollFunction.KEY, new SupportsCameraRollFunction());
		map.put(SaveToCameraRollFunction.KEY, new SaveToCameraRollFunction());
		map.put(AutoFocusCaptureFunction.KEY, new AutoFocusCaptureFunction());
		map.put(CaptureStillImageFunction.KEY, new CaptureStillImageFunction());
		map.put(GrabStillImageFunction.KEY, new GrabStillImageFunction());
		
		return map;
	}

}
