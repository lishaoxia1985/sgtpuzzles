package name.boyle.chris.sgtpuzzles;

import android.app.backup.BackupManager;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.os.Bundle;

import androidx.appcompat.app.AppCompatDelegate;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceManager;
import androidx.preference.SwitchPreference;

import java.text.MessageFormat;

import static name.boyle.chris.sgtpuzzles.PrefsActivity.BACKEND_EXTRA;

public class SettingFragment extends PreferenceFragmentCompat implements SharedPreferences.OnSharedPreferenceChangeListener
{
	private static final String NIGHT_MODE_KEY = "nightMode";
	private BackupManager backupManager = null;
	private SharedPreferences prefs;

	@Override
	public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
		prefs = PreferenceManager.getDefaultSharedPreferences(getContext());
		backupManager = new BackupManager(getContext());
		addPreferencesFromResource(R.xml.preferences);
		final String whichBackend = getActivity().getIntent().getStringExtra(BACKEND_EXTRA);
		final PreferenceCategory chooserCategory = findPreference("gameChooser");
		final PreferenceCategory thisGameCategory = findPreference("thisGame");
		if (whichBackend == null) {
			getPreferenceScreen().removePreference(thisGameCategory);
		} else {
			getPreferenceScreen().removePreference(chooserCategory);
			final int nameId = getResources().getIdentifier("name_" + whichBackend, "string", getContext().getPackageName());
			thisGameCategory.setTitle(nameId);
			if (!"bridges".equals(whichBackend)) thisGameCategory.removePreference(findPreference("bridgesShowH"));
			if (!"unequal".equals(whichBackend)) thisGameCategory.removePreference(findPreference("unequalShowH"));
			final Preference unavailablePref = findPreference("arrowKeysUnavailable");
			if (!("untangle" + "netslide" + "sixteen" + "loopy").contains(whichBackend)) {
				thisGameCategory.removePreference(unavailablePref);
				final Configuration configuration = getResources().getConfiguration();
				final SwitchPreference arrowKeysPref = new SwitchPreference(getContext());
				arrowKeysPref.setOrder(-1);
				arrowKeysPref.setKey(GamePlay.getArrowKeysPrefName(whichBackend, configuration));
				arrowKeysPref.setDefaultValue(GamePlay.getArrowKeysDefault(whichBackend, getResources(), getContext().getPackageName()));
				arrowKeysPref.setTitle(MessageFormat.format(getString(R.string.arrowKeysIn), getString(nameId)));
				thisGameCategory.addPreference(arrowKeysPref);
			} else {
				unavailablePref.setSummary(MessageFormat.format(getString(R.string.arrowKeysUnavailableIn), getString(nameId)));
			}
		}
		findPreference("about_content").setSummary(
				String.format(getString(R.string.about_content), BuildConfig.VERSION_NAME));
	}

	@Override
	public void onResume()
	{
		super.onResume();
		getPreferenceScreen().getSharedPreferences().registerOnSharedPreferenceChangeListener(this);
	}

	@Override
	public void onPause()
	{
		super.onPause();
		getPreferenceScreen().getSharedPreferences().unregisterOnSharedPreferenceChangeListener(this);
		backupManager.dataChanged();
	}

	@Override
	public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
		if (key.equals(NIGHT_MODE_KEY)) {
			applyNightMode();
		}
	}

	private void applyNightMode() {
		final boolean isNight = prefs.getBoolean(NIGHT_MODE_KEY, false);
		AppCompatDelegate.setDefaultNightMode(isNight ? AppCompatDelegate.MODE_NIGHT_YES : AppCompatDelegate.MODE_NIGHT_NO);
		getActivity().getWindow().setWindowAnimations(R.style.WindowAnimationFadeInOut);
	}
}
