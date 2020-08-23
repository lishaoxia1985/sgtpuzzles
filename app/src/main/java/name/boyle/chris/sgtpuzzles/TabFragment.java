package name.boyle.chris.sgtpuzzles;

import android.animation.LayoutTransition;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.LayerDrawable;
import android.net.Uri;
import android.os.Bundle;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.style.TextAppearanceSpan;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.cardview.widget.CardView;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.Fragment;
import androidx.gridlayout.widget.GridLayout;
import androidx.preference.PreferenceManager;

import java.util.HashMap;
import java.util.LinkedHashSet;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.atomic.AtomicInteger;

import static android.content.Context.MODE_PRIVATE;
import static name.boyle.chris.sgtpuzzles.GameChooser.CHOOSER_STYLE_KEY;

public class TabFragment extends Fragment implements SharedPreferences.OnSharedPreferenceChangeListener {
    String tabName;
    private SharedPreferences prefs;
    private boolean useGrid;
    private static final Set<String> DEFAULT_STARRED = new LinkedHashSet<>();
    private GridLayout table;
    private final HashMap<String,CardView> cardViewList = new HashMap<>();

    static {
        DEFAULT_STARRED.add("guess");
        DEFAULT_STARRED.add("keen");
        DEFAULT_STARRED.add("lightup");
        DEFAULT_STARRED.add("net");
        DEFAULT_STARRED.add("signpost");
        DEFAULT_STARRED.add("solo");
        DEFAULT_STARRED.add("towers");
    }

    public static TabFragment newInstance(String tabFragmentName) {
        TabFragment fragment = new TabFragment();
        Bundle args = new Bundle();
        args.putString("tabName", tabFragmentName);
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        tabName = getArguments().getString("tabName");
        prefs = PreferenceManager.getDefaultSharedPreferences(getContext());
        prefs.registerOnSharedPreferenceChangeListener(this);
        SharedPreferences state = getActivity().getSharedPreferences(GamePlay.STATE_PREFS_NAME, MODE_PRIVATE);
        String oldCS = state.getString(CHOOSER_STYLE_KEY, null);
        if (oldCS != null) {  // migrate to somewhere more sensible
            SharedPreferences.Editor ed = prefs.edit();
            ed.putString(CHOOSER_STYLE_KEY, oldCS);
            ed.apply();
            ed = state.edit();
            ed.remove(CHOOSER_STYLE_KEY);
            ed.apply();
        }
        useGrid = "grid".equals(prefs.getString(CHOOSER_STYLE_KEY, "list"));
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.tabfragment, container, false);
        table = view.findViewById(R.id.GridLayout);
        buildViews(table);
        return view;
    }

    @Override
    public void onConfigurationChanged(@NonNull Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        rethinkColumns();
    }

    void buildViews(GridLayout table)
    {
        String[] games = getResources().getStringArray(R.array.games);
        for(String gameId : games) {
            CardView view = (CardView) getLayoutInflater().inflate(
                    R.layout.list_item, table, false);
            final LayerDrawable starredIcon = mkStarryIcon(gameId);
            ((ImageView)view.findViewById(R.id.icon)).setImageDrawable(starredIcon);
            final int nameId = getResources().getIdentifier("name_"+gameId, "string", getActivity().getPackageName());
            final int descId = getResources().getIdentifier("desc_"+gameId, "string", getActivity().getPackageName());
            SpannableStringBuilder desc = new SpannableStringBuilder(nameId > 0 ?
                    getString(nameId) : gameId.substring(0,1).toUpperCase() + gameId.substring(1));
            desc.setSpan(new TextAppearanceSpan(getContext(), R.style.ChooserItemName),
                    0, desc.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
            desc.append(": ").append(getString(descId > 0 ? descId : R.string.no_desc));
            final TextView textView = view.findViewById(R.id.text);
            textView.setText(desc);
            textView.setVisibility(useGrid ? View.GONE : View.VISIBLE);
            view.setOnClickListener(v -> {
                Intent i1 = new Intent(getActivity(), GamePlay.class);
                i1.setData(Uri.fromParts("sgtpuzzles", gameId, null));
                startActivity(i1);
            });
            view.setOnLongClickListener(v -> {
                toggleStarred(gameId);
                GameChooser parentActivity = (GameChooser) getActivity();
                TabFragment myFavoritesFragment = parentActivity.tabFragmentList.get(0);
                TabFragment allGamesFragment = parentActivity.tabFragmentList.get(1);
                if(tabName.equals(getResources().getString(R.string.my_favorites)))
                    table.removeView(v);
                else if (isStarred(gameId))
                    myFavoritesFragment.table.addView(myFavoritesFragment.cardViewList.get(gameId));
                else
                    myFavoritesFragment.table.removeView(myFavoritesFragment.cardViewList.get(gameId));
                allGamesFragment.rethinkColumns();
                myFavoritesFragment.rethinkColumns();
                return true;
            });
            view.setFocusable(true);
            view.setLayoutParams(mkLayoutParams());
            cardViewList.put(gameId,view);
            if(tabName.equals(getResources().getString(R.string.all_games))){
                table.addView(view);
            }
            else if (isStarred(gameId)) {
                table.addView(view);
            }
        }
        enableTableAnimations(table);
        rethinkColumns();
    }

    private boolean isStarred(String game) {
        return prefs.getBoolean("starred_" + game, DEFAULT_STARRED.contains(game));
    }

    private void toggleStarred(String gameId) {
        SharedPreferences.Editor ed = prefs.edit();
        ed.putBoolean("starred_" + gameId, !isStarred(gameId));
        ed.apply();
    }

    private LayerDrawable mkStarryIcon(String gameId) {
        final int drawableId = getResources().getIdentifier(gameId, "drawable", getActivity().getPackageName());
        if (drawableId == 0) return null;
        final Drawable icon = ContextCompat.getDrawable(getContext(), drawableId);
        final LayerDrawable starredIcon = new LayerDrawable(new Drawable[]{
                icon, Objects.requireNonNull(ContextCompat.getDrawable(getContext(), R.drawable.ic_star)).mutate() });
        final float density = getResources().getDisplayMetrics().density;
        starredIcon.setLayerInset(1, (int)(42*density), (int)(42*density), 0, 0);
        return starredIcon;
    }

    private void enableTableAnimations(GridLayout table) {
        final LayoutTransition transition = new LayoutTransition();
        transition.enableTransitionType(LayoutTransition.CHANGING);
        table.setLayoutTransition(transition);
    }

    private void rethinkColumns() {
        DisplayMetrics dm = getResources().getDisplayMetrics();
        final int colWidthDipNeeded = useGrid ? 72 : 300;
        final double screenWidthDip = (double) dm.widthPixels / dm.density;
        final int columns = Math.max(1, (int) Math.floor(screenWidthDip / colWidthDipNeeded));
        final int colWidthActualPx = (int) Math.floor((double) dm.widthPixels / columns);
        mColumns = columns;
        mColWidthPx = colWidthActualPx;
        setViewsGridCells(cardViewList);
    }

    private void setViewsGridCells(final HashMap<String, CardView> views) {
        AtomicInteger col = new AtomicInteger();
        AtomicInteger row = new AtomicInteger();
        for (String gameId : views.keySet()) {
            if(tabName.equals(getResources().getString(R.string.my_favorites))){
                if (!isStarred(gameId)) continue;
            }
            final LayerDrawable layerDrawable = (LayerDrawable) ((ImageView) views.get(gameId).findViewById(R.id.icon)).getDrawable();
            if (layerDrawable != null) {
                final Drawable star = layerDrawable.getDrawable(1);
                star.setAlpha(isStarred(gameId) ? 255 : 0);
                if (col.get() >= mColumns) {
                    col.set(0);
                    row.getAndIncrement();
                }
            }
            setGridCells(views.get(gameId), col.getAndIncrement(), row.get());
        }
        row.getAndIncrement();
    }

    private int mColumns = 0;
    private int mColWidthPx = 0;

    private void setGridCells(View v, int x, int y) {
        final GridLayout.LayoutParams layoutParams = (GridLayout.LayoutParams) v.getLayoutParams();
        layoutParams.width = mColWidthPx;
        layoutParams.height = useGrid ? mColWidthPx + 10 : 250;
        layoutParams.columnSpec = GridLayout.spec(x, 1, GridLayout.CENTER);
        layoutParams.rowSpec = GridLayout.spec(y, 1, GridLayout.CENTER);
        layoutParams.setGravity(Gravity.START);
        v.setLayoutParams(layoutParams);
    }

    private GridLayout.LayoutParams mkLayoutParams() {
        final GridLayout.LayoutParams params = new GridLayout.LayoutParams();
        params.setGravity(Gravity.CENTER);
        return params;
    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
        if (!key.equals(CHOOSER_STYLE_KEY)) return;
        final boolean newGrid = "grid".equals(prefs.getString(CHOOSER_STYLE_KEY, "list"));
        if(useGrid == newGrid) return;
        useGrid = newGrid;
        for (CardView v : cardViewList.values()) {
            v.findViewById(R.id.text).setVisibility(useGrid ? View.GONE : View.VISIBLE);
        }
        rethinkColumns();
    }
}