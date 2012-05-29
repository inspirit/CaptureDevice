package ru.inspirit.capture;

import com.adobe.fre.FREContext;
import com.adobe.fre.FREExtension;

public class CaptureAndroid implements FREExtension {

	@Override
	public FREContext createContext(String arg0) {
		return new CaptureAndroidContext();
	}

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
	public void initialize() {
		// TODO Auto-generated method stub

	}

}
