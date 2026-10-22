package space.mooncellar.rooftops;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.provider.MediaStore;
import android.provider.OpenableColumns;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.SurfaceHolder;

import org.libsdl.app.SDLActivity;
import org.libsdl.app.SDLSurface;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class GameActivity extends SDLActivity {
    private static final int PICK_GAME = 4801;

    private static volatile String sPicked;
    private static volatile String sPickDir;

    @Override
    protected String[] getLibraries() {
        return new String[] {"SDL2", "main"};
    }

    private static volatile boolean sTurned;

    @Override
    public void setOrientationBis(int w, int h, boolean resizable, String hint) {
        setRequestedOrientation(sTurned ? ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED
                                        : ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
    }

    @Override
    protected SDLSurface createSDLSurface(Context context) {
        return new SDLSurface(context) {
            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                checkLetterbox(width, height);
                super.surfaceChanged(holder, format, width, height);
            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                nativeSurfaceDestroying();
                super.surfaceDestroyed(holder);
            }
        };
    }

    private static native void nativeSurfaceDestroying();

    private void checkLetterbox(int width, int height) {
        if (sTurned || isInMultiWindowMode()) return;
        DisplayMetrics real = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getRealMetrics(real);
        boolean portraitScreen = real.heightPixels > real.widthPixels;
        boolean band = width > height && (width < real.widthPixels || height < real.heightPixels);
        if (portraitScreen && band) {
            sTurned = true;
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
        }
    }

    public static void pickFile(String dir, String mime) {
        sPicked = "";
        sPickDir = dir;
        final SDLActivity activity = mSingleton;
        if (activity == null) {
            sPicked = "-";
            return;
        }
        activity.runOnUiThread(() -> {
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType(mime);
            try {
                activity.startActivityForResult(intent, PICK_GAME);
            } catch (Exception e) {
                sPicked = "-";
            }
        });
    }

    public static int displayRotation() {
        SDLActivity activity = mSingleton;
        if (activity == null) return 0;
        Display display = Build.VERSION.SDK_INT >= 30 ? activity.getDisplay()
                                                       : activity.getWindowManager().getDefaultDisplay();
        return display == null ? 0 : display.getRotation();
    }

    public static void restart() {
        SDLActivity activity = mSingleton;
        if (activity == null) return;
        Intent intent = new Intent(activity, RestartActivity.class);
        intent.putExtra(RestartActivity.PID, android.os.Process.myPid());
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        activity.startActivity(intent);
        Runtime.getRuntime().exit(0);
    }

    public static String saveToDownloads(String path, String name, String mime) {
        SDLActivity activity = mSingleton;
        if (activity == null || Build.VERSION.SDK_INT < 29) return null;
        String folder = Environment.DIRECTORY_DOWNLOADS + "/MoonCellar";
        ContentValues values = new ContentValues();
        values.put(MediaStore.MediaColumns.DISPLAY_NAME, name);
        values.put(MediaStore.MediaColumns.MIME_TYPE, mime);
        values.put(MediaStore.MediaColumns.RELATIVE_PATH, folder);
        ContentResolver resolver = activity.getContentResolver();
        Uri uri = resolver.insert(MediaStore.Downloads.EXTERNAL_CONTENT_URI, values);
        if (uri == null) return null;
        try (InputStream in = new FileInputStream(path); OutputStream out = resolver.openOutputStream(uri)) {
            if (out == null) throw new IOException("no stream");
            byte[] buf = new byte[1 << 16];
            int n;
            while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
        } catch (IOException | SecurityException e) {
            resolver.delete(uri, null, null);
            return null;
        }
        String shown = name;
        try (Cursor c = resolver.query(uri, new String[] {MediaStore.MediaColumns.DISPLAY_NAME}, null, null, null)) {
            if (c != null && c.moveToFirst() && c.getString(0) != null) shown = c.getString(0);
        } catch (Exception ignored) {
        }
        return folder + "/" + shown;
    }

    public static String pickedFile() {
        String picked = sPicked;
        return picked == null ? "-" : picked;
    }

    @Override
    protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        if (request != PICK_GAME) return;
        if (result != RESULT_OK || data == null || data.getData() == null) {
            sPicked = "-";
            return;
        }
        final Uri uri = data.getData();
        new Thread(() -> sPicked = copy(uri), "medecomp-copy-game").start();
    }

    private String copy(Uri uri) {
        String name = "game.ipa";
        try (Cursor c = getContentResolver().query(uri, new String[] {OpenableColumns.DISPLAY_NAME}, null, null, null)) {
            if (c != null && c.moveToFirst() && c.getString(0) != null) name = c.getString(0);
        } catch (Exception ignored) {
        }
        name = name.replace('/', '_').replace('\\', '_');
        File dir = new File(sPickDir);
        if (!dir.isDirectory() && !dir.mkdirs()) return "-";
        File part = new File(dir, name + ".part");
        try (InputStream in = getContentResolver().openInputStream(uri);
             OutputStream out = new FileOutputStream(part)) {
            if (in == null) return "-";
            byte[] buf = new byte[1 << 20];
            int n;
            while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
        } catch (IOException | SecurityException e) {
            part.delete();
            return "-";
        }
        File done = new File(dir, name);
        done.delete();
        return part.renameTo(done) ? done.getAbsolutePath() : "-";
    }
}
