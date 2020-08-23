package name.boyle.chris.sgtpuzzles;

import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;

public class PrefsActivity extends AppCompatActivity
{
	static final String BACKEND_EXTRA = "backend";

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_preferences);
		getSupportFragmentManager().beginTransaction().add(R.id.add_fragment, new SettingFragment()).commit();
	}
}
