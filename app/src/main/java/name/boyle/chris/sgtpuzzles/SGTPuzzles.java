package name.boyle.chris.sgtpuzzles;

import android.content.Intent;
import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.TaskStackBuilder;

public class SGTPuzzles extends AppCompatActivity
{
	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		final Intent intent = getIntent();
		intent.setClass(this, GamePlay.class);
		TaskStackBuilder.create(this)
				.addNextIntentWithParentStack(intent)
				.startActivities();
		finish();
	}
}
