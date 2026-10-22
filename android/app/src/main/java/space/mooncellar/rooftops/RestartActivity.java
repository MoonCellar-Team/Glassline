package space.mooncellar.rooftops;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;

public class RestartActivity extends Activity {
    static final String PID = "pid";

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        int pid = getIntent().getIntExtra(PID, -1);
        if (pid > 0) android.os.Process.killProcess(pid);
        Intent game = new Intent(this, GameActivity.class);
        game.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
        startActivity(game);
        finish();
        Runtime.getRuntime().exit(0);
    }
}
