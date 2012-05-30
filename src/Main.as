package 
{
    import flash.display.Bitmap;
    import flash.display.BitmapData;
    import flash.display.Sprite;
    import flash.display.StageAlign;
    import flash.display.StageScaleMode;
    import flash.events.Event;
    import flash.events.MouseEvent;
    import flash.geom.Point;
    import flash.text.TextField;
    import flash.text.TextFormat;
    import ru.inspirit.capture.CaptureDevice;
    import ru.inspirit.capture.CaptureDeviceInfo;
    
    /**
     * Simple Demo App
     * @author Eugene Zatepyakin
     */
    [SWF(frameRate='30',backgroundColor='0xFFFFFF')]
    public final class Main extends Sprite 
    {
        public var captDevs:Vector.<CaptureDevice>;
        public var captLabs:Vector.<TextField>;
        public var _txt:TextField;

        public var scrBitmap:Bitmap;
        public var scr_bmp:BitmapData;

        public var dev_info:String;
        
        public function Main():void 
        {
            stage.scaleMode = StageScaleMode.NO_SCALE;
            stage.align = StageAlign.TOP_LEFT;

            scr_bmp = new BitmapData(800, 600, false, 0xFFFFFF);
            scrBitmap = new Bitmap(scr_bmp, 'auto', true);
            addChild(scrBitmap);
            
            //
            _txt = new TextField();
            _txt.width = 200;
            _txt.multiline = true;
            _txt.autoSize = 'left';
            _txt.background = true;
            _txt.backgroundColor = 0xffffff;
            _txt.x = 320;
            _txt.y = 240;
            
            addChild(_txt);
            //
            
            initCapture();
        }
        
        protected function createCaptLab(capt:CaptureDevice):TextField
        {
            var lab:TextField;
            
            var fmt:TextFormat = new TextFormat('_sans',9, 0xffffff,false,false,false,null,null,'left',4,6);
            
            lab = new TextField();
            lab.background = true;
            lab.backgroundColor = 0x0;
            lab.autoSize = 'left';
            lab.selectable = false;                                                   
            lab.text = capt.name.toUpperCase()  + ' // ' + capt.width + 'x'+capt.height;
            lab.setTextFormat(fmt);

            lab.addEventListener(MouseEvent.CLICK, onLabClick);
            
            return lab;
        }
        private function onLabClick(e:MouseEvent):void
        {
            var lab:TextField = e.currentTarget as TextField;
            var ind:int = captLabs.indexOf(lab);
            if(ind >= 0 && ind < captDevs.length)
            {
                var capt:CaptureDevice = captDevs[ind];
                if(capt.isCapturing)
                {
                    capt.stop();
                } else {
                    capt.start();
                }
            }
        }
        
        protected function initCapture():void
        {
            CaptureDevice.initialize();
            
            dev_info = '';
            dev_info += 'CaptureInterface init: ' + CaptureDevice.available + '\n';
            
            var dev_inf:Vector.<CaptureDeviceInfo> = CaptureDevice.getDevices(false);
            captDevs = new Vector.<CaptureDevice>();
            captLabs = new Vector.<TextField>();
            
            var capt:CaptureDevice;
            var lab:TextField;
            
            var n:int = dev_inf.length;
            for (var i:int = 0; i < n; ++i)
            {
                var dev:CaptureDeviceInfo = dev_inf[i];
                dev_info += 'Device[' + i + ']: ' + '{ name: '+ dev.name + ', connected: ' + dev.connected +', available: ' + dev.available +' }\n';
                dev_info += '\ttrying to init: ';
                
                try 
                {
                    capt = new CaptureDevice(dev.name, 320, 240);
                    capt.addEventListener('CAPTURE_DEVICE_LOST', onDeviceLost);
					
					// setup for specific data
					capt.setupForDataType(CaptureDevice.GET_FRAME_BITMAP);
                    
                    lab = createCaptLab(capt);

                    captLabs.push(lab);
                    addChild(lab);
                    
                    captDevs.push(capt);
                    dev_info += 'OK';
                }
                catch (err:Error)
                {
                    dev_info += 'failed :(';
                }
                
                dev_info += '\n';
            }
            
            // dirty test of dispose method
            /*
            setTimeout(function():void
            {
                captDevs[1].dispose();
                removeChild(captLabs[1]);
                captDevs.splice(1, 1);
                captLabs.splice(1, 1);
            }, 5000);
            */

            if (captDevs.length)
            {
                addEventListener(Event.ENTER_FRAME, updateFrame);
            }
            
            _txt.text = dev_info;
        }
        
        private function onDeviceLost(e:Event):void 
        {
            var capt:CaptureDevice = e.currentTarget as CaptureDevice;

            // please be carefull
            // this causes app hand on Mac but works on Windows
            capt.dispose();
            
            var n:int = captDevs.length;
            for (var i:int = 0; i < n; ++i)
            {
                if (captDevs[i].id == capt.id)
                {
                    captDevs.splice(i, 1);

                    captLabs[i].removeEventListener(MouseEvent.CLICK, onLabClick);
                    removeChild(captLabs[i]);
                    captLabs.splice(i,  1);

                    break;
                }
            }

            capt.removeEventListener('CAPTURE_DEVICE_LOST', onDeviceLost);
            
            e.stopImmediatePropagation();
        }
        
        private function updateFrame(e:Event):void 
        {
            var n:int = captDevs.length;
            var capt:CaptureDevice;
            var lab:TextField;
            var isNewFrame:Boolean;
            var pt:Point = new Point();

            var sw:Number = stage.stageWidth;
            var sh:Number = stage.stageHeight;

            var _str:String = '\n';
            for (var i:int = 0; i < n; ++i)
            {
                capt = captDevs[i];
                isNewFrame = capt.requestFrame(CaptureDevice.GET_FRAME_BITMAP);
                //isNewFrame = capt.requestFrame(CaptureDevice.GET_FRAME_RAW_BYTES);
                //isNewFrame = capt.requestFrame(CaptureDevice.GET_FRAME_RAW_BYTES | CaptureDevice.GET_FRAME_BITMAP);
                
                _str += 'device[' + capt.name + '] update ' + isNewFrame + '\n';
                
                if(isNewFrame)
                {
                    pt.x = 320*(i%2);
                    pt.y = 240*(i>>1);
                    
                    scr_bmp.copyPixels(capt.bmp, capt.bmp.rect, pt);
                    
                    lab = captLabs[i];
                    lab.x = pt.x;
                    lab.y = pt.y;
                }
            }

            _txt.text = dev_info + _str;
        }
        
    }
    
}