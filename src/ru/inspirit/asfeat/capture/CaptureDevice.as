package ru.inspirit.asfeat.capture 
{
	import flash.display.BitmapData;
	import flash.events.Event;
	import flash.events.EventDispatcher;
	import flash.external.ExtensionContext;
	import flash.utils.ByteArray;
	import flash.utils.Endian;
	
	/**
	 * Class for video capturing from cameras
	 * @author Eugene Zatepyakin
	 */
	
	public final class CaptureDevice extends EventDispatcher
	{
		protected static var _context:ExtensionContext;
        protected static var _info_buff:ByteArray;
		
		public static const GET_FRAME_BITMAP:int = 2;// 1 << 1;
		public static const GET_FRAME_RAW_BYTES:int = 4;// 1 << 2;
		
        protected var _id:int = -1;
        protected var _deviceName:String;
		protected var _width:int;
		protected var _height:int;
        protected var _isCapturing:Boolean = false;
		
		public var bmp:BitmapData;
		public var raw:ByteArray;
		
		/**
		 * Try to start provided Device.
		 * if everything runs OK u will get new instance
		 * otherwise u have to catch an Error event
		 * 
		 * please note that result video frame width\height may differ from requested
		 * 
		 * @param	deviceName 	wanted device name received by CaptureDevice.getDevices method
		 * @param	width 		desired video frame width
		 * @param	height 		desired video frame width
		 */
		public function CaptureDevice(deviceName:String, width:int, height:int) 
		{
			var res:Boolean = _init(deviceName, width, height);
			if (!res)
			{
				throw new Error('CaptureDevice: ' + deviceName + ' failed to start');
			}
		}
		
		/**
		 * Begin capturing video
		 */
		public function start():void
		{
			_context.call('toggleCapturing', _id, 1);
            _isCapturing = true;
		}
		
		/**
		 * Stop capturing video
		 */
		public function stop():void
		{
			_context.call('toggleCapturing', _id, 0);
            _isCapturing = false;
		}
		
		/**
		 * Update Device data if new frame is available
		 * u should provide what actions to perform
		 * 
		 * Returns whether there is a new video frame available since the last call
		 * 
		 * @param	options 	specify what data to gather. use CaptureDevice.GET_FRAME_BITMAP and/or CaptureDevice.GET_FRAME_RAW_BYTES
		 * @return 	true if new frame was received otherwise false
		 */
		public function requestFrame(options:int = GET_FRAME_BITMAP):Boolean
		{
			var isNewFrame:int = _context.call('getCaptureFrame', _id, options, bmp, raw) as int;
			
			// we lost device - happens when u unplug the cam during capturing
			//
			// on Mac system i wasnt able to dispose it without hanging the whole app
			// smth is wrong there and i dont see how to handle it
			// so it is better to remove hanged device from update list but dont try to dispose it
			//
			// on Windows machines disposing hanged device works OK.
			// so as soon as u get this message u can dispose it and remove from update list
			if (isNewFrame == -1)
			{
                _isCapturing = false;
				dispatchEvent( new Event('CAPTURE_DEVICE_LOST', false, true) );
			}
			
			return isNewFrame == 1;
		}
		
		/**
		 * Dispose current Device instance internally
		 * and clear instance objects
		 */
		public function dispose():void
		{
			_context.call('releaseCapture', _id);
            _isCapturing = false;
            _disposeData();
		}
		
		/**
		 * Initialize native side and prepare internal buffer.
		 */
		public static function initialize():void
		{
			if (!_context)
			{
				_context = ExtensionContext.createExtensionContext("ru.inspirit.asfeat.capture", null);
				
				_info_buff = new ByteArray();
				_info_buff.endian = Endian.LITTLE_ENDIAN;
				_info_buff.length = 64 * 1024;
			}
		}
		
		/**
		 * Check if extension was initialized and ready to use
		 */
		public static function get available():Boolean
        {
            if(!_context) return false;
            return true;
        }

		/**
		 * Dispose native extension
		 * after calling u should start with initialize method
		 */
        public static function unInitialize():void
        {
            _context.call('disposeANE');
            _context.dispose();
            _context = null;
            _info_buff.length = 0;
            _info_buff = null;
        }
		
		/**
		 * Returns a vector of all Devices connected to the system. 
		 * If forceRefresh then the system will be polled for connected devices. 
		 */
		public static function getDevices(forceRefresh:Boolean = false):Vector.<CaptureDeviceInfo>
		{
			_info_buff.position = 0;
			_context.call('listDevices', forceRefresh ? 1 : 0, _info_buff);
			
			var n:int = _info_buff.readInt();
			var i:int;
			
			var devs:Vector.<CaptureDeviceInfo> = new Vector.<CaptureDeviceInfo>(n);
			
			for (i = 0; i < n; ++i)
			{
				var name_size:int = _info_buff.readInt();
				var name_str:String = _info_buff.readUTFBytes(name_size);
				var available:Boolean = Boolean(_info_buff.readInt());
				var connected:Boolean = Boolean(_info_buff.readInt());
				var dev:CaptureDeviceInfo = new CaptureDeviceInfo();
				dev.name = name_str;
				dev.available = available;
				dev.connected = connected;
				
				devs[i] = dev;
			}
			
			return devs;
		}
		
		internal function _init(deviceName:String, width:int, height:int):Boolean
		{
			_info_buff.position = 0;
			_context.call('getCapture', deviceName, width, height, _info_buff);
			
			_id = _info_buff.readInt();
			
			if (_id != -1)
			{
				_width = _info_buff.readInt();
				_height = _info_buff.readInt();
				
				_deviceName = deviceName;
                _isCapturing = true; // always autostarts
				
				_disposeData();
				
				bmp = new BitmapData(_width, _height, false, 0x0);
				raw = new ByteArray();
				raw.endian = Endian.LITTLE_ENDIAN;
				raw.length = _width * _height * 3; // always fixed to be RGB internaly
			}
			
			return _id != -1;
		}

        internal function _disposeData():void
        {
            if (bmp)
            {
                bmp.dispose();
                raw.length = 0;
                bmp = null;
                raw = null;
            }
        }
		
		public function get id():int
		{
			return _id;
		}
		
		public function get width():int
		{
			return _width;
		}
		
		public function get height():int
		{
			return _height;
		}

        public function get name():String
        {
            return _deviceName;
        }

        public function get isCapturing():Boolean
        {
            return _isCapturing;
        }

        override public function toString():String
        {
            return 'CaptureDevice { '+ _deviceName +' } { ' + _width + 'x' + _height + ' }';
        }
		
	}

}