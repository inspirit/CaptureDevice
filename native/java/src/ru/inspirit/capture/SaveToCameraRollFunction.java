package ru.inspirit.capture;

import android.util.Log;
import android.os.Environment;
import android.net.Uri;
import android.media.MediaScannerConnection;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.FileChannel;
import java.io.File;
import java.io.FileOutputStream;

import com.adobe.fre.FREContext;
import com.adobe.fre.FREFunction;
import com.adobe.fre.FREObject;
import com.adobe.fre.FREByteArray;

public class SaveToCameraRollFunction implements FREFunction 
{
    private static final String TAG = "SaveToCameraRollFunction";
    
    public static final String KEY ="saveToCameraRoll";
    
    @Override
    public FREObject call(FREContext context, FREObject[] args) 
    {
        FREObject result = null;
        String _name = null;
        //int _size;
        FREByteArray _info;
        ByteBuffer bytes;

        try
        {
            _name = args[0].getAsString();
        }
        catch (Exception e) 
        {
            e.printStackTrace();
        }

        File root = Environment.getExternalStorageDirectory();
        File file = new File(root.getAbsolutePath() + "/" + _name);
        try 
        {
            file.createNewFile();
            FileChannel wChannel = new FileOutputStream(file).getChannel();
            
            _info = (FREByteArray)args[1];
            _info.acquire();
            bytes = _info.getBytes();
            bytes.order(ByteOrder.LITTLE_ENDIAN);

            wChannel.write(bytes);

            _info.release();
            wChannel.close();

            MediaScannerConnection.scanFile(context.getActivity(),
                    new String[] { file.toString() }, null,
                    new MediaScannerConnection.OnScanCompletedListener() {
                public void onScanCompleted(String path, Uri uri) {
                    Log.i("ExternalStorage", "Scanned " + path + ":");
                    Log.i("ExternalStorage", "-> uri=" + uri);
                }
            });

            result = FREObject.newObject( 1 );
        } 
        catch (Exception e) 
        {
            Log.i(TAG, "Writing File Error: " + e.toString());
        }

        return result;
    }

}