package com.mapzen.tangram.android;

import android.content.Context;
import android.support.annotation.NonNull;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Filter;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

public class PresetSelectionTextView extends android.support.v7.widget.AppCompatAutoCompleteTextView {

    public interface OnSelectionListener {
        void onSelection(String selection);
    }

    // Custom ArrayAdaptor that displays any suggestions that contain the input as a substring.
    private class PartialMatchArrayAdapter extends ArrayAdapter {

        List<String> presets;
        List<String> suggestions = new ArrayList<>();

        Filter partialMatchFilter = new Filter() {
            @Override
            public CharSequence convertResultToString(Object resultValue) {
                return resultValue.toString().toLowerCase();
            }

            @Override
            protected FilterResults performFiltering(CharSequence constraint) {
                FilterResults results = new FilterResults();
                if (constraint != null) {
                    suggestions.clear();
                    String substring = constraint.toString().toLowerCase();
                    for (String preset: presets) {
                        if (preset.toLowerCase().contains(substring)) {
                            suggestions.add(preset);
                        }
                    }
                    results.values = suggestions;
                    results.count = suggestions.size();
                }
                return results;
            }

            @Override
            protected void publishResults(CharSequence constraint, FilterResults results) {
                if (results != null && results.count > 0) {
                    List<String> filterList = (ArrayList<String>)results.values;
                    clear();
                    addAll(filterList);
                    notifyDataSetChanged();
                }
            }
        };

        PartialMatchArrayAdapter(Context context, List<String> presets) {
            super(context, android.R.layout.simple_list_item_1);
            this.presets = presets;
        }

        @Override @NonNull
        public Filter getFilter() {
            return partialMatchFilter;
        }
    }

    public PresetSelectionTextView(Context context, AttributeSet attr) {
        super(context, attr);
        setOnEditorActionListener(new OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView textView, int action, KeyEvent keyEvent) {
                finishEntry();
                return false;
            }
        });
        setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> adapterView, View view, int i, long l) {
                finishEntry();
            }
        });
    }

    public void setPresetStrings(List<String> presets) {
        setAdapter(new PartialMatchArrayAdapter(getContext(), presets));
    }

    public void setOnSelectionListener(OnSelectionListener listener) {
        this.onSelectionListener = listener;
    }

    public String getCurrentString() {
        return getText().toString();
    }

    private OnSelectionListener onSelectionListener;

    private void finishEntry() {
        clearFocus();
        InputMethodManager imm = (InputMethodManager)getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
        imm.hideSoftInputFromWindow(getWindowToken(), 0);
        if (onSelectionListener != null) {
            onSelectionListener.onSelection(getCurrentString());
        }
    }

}
