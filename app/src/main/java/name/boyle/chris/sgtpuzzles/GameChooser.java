package name.boyle.chris.sgtpuzzles;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.util.DisplayMetrics;
import android.view.Menu;
import android.view.MenuItem;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentPagerAdapter;
import androidx.viewpager.widget.ViewPager;

import com.google.android.material.tabs.TabLayout;

import java.util.ArrayList;
import java.util.List;

public class GameChooser extends AppCompatActivity
{
	static final String CHOOSER_STYLE_KEY = "chooserStyle";
	private static final int REQ_CODE_PICKER = AppCompatActivity.RESULT_FIRST_USER;

	private Menu menu;
	private String[] tabs;
	final List<TabFragment> tabFragmentList = new ArrayList<>();

	@Override
    protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.chooser);
		TabLayout tabLayout = findViewById(R.id.tab_layout);
		ViewPager viewPager = findViewById(R.id.view_pager);
		tabs = new String[]{getResources().getString(R.string.my_favorites), getResources().getString(R.string.all_games)};
		for (String tab : tabs) {
			tabLayout.addTab(tabLayout.newTab().setText(tab));
			tabFragmentList.add(TabFragment.newInstance(tab));
		}
		viewPager.setAdapter(new FragmentPagerAdapter(getSupportFragmentManager(), FragmentPagerAdapter.BEHAVIOR_RESUME_ONLY_CURRENT_FRAGMENT) {
			@Override
			@NonNull
			public Fragment getItem(int position) { return tabFragmentList.get(position); }
			@Override
			public int getCount() { return tabFragmentList.size(); }
			@Override
			@Nullable
			public CharSequence getPageTitle(int position) { return tabs[position]; }
		});
		tabLayout.setupWithViewPager(viewPager,false);
		rethinkActionBarCapacity();
	}

	@Override
	protected void onResume() {
		super.onResume();
	}

	private void rethinkActionBarCapacity() {
		if (menu == null) return;
		DisplayMetrics dm = getResources().getDisplayMetrics();
		final int screenWidthDIP = (int) Math.round(((double) dm.widthPixels) / dm.density);
		int state = MenuItem.SHOW_AS_ACTION_ALWAYS;
		if (screenWidthDIP >= 480) {
			state |= MenuItem.SHOW_AS_ACTION_WITH_TEXT;
		}
		menu.findItem(R.id.settings).setShowAsAction(state);
		menu.findItem(R.id.load).setShowAsAction(state);
		menu.findItem(R.id.help_menu).setShowAsAction(state);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu)
	{
		super.onCreateOptionsMenu(menu);
		this.menu = menu;
		getMenuInflater().inflate(R.menu.chooser, menu);
		rethinkActionBarCapacity();
		return true;
	}

	@Override
	public boolean onPrepareOptionsMenu(Menu menu)
	{
		super.onPrepareOptionsMenu(menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item)
	{
		switch(item.getItemId()) {
			case R.id.settings:
				startActivity(new Intent(this, PrefsActivity.class));
				return true;
			case R.id.load:
				if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
					// GET_CONTENT would include Dropbox, but it returns file:// URLs that need SD permission :-(
					Intent picker = new Intent(Intent.ACTION_OPEN_DOCUMENT);
					picker.addCategory(Intent.CATEGORY_OPENABLE);
					picker.setType("*/*");
					picker.putExtra(Intent.EXTRA_MIME_TYPES, new String[] {"text/*", "application/octet-stream"});
					try {
						startActivityForResult(picker, REQ_CODE_PICKER);
					} catch (ActivityNotFoundException ignored) {
						SendFeedbackActivity.promptToReport(this, R.string.saf_missing_desc, R.string.saf_missing_short);
					}
				} else {
					FilePicker.createAndShow(this, Environment.getExternalStorageDirectory(), false);
				}
				return true;
			case R.id.contents:
				Intent intent = new Intent(this, HelpActivity.class);
				intent.putExtra(HelpActivity.TOPIC, "index");
				startActivity(intent);
				return true;
			case R.id.email:
				startActivity(new Intent(this, SendFeedbackActivity.class));
				return true;
			default:
				return super.onOptionsItemSelected(item);
		}
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent dataIntent) {
		super.onActivityResult(requestCode, resultCode, dataIntent);
		if (requestCode != REQ_CODE_PICKER || resultCode != Activity.RESULT_OK || dataIntent == null)
			return;
		final Uri uri = dataIntent.getData();
		startActivity(new Intent(Intent.ACTION_VIEW, uri, this, GamePlay.class));
		overridePendingTransition(0, 0);
	}
}
