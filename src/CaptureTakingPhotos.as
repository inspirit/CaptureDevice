package  
{
    import flash.desktop.NativeApplication;
    import flash.desktop.SystemIdleMode;
    import flash.display.Sprite;
    import flash.display.Stage3D;
    import flash.display.StageAlign;
    import flash.display.StageQuality;
    import flash.display.StageScaleMode;
    import flash.display3D.Context3D;
    import flash.events.Event;
    import flash.events.MouseEvent;
    import flash.filesystem.File;
    import flash.geom.Rectangle;
    import flash.system.Capabilities;
    import flash.text.TextField;
    import flash.ui.Multitouch;
    import flash.ui.MultitouchInputMode;
    import flash.utils.ByteArray;
    import flash.utils.Endian;
    import ru.inspirit.capture.CaptureDevice;
    import ru.inspirit.capture.CaptureDeviceInfo;
    import ru.inspirit.gpu.image.GPUImage;
	
	/**
     * Combining CaptureDevice with GPUImage for
     * optimal perfomance on mobile platforms
     * 
     * using Mobiles only feature to take photos
     * 
     * @author Eugene Zatepyakin
     */
    [SWF(frameRate='30',backgroundColor='0xFFFFFF')]
    public final class CaptureTakingPhotos extends Sprite 
    {
        // we use a clip rect to limit texture size
        // since uploading textures is slow on mobiles
        // u can set it to null to see the difference
        public const clipRect:Rectangle = new Rectangle(0, 0, 512, 512);

        // Stage3D stuff
        public var context3D:Context3D;
        public var antiAlias:int = 0;
        public var enableDepthAndStencil:Boolean = false;
        public var stageW:int = 640;
        public var stageH:int = 480;
        
        // Capture stuff
        public var capture:CaptureDevice;
        public var devices:Vector.<CaptureDeviceInfo>;
        public var streamW:int = 640;
        public var streamH:int = 480;
        public var streamFPS:int = 15;

        // GPUImage stuff
        public var gpuImage:GPUImage;
        
        // debug
        protected var _txt:TextField;
        
        public function CaptureTakingPhotos() 
        {
            stage.scaleMode = StageScaleMode.NO_SCALE;
            stage.align = StageAlign.TOP_LEFT;
            stage.quality = StageQuality.LOW;

            stage.addEventListener(Event.DEACTIVATE, deactivate);
            Multitouch.inputMode = MultitouchInputMode.TOUCH_POINT;
            NativeApplication.nativeApplication.systemIdleMode = SystemIdleMode.KEEP_AWAKE;
            
            // save some CPU cicles
            mouseEnabled = false;
            mouseChildren = false;
            
            // for status debug
            _txt = new TextField();
            _txt.width = 200;
            _txt.multiline = true;
            _txt.autoSize = 'left';
            _txt.background = true;
            _txt.backgroundColor = 0x000000;
            _txt.textColor = 0xFFFFFF;
            _txt.x = 70;
            _txt.y = 0;
            addChild(_txt);
            //
            
            initCapture();
            
            // on click we run auto focus
            // on double we take photo and save to camera roll
            // probably not the best way since u can run auto focus 
            // and then by accident take a shot and i guess it wont look good :)
            // so it is better to have a special button for it...
            stage.doubleClickEnabled = true;
            stage.addEventListener(MouseEvent.DOUBLE_CLICK, onDoubleClick);
            stage.addEventListener(MouseEvent.CLICK, onStageClick);
        }
        
        protected function onStageClick(e:MouseEvent):void 
        {
            if (capture)
            {
                capture.focusAtPoint();
            }
        }
        
        protected function onDoubleClick(e:MouseEvent):void 
        {
            if (capture)
            {
                capture.addEventListener(CaptureDevice.EVENT_STILL_IMAGE_READY, onStillImageReady);
                capture.captureStillImage();
            }
        }
        
        protected function onStillImageReady(e:Event):void
        {
            capture.removeEventListener(CaptureDevice.EVENT_STILL_IMAGE_READY, onStillImageReady);
            
            // the result is JPEG file
            var ba:ByteArray = new ByteArray();
            ba.endian = Endian.LITTLE_ENDIAN;
            capture.grabStillImage(ba);
            ba.position = 0;
            
            // now lets save it to camera roll
            if(CaptureDevice.supportsSaveToCameraRoll())
            {
                var now:Date = new Date();
                var filename:String = "IMG_" + now.fullYear +
                                        now.month +
                                        now.day +
                                        now.hours +
                                        now.minutes +
                                        now.seconds + ".jpg";
                if (Capabilities.manufacturer.toLowerCase().indexOf('android') != -1)
                {
                    // on android we save to /sdcard/ folder by default
                    // so u can adjust path by simply changing filename
                    var fl:File = File.userDirectory.resolvePath("CaptureDevice");
                    if(!fl.exists) fl.createDirectory();

                    filename = 'CaptureDevice/' + filename;
                }
                CaptureDevice.saveToCameraRoll(filename, ba, ba.length);
            }
        }
        
        protected function initCapture():void
        {
            // static initializer
            // just loads native library/code to memory
            CaptureDevice.initialize();

            devices = CaptureDevice.getDevices(true);

            capture = null;

            // Back Camera on mobiles
            var dev:CaptureDeviceInfo = devices[0];
            //
            try
            {
                // u can change desired photo quality
                capture = new CaptureDevice(dev.name, streamW, streamH, streamFPS, 
                                                CaptureDevice.ANDROID_STILL_IMAGE_QUALITY_BEST);

                // result camera dimension may be different from requested
                streamW = capture.width;
                streamH = capture.height;
            }
            catch (err:Error)
            {
                _txt.text = "CAN'T CONNECT TO CAMERA :(";
            }

            if (capture)
            {
                // we will use bytearrays only
                // power of 2 for stage3D
                // and specify clipping rectangle so it will use smaller power of 2 texture size
                capture.setupForDataType(CaptureDevice.GET_POWER_OF_2_FRAME_BGRA_BYTES, clipRect);

                // request stage3d
                getContext(Context3DRenderMode.AUTO);
            }
        }
        
         protected function getContext(mode:String): void
        {
            context3D = null;
            var stage3D:Stage3D = stage.stage3Ds[0];
            stage3D.addEventListener(Event.CONTEXT3D_CREATE, onContextCreated);
            stage3D.requestContext3D(mode);
        }

        protected function onContextCreated(ev:Event): void
        {
            // resize to stage dimension
            stageW = stage.stageWidth;
            stageH = stage.stageHeight;

            // Setup context
            var stage3D:Stage3D = stage.stage3Ds[0];
            stage3D.removeEventListener(Event.CONTEXT3D_CREATE, onContextCreated);
            context3D = stage3D.context3D;
            context3D.configureBackBuffer(
                    stageW,
                    stageH,
                    antiAlias,
                    enableDepthAndStencil
            );

            gpuImage = new GPUImage();
            gpuImage.init(context3D, antiAlias, false, stageW, stageH, streamW, streamH, clipRect);
            
            // dispose allocated Texture BitmapData
            // we are going to use Capture ByteArrays
            gpuImage.disposeTextureBitmap();
            

            addEventListener(Event.ENTER_FRAME, render);
        }
        
        protected function render(e:Event):void
        {
            var isNewFrame:Boolean;

            isNewFrame = capture.requestFrame(CaptureDevice.GET_POWER_OF_2_FRAME_BGRA_BYTES);
            
            if(isNewFrame)
            {
                context3D.clear(0.5, 0.5, 0.5, 1.0);

                gpuImage.uploadBytes(capture.bgraP2Bytes, 0);
                gpuImage.render();
                
                context3D.present();
            }
        }
        
    }

}