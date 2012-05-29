package ru.inspirit.capture;

import java.io.IOException;
import java.util.List;
import java.nio.ByteBuffer;
import java.nio.IntBuffer;
import java.lang.Integer;

import android.content.Context;
import android.graphics.ImageFormat;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Camera.AutoFocusCallback;
import android.hardware.Camera.PictureCallback;
import android.hardware.Camera.ShutterCallback;
import android.os.Build;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import com.adobe.fre.FREContext;

public class CameraSurface extends SurfaceView implements SurfaceHolder.Callback, Runnable 
{

	private static final String TAG = "CameraSurface";

    public static FREContext captureContext = null;

    private int _frameDataSize;
    private int _frameDataSize2;
    private int _rawFrameDataSize;
    private int _jpegDataSize;

    private byte[] _RGBA;
    private byte[] _RGBA_P2;
    private byte[] _frameData;
    private byte[] _jpegData;
    private byte[] _callbackBuffer;

    private Camera              _camera;
    private SurfaceHolder       _holder;
    private int                 _frameWidth;
    private int                 _frameHeight;
    private int                 _frameWidth2;
    private int                 _frameHeight2;
    private int                 _pictureWidth;
    private int                 _pictureHeight;
    private boolean             _threadRun;

    public boolean              isNewFrame;
    public boolean              isCapturing;
    public Integer              previewFormat;
    public Integer              pictureFormat;

    //
    static {
        System.loadLibrary("color_convert");
    }
    private native static void convert(byte[] input, byte[] output, int width, int height, int format);
    private native static void setupConvert(int width, int height);
    private native static void disposeConvert();
    //
    
    public CameraSurface(Context context) 
    {
        super(context);

        _holder = getHolder();
        _holder.addCallback(this);
        _holder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);

        isNewFrame = false;
        isCapturing = false;

        Log.i(TAG, "Instantiated new " + this.getClass());
    }

    public int getFrameWidth() {
        return _frameWidth;
    }

    public int getFrameHeight() {
        return _frameHeight;
    }

    public void startPreview()
    {
        if(!isCapturing)
        {
            _camera.startPreview();
            isCapturing = true;
        }
    }
    public void stopPreview()
    {
        if(isCapturing)
        {
            _camera.stopPreview();
            isCapturing = false;
        }
    }
    public void focusAtPoint(double x, double y)
    {
        _camera.autoFocus(new AutoFocusCallback() {
                        @Override
                        public void onAutoFocus(boolean success, Camera camera) {
                            //
                        }
                });
    }
    public void startTakePicture()
    {
        _camera.autoFocus(new AutoFocusCallback() {
                @Override
                public void onAutoFocus(boolean success, Camera camera) {
                        takePicture();
                }
        });
    }
    public void takePicture() 
    {
        Log.i(TAG, "takePicture");
        //System.gc();
        _camera.takePicture(
                    null,
                    null, 
                    new PictureCallback() {
                            @Override
                            public void onPictureTaken(byte[] data, Camera camera){
                                //jpeg
                                Log.i(TAG, "on jpeg arrives");

                                _jpegDataSize = data.length;
                                System.arraycopy(data, 0, _jpegData, 0, _jpegDataSize);

                                // notify AIR
                                CameraSurface.captureContext.dispatchStatusEventAsync( "CAM_SHOT", "0" );

                                // resume camera
                                camera.startPreview();
                            }
                    });
    }

    protected void onPreviewStared(int previewWidth, int previewHeight) 
    {
        _frameDataSize = previewWidth * previewHeight * 4;
        _RGBA = new byte[_frameDataSize + 4096]; // safe


        _frameWidth2 = nextPowerOfTwo(_frameWidth);
        _frameHeight2 = nextPowerOfTwo(_frameHeight);
        _frameDataSize2 = _frameWidth2 * _frameHeight2 * 4;
        _RGBA_P2 = new byte[_frameDataSize2];

        // jpeg buffer
        int jpeg_size = _pictureWidth * _pictureHeight;
        _jpegData = new byte[jpeg_size * 9]; // should be enough for jpeg

        setupConvert(previewWidth, previewHeight);

        isCapturing = true;
    }

    protected void onPreviewStopped() 
    {
        _RGBA = null;
        _RGBA_P2 = null;
        _jpegData = null;
        isCapturing = false;

        disposeConvert();
    }

    protected void processFrame(byte[] data) 
    {
        if(previewFormat == ImageFormat.NV21)
        {
            convert(data, _RGBA, _frameWidth, _frameHeight, 0);
        } 
        else if(previewFormat == ImageFormat.YV12) // doesnt work
        {
            convert(data, _RGBA, _frameWidth, _frameHeight, 1);
        }

        isNewFrame = true;

        debugShowFPS();
    }

    public void grabFrame(ByteBuffer bytes)
    {
        bytes.put(_RGBA, 0, _frameDataSize);
    }

    public void grabP2Frame(ByteBuffer bytes, int w2, int h2)
    {
        int off_x = (w2 - _frameWidth) >> 1;
        int off_y = (h2 - _frameHeight) >> 1;
        int p2stride = w2 * 4;
        int stride = _frameWidth * 4;

        int b_off_x, b_off_y, a_off_x, a_off_y;

        if(off_x < 0)
        {
            b_off_x = 0;
            a_off_x = -off_x;
        } else {
            b_off_x = off_x;
            a_off_x = 0;
        }

        if(off_y < 0)
        {
            b_off_y = 0;
            a_off_y = -off_y;
        } else {
            b_off_y = off_y;
            a_off_y = 0;
        }

        int nw = _frameWidth - a_off_x*2;
        int nh = _frameHeight - a_off_y*2;
        int new_stride = nw*4;

        int offset = b_off_y * p2stride + b_off_x*4;
        int src_pos = a_off_y * stride + a_off_x*4;

        for(int i = 0; i < nh; ++i)
        {
            System.arraycopy(_RGBA, src_pos, _RGBA_P2, offset, new_stride);
            offset += p2stride;
            src_pos += stride;
        }

        bytes.put(_RGBA_P2, 0, p2stride*h2);
    }

    public void grabRawFrame(ByteBuffer bytes)
    {
        bytes.put(_frameData, 0, _rawFrameDataSize);
    }

    public int getJpegDataSize()
    {
        return _jpegDataSize;
    }
    public void grabJpegFrame(ByteBuffer bytes)
    {
        bytes.put(_jpegData, 0, _jpegDataSize);
    }

    public void setPreview() throws IOException 
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB)
        {
            _camera.setPreviewTexture( new SurfaceTexture(10) );
        } else {
            _camera.setPreviewDisplay(null);
        }
    }

    public void setupCameraSurface(int width, int height, int fps, int pictureQuality) 
    {
        Log.i(TAG, "setupCameraSurface: " + width + "/" + height);
        if (_camera != null) 
        {
            Camera.Parameters params = _camera.getParameters();
            List<Camera.Size> sizes = params.getSupportedPreviewSizes();
            _frameWidth = width;
            _frameHeight = height;

            // selecting optimal camera preview size
            {
                int  minDiff = Integer.MAX_VALUE;
                for (Camera.Size size : sizes) {
                    if (Math.abs(size.height - height) < minDiff) {
                        _frameWidth = size.width;
                        _frameHeight = size.height;
                        minDiff = Math.abs(size.height - height);
                    }
                }
            }

            params.setPreviewSize(_frameWidth, _frameHeight);
            //

            // setup Picture size
            // i simply choose from middle :)
            sizes = params.getSupportedPictureSizes();
            int cnt = sizes.size();
            if(pictureQuality == 1)
            {
                _pictureWidth = sizes.get(cnt>>1).width;
                _pictureHeight = sizes.get(cnt>>1).height;
            }
            else if(pictureQuality == 2)
            {
                _pictureWidth = sizes.get(cnt-1).width;
                _pictureHeight = sizes.get(cnt-1).height;
            }

            params.setPictureSize(_pictureWidth, _pictureHeight);
            //

            List<String> FocusModes = params.getSupportedFocusModes();
            if (FocusModes.contains(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO))
            {
                params.setFocusMode(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO);
                Log.i(TAG, "setFocusMode: FOCUS_MODE_CONTINUOUS_VIDEO");
            }
            else if(FocusModes.contains(Camera.Parameters.FOCUS_MODE_AUTO))
            {
                params.setFocusMode(Camera.Parameters.FOCUS_MODE_AUTO);
                Log.i(TAG, "setFocusMode: FOCUS_MODE_AUTO");
            }

            // set fps
            List<int[]> fps_ranges = params.getSupportedPreviewFpsRange();
            int des_fps = fps * 1000;
            int min_fps = 0;
            int max_fps = 0;
            // selecting optimal camera fps range
            {
                int minDiff = Integer.MAX_VALUE;
                for (int[] fps_range : fps_ranges) {
                    int dnf = fps_range[0] - des_fps;
                    int dxf = fps_range[1] - des_fps;
                    if (dnf*dnf + dxf*dxf < minDiff) {
                        min_fps = fps_range[0];
                        max_fps = fps_range[1];
                        minDiff = dnf*dnf + dxf*dxf;
                    }
                }
            }
            params.setPreviewFpsRange(min_fps, max_fps);
            Log.i(TAG, "setPreviewFpsRange: " + min_fps + "/" + max_fps);

            // preview format
            /*
            List<Integer> preview_fmt = params.getSupportedPreviewFormats();
            if(preview_fmt.contains(ImageFormat.YV12))
            {
                params.setPreviewFormat(ImageFormat.YV12);
                Log.i(TAG, "setPreviewFormat: YV12");
            } 
            else if(preview_fmt.contains(ImageFormat.NV21))
            {
                params.setPreviewFormat(ImageFormat.NV21);
                Log.i(TAG, "setPreviewFormat: NV21");
            }
            */

            // picture format
            List<Integer> pict_fmt = params.getSupportedPictureFormats();
            {
                for (Integer fmt : pict_fmt) {
                    Log.i(TAG, "picture fmt: " + fmt);
                }
            }
            if(pict_fmt.contains(ImageFormat.JPEG))
            {
                params.setPictureFormat(ImageFormat.JPEG);
                Log.i(TAG, "setPictureFormat: JPEG");
            } 


            params.setPreviewFormat(ImageFormat.NV21);
            // too much memory needed
            // + not possible on half of devices
            //params.setPictureFormat(ImageFormat.NV21);

            _camera.setParameters(params);

            previewFormat = params.getPreviewFormat();
            pictureFormat = params.getPictureFormat();

            Log.i(TAG, "selected previewFormat: " + previewFormat);
            Log.i(TAG, "selected pictureFormat: " + pictureFormat);
            Log.i(TAG, "selected preview size: " + _frameWidth + "x" + _frameHeight);
            Log.i(TAG, "selected picture size: " + _pictureWidth + "x" + _pictureHeight);

            /* Now allocate the buffer */

            _rawFrameDataSize = _frameWidth * _frameHeight;
            _rawFrameDataSize = _rawFrameDataSize * ImageFormat.getBitsPerPixel(previewFormat) / 8;
            _callbackBuffer = new byte[_rawFrameDataSize];
            /* The buffer where the current frame will be copied */
            _frameData = new byte [_rawFrameDataSize];
            _camera.addCallbackBuffer(_callbackBuffer);

            try {
                setPreview();
            } catch (IOException e) {
                Log.e(TAG, "setPreviewDisplay/setPreviewTexture fails: " + e);
            }

            /* Notify that the preview is about to be started and deliver preview size */
            onPreviewStared(_frameWidth, _frameHeight);

            /* Now we can start a preview */
            _camera.startPreview();
        }
    }

    public void createCameraSurface(int index) 
    {
        Log.i(TAG, "createCameraSurface: " + index);

        if (_camera != null) 
        {
            _threadRun = false;
            synchronized (this) {
                _camera.stopPreview();
                _camera.setPreviewCallback(null);
                _camera.release();
                _camera = null;
            }
            onPreviewStopped();
        }
        
        synchronized (this) 
        {
            _camera = Camera.open(index);

            _camera.setPreviewCallbackWithBuffer(new PreviewCallback() {
                public void onPreviewFrame(byte[] data, Camera camera) {
                    synchronized (CameraSurface.this) {
                        System.arraycopy(data, 0, _frameData, 0, data.length);
                        CameraSurface.this.notify(); 
                    }
                    camera.addCallbackBuffer(_callbackBuffer);
                }
            });
        }
                    
        (new Thread(this)).start();
    }

    public void destroyCameraSurface() 
    {
        Log.i(TAG, "destroyCameraSurface");
        _threadRun = false;
        if (_camera != null) {
            synchronized (this) {
                _camera.stopPreview();
                _camera.setPreviewCallback(null);
                _camera.release();
                _camera = null;
            }
        }
        onPreviewStopped();
    }

    public void run() 
    {
        _threadRun = true;
        Log.i(TAG, "Starting processing thread");
        while (_threadRun) 
        {
            synchronized (this) {
                try {
                    this.wait();
                    processFrame(_frameData);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    private static int frameCounter = 0;
    private static long lastFpsTime = System.currentTimeMillis();
    private static float camFps = 0;
    private void debugShowFPS()
    {
        frameCounter++;
        int delay = (int)(System.currentTimeMillis() - lastFpsTime);
        if (delay > 1000) 
        {
            camFps = (((float)frameCounter)/delay)*1000;
            frameCounter = 0;
            lastFpsTime = System.currentTimeMillis();
            Log.i(TAG, "### Camera FPS ### " + camFps + " FPS");  
        }
    }

    private static int nextPowerOfTwo(int v)
    {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v++;
        return v;
    }

    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) 
    {
        Log.i(TAG, "surfaceChanged");
    }
    public void surfaceCreated(SurfaceHolder holder) 
    {
        Log.i(TAG, "surfaceCreated");
    }
    public void surfaceDestroyed(SurfaceHolder holder) 
    {
        Log.i(TAG, "surfaceDestroyed");
        destroyCameraSurface();
    }
}
