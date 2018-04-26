package com.tangrams.reactnative;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReactContextBaseJavaModule;
/**
 * Created by saeed tabrizi saeed@nowcando.com on 2/26/17.
 */




public class ReactTangramESModule extends ReactContextBaseJavaModule {

    private ReactApplicationContext context;
    private TangramPackage aPackage;

    public ReactTangramESModule(ReactApplicationContext reactContext,TangramPackage thePackage) {
        super(reactContext);
        this.context = reactContext;
        this.aPackage = thePackage;
    }
    @Override
    public String getName() {
        return "TangramEsManager";
    }
}