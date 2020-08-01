package name.boyle.chris.sgtpuzzles;

import android.content.Intent;
import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;

public class RestartActivity extends AppCompatActivity
{
	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		startActivity(new Intent(this, GamePlay.class));
		finish();
	}
}
