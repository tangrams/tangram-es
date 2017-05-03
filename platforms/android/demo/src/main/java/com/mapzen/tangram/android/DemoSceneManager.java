package com.mapzen.tangram.android;

import android.content.Context;
import android.support.annotation.NonNull;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.AutoCompleteTextView;
import android.widget.Filter;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * DemoSceneManager handles entering a scene file URL in a text view using a list of suggestions.
 */

class DemoSceneManager implements AdapterView.OnItemClickListener, TextView.OnEditorActionListener {

    interface LoadSceneCallback {
        void loadSceneCallback(String scene);
    }

    // Override ArrayAdaptor to include any text
    private class SceneAutoCompleteArrayAdapter extends ArrayAdapter {

        List<String> suggestions;

        Filter nameFilter = new Filter() {
            @Override
            public CharSequence convertResultToString(Object resultValue) {
                return resultValue.toString().toLowerCase();
            }

            @Override
            protected FilterResults performFiltering(CharSequence constraint) {
                if (constraint != null) {
                    suggestions.clear();
                    for (String scene: scenes) {
                        if (scene.toLowerCase().contains(constraint.toString().toLowerCase())) {
                            suggestions.add(scene);
                        }
                    }

                    FilterResults filterResults = new FilterResults();
                    filterResults.values = suggestions;
                    filterResults.count = suggestions.size();
                    return filterResults;
                } else {
                    return new FilterResults();
                }
            }

            @Override
            protected void publishResults(CharSequence constraint, FilterResults results) {
                List<String> filterList = (ArrayList<String>) results.values;
                if (results != null && results.count > 0) {
                    clear();
                    for (String scene: filterList) {
                        add(scene);
                        notifyDataSetChanged();
                    }
                }
            }
        };

        SceneAutoCompleteArrayAdapter(Context context, int resource, List<String> objects) {
            super(context, resource, objects);
            suggestions = new ArrayList<>();
        }

        @Override @NonNull
        public Filter getFilter() {
            return nameFilter;
        }
    }

    private final String[] scenes = {
            "scene.yaml",
            "https://mapzen.com/carto/bubble-wrap-style-more-labels/bubble-wrap-style-more-labels.zip",
            "https://mapzen.com/carto/refill-style-more-labels/refill-style-more-labels.zip",
            "https://mapzen.com/carto/walkabout-style-more-labels/walkabout-style-more-labels.zip",
            "https://mapzen.com/carto/tron-style-more-labels/tron-style-more-labels.zip",
            "https://mapzen.com/carto/cinnabar-style-more-labels/cinnabar-style-more-labels.zip",
            "https://mapzen.com/carto/zinc-style-more-labels/zinc-style-more-labels.zip"
    };

    AutoCompleteTextView autoCompleteView;
    InputMethodManager inputMethodService;
    SceneAutoCompleteArrayAdapter urlLoaderAdapter;
    LoadSceneCallback loadSceneCb;

    DemoSceneManager(AutoCompleteTextView view, InputMethodManager in, LoadSceneCallback cb) {
        autoCompleteView = view;
        inputMethodService = in;
        loadSceneCb = cb;
        List<String> lst = new ArrayList<>(Arrays.asList(scenes));
        urlLoaderAdapter = new SceneAutoCompleteArrayAdapter(view.getContext(), android.R.layout.simple_list_item_1, lst);
        autoCompleteView.setAdapter(urlLoaderAdapter);
        autoCompleteView.setThreshold(1);
        autoCompleteView.setOnItemClickListener(this);
        autoCompleteView.setOnEditorActionListener(this);
        autoCompleteView.setText(scenes[0]);
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        completeSelection();
    }

    @Override
    public boolean onEditorAction(TextView textView, int action, KeyEvent keyEvent) {
        if (action == EditorInfo.IME_ACTION_GO) {
            completeSelection();
            return true;
        }
        return false;
    }

    private void completeSelection() {
        autoCompleteView.clearFocus();
        inputMethodService.hideSoftInputFromWindow(autoCompleteView.getWindowToken(), 0);
        loadSceneCb.loadSceneCallback(autoCompleteView.getText().toString());
    }

    String getCurrentScene() {
        return autoCompleteView.getText().toString();
    }

}
