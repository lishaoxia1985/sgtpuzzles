package name.boyle.chris.sgtpuzzles;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.content.res.Resources;
import android.graphics.Color;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.MenuItem;
import android.webkit.WebChromeClient;
import android.webkit.WebResourceRequest;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import androidx.appcompat.app.AppCompatActivity;
import androidx.preference.PreferenceManager;

import java.io.IOException;
import java.text.MessageFormat;
import java.util.regex.Pattern;


public class HelpActivity extends AppCompatActivity {

	static final String TOPIC = "name.boyle.chris.sgtpuzzles.TOPIC";
	private static final Pattern ALLOWED_TOPICS = Pattern.compile("^[a-z]+$");
	private static final Pattern URL_SCHEME = Pattern.compile("^[a-z0-9]+:");
	private static final String NIGHT_MODE_KEY = "nightMode";
	private WebView webView;

	@Override
	@SuppressLint("SetJavaScriptEnabled")
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		Intent intent = getIntent();
		String topic = intent.getStringExtra(TOPIC);
		if (!ALLOWED_TOPICS.matcher(topic).matches()) {
			finish();
			return;
		}
		if (getSupportActionBar() != null) {
			getSupportActionBar().setDisplayHomeAsUpEnabled(true);
		}
		setContentView(R.layout.activity_help);
		webView = findViewById(R.id.webView);
		final boolean isNight = PreferenceManager.getDefaultSharedPreferences(this).getBoolean(NIGHT_MODE_KEY, false);
		webView.setWebChromeClient(new WebChromeClient() {
			public void onReceivedTitle(WebView w, String title) {
				getSupportActionBar().setTitle(getString(R.string.title_activity_help) + ": " + title);
			}

			// onReceivedTitle doesn't happen on back button :-(
			public void onProgressChanged(WebView w, int progress) {
				if (progress == 100)
					getSupportActionBar().setTitle(getString(R.string.title_activity_help) + ": " + w.getTitle());
			}
		});
		if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.LOLLIPOP) {
			webView.setWebViewClient(new WebViewClient() {
				@Override
				public boolean shouldOverrideUrlLoading(WebView view, WebResourceRequest request) {
					String url = request.getUrl().toString();
					if (url.startsWith("javascript:")) {
					return false;
					}
					if (url.startsWith("file:") || !URL_SCHEME.matcher(url).find()) {
						webView.loadUrl(url);
						return true;
					}
					// spawn other app
					startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(url)));
					return true;
				}
				@Override
				public void onPageFinished(WebView view, String url) {
					refreshNightNow(isNight);
				}
			});
		} else {
			webView.setWebViewClient(new WebViewClient() {
				@Override
				public boolean shouldOverrideUrlLoading(WebView view, String url) {
					if (url.startsWith("javascript:")) {
						return false;
					}
					if (url.startsWith("file:") || !URL_SCHEME.matcher(url).find()) {
						webView.loadUrl(url);
						return true;
					}
					// spawn other app
					startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(url)));
					return true;
				}
				@Override
				public void onPageFinished(WebView view, String url) {
					refreshNightNow(isNight);
				}
			});
		}
		final WebSettings settings = webView.getSettings();
		settings.setJavaScriptEnabled(true);
		settings.setAllowFileAccess(false);  // android_asset still works
		settings.setBlockNetworkImage(true);
		settings.setBuiltInZoomControls(true);
		settings.setBlockNetworkLoads(true);
		settings.setDisplayZoomControls(false);
		settings.setAllowContentAccess(false);
		final Resources resources = getResources();
		final String lang = Build.VERSION.SDK_INT >= Build.VERSION_CODES.N ? resources.getConfiguration().getLocales().get(0).getLanguage() : resources.getConfiguration().locale.getLanguage();
		String assetPath = helpPath(lang, topic);
		boolean haveLocalised = false;
		try {
			final String[] list = resources.getAssets().list(lang);
			for (String s : list)
				if (s.equals(topic + ".html")) {
					haveLocalised = true;
					break;
				}
		} catch (IOException ignored) {}
		if (!haveLocalised) assetPath = helpPath("en", topic);
		webView.loadUrl("file:///android_asset/" + assetPath);
	}

	private static String helpPath(String lang, String topic) {
		return MessageFormat.format("{0}/{1}.html", lang, topic);
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (event.getAction() == KeyEvent.ACTION_DOWN && event.getRepeatCount() == 0
				&& keyCode == KeyEvent.KEYCODE_BACK) {
			if (webView.canGoBack()) webView.goBack();
			else finish();
			return true;
		}
		return super.onKeyDown(keyCode, event);
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		int id = item.getItemId();
		if (id == android.R.id.home) {
			finish();
			return true;
		}
		return super.onOptionsItemSelected(item);
	}

	public void refreshNightNow(final boolean isNight) {
		webView.setBackgroundColor(isNight ? Color.BLACK : Color.WHITE);
		webView.loadUrl(isNight
				? "javascript:document.body.className += ' night';null;"
				: "javascript:document.body.className = " +
				"document.body.className.replace(/(?:^|\\s)night(?!\\S)/g, '');null;");
	}

	@Override
	protected void onPause() {
		super.onPause();
	}

	@Override
	protected void onResume() {
		super.onResume();
	}
}
