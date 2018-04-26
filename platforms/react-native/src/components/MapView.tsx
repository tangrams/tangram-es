import * as React from "react";
import {
    View,
    UIManager,
    NativeModules,
    Dimensions,
    NativeAppEventEmitter,
    requireNativeComponent,
    NativeComponent,
    NativeMethodsMixin,
    findNodeHandle,
    Platform,
    ViewStyle,
    StyleProp,
    ViewProperties,
} from "react-native";
import * as PropTypes from "prop-types";

const LN2 = 0.6931471805599453;
const WORLD_PX_HEIGHT = 256;
const WORLD_PX_WIDTH = 256;
const ZOOM_MAX = 21;

/*if (Platform.OS === 'android') {

}*/

const REACT_CLASS = "RCTTangramMapView";
const REACT_ANDROID_CALLBACK = "RCTTangramMapViewAndroidCallback";

export interface BitmapData {
        format:"PNG";
        height: number;
        width: number;
        data: string;
        base64Data: string;
 }

export interface PointF {
    x: number;
    y: number;
}

export interface Polygon {
    points: Array<Array<LngLat>>;
    properties: Map<string, string>;
}
export interface Polyline {
    points: LngLat[];
    properties: Map<string, string>;
}

export type EaseType = "LINEAR" | "CUBIC" | "QUINT" | "SINE";

export class MarkerData {
    markerID: number;
    isVisible?: boolean;
    result?: any;
}

export class Marker {
    private userData: any;
    constructor(private map: MapView, private id: number, private isVisible?: boolean) {

    }

    public get ID(): number { return this.id; }

    public getMarkerId(): number {
        return this.id;
    }

    public setUserData<T>(data: T): Marker {
        this.userData = data;
        return this;
    }
    public getUserData<T>(): T {
        return this.userData;
    }
    public setVisible(visible: boolean): Promise<boolean> {
        return this.map.setMarkerVisible(this.id, visible);
    }
    public setDrawOrder(order: number): Promise<boolean> {
        return this.map.setMarkerDrawOrder(this.id, order);
    }
    public setPoint(point: LngLat): Promise<boolean> {
        return this.map.setMarkerPoint(this.id, point);
    }
    public setPointEased(point: LngLat, duration: number, easeType?: EaseType): Promise<boolean> {
        return this.map.setMarkerPointEased(this.id, point, duration, easeType);
    }
    public setPolygon(polygon: Polygon): Promise<boolean> {
        return this.map.setMarkerPolygon(this.id, polygon);
    }
    public setPolyline(polygon: Polyline): Promise<boolean> {
        return this.map.setMarkerPolyline(this.id, polygon);
    }
    public setStylingFromPath(style: string): Promise<boolean> {
        return this.map.setMarkerStylingFromPath(this.id, style);
    }
    public setStylingFromString(style: string): Promise<boolean> {
        return this.map.setMarkerStylingFromString(this.id, style);
    }
    public setBitmap(base64Bitmap: string): Promise<boolean> {
        return this.map.setMarkerBitmap(this.id, base64Bitmap);
    }
}


export interface LngLat {
    longitude: number;
    latitude: number;
}



export type CameraType = "PERSPECTIVE" | "ISOMETRIC" | "FLAT";

export interface MapViewProps extends ViewProperties {

    style?: StyleProp<ViewStyle>;
    scenePath?: string;
    sceneUpdates?: object;
    layers?: string[];
    geoPosition?: LngLat;
    pickRadius?: number;
    cacheName?: string;
    cacheSize?: number;
    cameraType?: CameraType;
    zoom?: number;
    minZoom?: number;
    maxZoom?: number;

    tilt?: number;
    minTilt?: number;
    maxTilt?: number;

    rotate?: number;
    minRotate?: number;
    maxRotate?: number;

    handleDoubleTap?: boolean;
    handleSingleTapUp?: boolean;
    handleSingleTapConfirmed?: boolean;
    handleFling?: boolean;
    handlePan?: boolean;
    handleScale?: boolean;
    handleShove?: boolean;
    handleRotate?: boolean;


    onError?(event?: Event): void;

    onSceneReloading?(event?: Event): void;
    onSceneReloaded?(event?: Event): void;
    onSceneReady?(event?: Event): void;
    onFeaturePick?(event?: Event): void;
    onLabelPick?(event?: Event): void;
    onMarkerPick?(event?: Event): void;
    onDoubleTap?(event?: Event): void;
    onLongPress?(event?: Event): void;
    onSingleTapUp?(event?: Event): void;
    onSingleTapConfirmed?(event?: Event): void;

    onViewComplete?(event?: Event): void;
    onPan?(event?: Event): void;
    onFling?(event?: Event): void;
    onScale?(event?: Event): void;
    onRotate?(event?: Event): void;
    onShove?(event?: Event): void;

}

export interface MapViewStates {
    isReady: boolean;
}


//const { TangramEsManager } = NativeModules;
const TangramEsManager: any = {};
if (Platform.OS === 'android') {
    const RCTUIManager = NativeModules.UIManager;
    const commands = RCTUIManager[REACT_CLASS].Commands;

    // Since we cannot pass functions to dispatchViewManagerCommand, we keep a
    // map of callbacks and send an int instead
    const callbackMap = new Map();
    let nextCallbackId = 0;

    Object.keys(commands).forEach(command => {
        TangramEsManager[command] = (handle: number, ...rawArgs: any[]) => {
            const args = rawArgs.map(arg => {
                if (typeof arg === 'function') {
                    callbackMap.set(nextCallbackId, arg);
                    return nextCallbackId++;
                }
                return arg;
            });
            RCTUIManager.dispatchViewManagerCommand(handle, commands[command], args);
        };
    });

    NativeAppEventEmitter.addListener(REACT_ANDROID_CALLBACK, ([callbackId, args]) => {
        const callback = callbackMap.get(callbackId);
        if (!callback) {
            throw new Error(`Native is calling a callbackId ${callbackId}, which is not registered`);
        }
        callbackMap.delete(callbackId);
        callback.apply(null, args);
    });
}


export class MapView extends React.Component<MapViewProps, MapViewStates> {

    private map: NativeMethodsMixin & MapView = null;
    static defaultProps = {
        scenePath: "default_scene.yaml",
        cameraType: "FLAT",
        cacheName: "tile_cache",
        cacheSize: 30,
        zoom: 14,
        minZoom: 1,
        maxZoom: 24,
        handleDoubleTap: true,
        handleSingleTapUp: true,
        handleSingleTapConfirmed: true,
        handlePan: true,
        handleFling: true,


    }
    static propTypes: any = {
        ...View.propTypes,
        scenePath: PropTypes.string,
        sceneUpdates: PropTypes.any,
        layers: PropTypes.array,
        geoPosition: PropTypes.shape({
            latitude: PropTypes.number.isRequired,
            longitude: PropTypes.number.isRequired
        }),
        pickRadius: PropTypes.number,
        cacheName: PropTypes.string,
        cacheSize: PropTypes.number,
        cameraType: PropTypes.oneOf(["PERSPECTIVE", "ISOMETRIC", "FLAT"]),
        zoom: PropTypes.number,
        minZoom: PropTypes.number,
        maxZoom: PropTypes.number,

        tilt: PropTypes.number,
        minTilt: PropTypes.number,
        maxTilt: PropTypes.number,

        rotate: PropTypes.number,
        minRotate: PropTypes.number,
        maxRotate: PropTypes.number,


        /**
        * Callback that is called once, when the error has occured.
        */
        onError: PropTypes.func,

        /**
       * Callback that is called once, when the map reloading scene.
       */
        onSceneReloading: PropTypes.func,
        /**
        * Callback that is called once, when the map reload scene.
        */
        onSceneReloaded: PropTypes.func,

        /**
        * Callback that is called once, when the scene ready.
        */
        onSceneReady: PropTypes.func,

        /**
                * Callback that is called once, when the feature pick.
                */
        onFeaturePick: PropTypes.func,

        /**
        * Callback that is called once, when the label pick.
        */
        onLabelPick: PropTypes.func,



        /**
        * Callback that is called once, when the marker pick.
        */
        onMarkerPick: PropTypes.func,


        /**
        * Callback that is called once, when the double tap.
        */
        onDoubleTap: PropTypes.func,

        /**
               * Callback that is called once, when the long press.
               */
        onLongPress: PropTypes.func,


        /**
        * Callback that is called once, when the single tap.
        */
        onSingleTapUp: PropTypes.func,

        /**
               * Callback that is called once, when the single tap confirmed.
               */
        onSingleTapConfirmed: PropTypes.func,


        /**
        * Callback that is called once, when the view completed.
        */
        onViewComplete: PropTypes.func,

        /**
                * Callback that is called once, when the pan.
                */
        onPan: PropTypes.func,


        /**
        * Callback that is called once, when the pan fling.
        */
        onFling: PropTypes.func,


        /**
               * Callback that is called once, when the scale changed.
               */
        onScale: PropTypes.func,


        /**
       * Callback that is called once, when the rotating.
       */
        onRotate: PropTypes.func,


        /**
       * Callback that is called once, when the shoving two finger.
       */
        onShove: PropTypes.func,


        handleDoubleTap: PropTypes.bool,
        handleSingleTapUp: PropTypes.bool,
        handleSingleTapConfirmed: PropTypes.bool,
        handleFling: PropTypes.bool,
        handlePan: PropTypes.bool,
        handleScale: PropTypes.bool,
        handleShove: PropTypes.bool,
        handleRotate: PropTypes.bool,


    }
    private markers = new Map<number, Marker>();
    constructor(props: MapViewProps, states: MapViewStates) {
        super(props, states);

        this.state = {
            isReady: Platform.OS === 'ios',
        };
        this._onError = this._onError.bind(this);
        this._onLongPress = this._onLongPress.bind(this);
        this._onSingleTapUp = this._onSingleTapUp.bind(this);
        this._onSingleTapConfirmed = this._onSingleTapConfirmed.bind(this);
        this._onSceneReady = this._onSceneReady.bind(this);
        this._onViewComplete = this._onViewComplete.bind(this);
        this._onDoubleTap = this._onDoubleTap.bind(this);
        this._onFling = this._onFling.bind(this);
        this._onFeaturePick = this._onFeaturePick.bind(this);
        this._onMarkerPick = this._onMarkerPick.bind(this);
        this._onLabelPick = this._onLabelPick.bind(this);
        this._onPan = this._onPan.bind(this);
        this._onScale = this._onScale.bind(this);
        this._onRotate = this._onRotate.bind(this);
        this._onShove = this._onShove.bind(this);

    }

    private getNativeName(): string {
        return REACT_CLASS;
    }


    componentDidMount() {
        if (this.refs && this.refs.map) {
            // this.requestRender();
        }
    }


    private _uiManagerCommand(name: string) {
        return NativeModules.UIManager[this.getNativeName()].Commands[name];
    }

    private _mapManagerCommand(name: string) {
        return NativeModules[`${this.getNativeName()}Manager`][name];
    }

    private _getHandle() {
        return findNodeHandle(this.map);
    }

    private _runCommand(name: string, args: any[]) {
        switch (Platform.OS) {
            case 'android':
                NativeModules.UIManager.dispatchViewManagerCommand(
                    this._getHandle(),
                    this._uiManagerCommand(name),
                    args
                );
                break;

            case 'ios':
                this._mapManagerCommand(name)(this._getHandle(), ...args);
                break;

            default:
                break;
        }
    }


    private _updateStyle() {
        /* const { customMapStyle } = this.props;
         this.map.setNativeProps({ customMapStyleString: JSON.stringify(customMapStyle) });*/
    }

    private _onSceneReady(event: Event) {
        const { geoPosition } = this.props;
        this.markers.clear();
        if (geoPosition) {
            this.map.setNativeProps({ geoPosition });
        }
        if (this.refs && this.refs.map) {
            this.requestRender();
        }
        this._updateStyle();
        this.setState({ isReady: true });
        if (this.props.onSceneReady) this.props.onSceneReady((event as any).nativeEvent.src);
    }


    private _onPan(event: Event) {
        if (this.props.onPan) this.props.onPan((event as any).nativeEvent.src);
    }
    private _onFling(event: Event) {
        if (this.props.onFling) this.props.onFling((event as any).nativeEvent.src);
    }

    private _onScale(event: Event) {
        if (this.props.onScale) this.props.onScale((event as any).nativeEvent.src);
    }

    private _onShove(event: Event) {
        if (this.props.onShove) this.props.onShove((event as any).nativeEvent.src);
    }

    private _onRotate(event: Event) {
        if (this.props.onRotate) this.props.onRotate((event as any).nativeEvent.src);
    }

    private _onFeaturePick(event: Event) {
        if (this.props.onFeaturePick) this.props.onFeaturePick((event as any).nativeEvent.src);
    }

    private _onLabelPick(event: Event) {
        if (this.props.onLabelPick) this.props.onLabelPick((event as any).nativeEvent.src);
    }

    private _onMarkerPick(event: Event) {
        if (this.props.onMarkerPick) this.props.onMarkerPick((event as any).nativeEvent.src);
    }

    private _onViewComplete(event: Event) {
        if (this.props.onViewComplete) this.props.onViewComplete((event as any).nativeEvent.src);
    }
    private _onError(event: Event) {
        if (this.props.onError) this.props.onError((event as any).nativeEvent.src);
    }
    private _onDoubleTap(event: Event) {
        if (this.props.onDoubleTap) this.props.onDoubleTap((event as any).nativeEvent.src);
    }
    private _onLongPress(event: Event) {
        if (this.props.onLongPress) this.props.onLongPress((event as any).nativeEvent.src);
    }
    private _onSingleTapUp(event: Event) {
        if (this.props.onSingleTapUp) this.props.onSingleTapUp((event as any).nativeEvent.src);
    }
    private _onSingleTapConfirmed(event: Event) {
        if (this.props.onSingleTapConfirmed) this.props.onSingleTapConfirmed((event as any).nativeEvent.src);
    }


    public async getGeoPosition(): Promise<LngLat> {
        let pr = new Promise<LngLat>((resolve, reject) => {
            TangramEsManager.getGeoPosition(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: LngLat) => {
                resolve(result);
            });
        });

        return pr;
    }

    public async getRotation(): Promise<number> {
        let pr = new Promise<number>((resolve, reject) => {
            TangramEsManager.getRotation(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: number) => {
                resolve(result);
            });
        });

        return pr;
    }

    public async getTilt(): Promise<number> {
        let pr = new Promise<number>((resolve, reject) => {
            TangramEsManager.getTilt(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: number) => {
                resolve(result);
            });
        });

        return pr;
    }

    public async getZoom(): Promise<number> {
        let pr = new Promise<number>((resolve, reject) => {
            TangramEsManager.getZoom(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: number) => {
                resolve(result);
            });
        });

        return pr;
    }

    public async getCameraType(): Promise<CameraType> {
        let pr = new Promise<CameraType>((resolve, reject) => {
            TangramEsManager.getCameraType(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: CameraType) => {
                resolve(result);
            });
        });

        return pr;
    }


    public async setGeoPosition(pos: LngLat): Promise<LngLat> {
        let pr = new Promise<LngLat>((resolve, reject) => {
            TangramEsManager.setGeoPosition(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: LngLat) => {
                resolve(result);
            }, pos);
        });

        return pr;
    }


    public async setTilt(tilt: number): Promise<number> {
        let pr = new Promise<number>((resolve, reject) => {
            TangramEsManager.setTilt(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: number) => {
                resolve(result);
            }, tilt);
        });

        return pr;
    }


    public async setZoom(zoom: number): Promise<number> {
        let pr = new Promise<number>((resolve, reject) => {
            TangramEsManager.setZoom(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: number) => {
                resolve(result);
            }, zoom);
        });

        return pr;
    }


    public async setRotation(amount: number): Promise<number> {
        let pr = new Promise<number>((resolve, reject) => {
            TangramEsManager.setRotation(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: number) => {
                resolve(result);
            }, amount);
        });

        return pr;
    }

    public async setCameraType(cameraType: CameraType): Promise<CameraType> {
        let pr = new Promise<CameraType>((resolve, reject) => {
            TangramEsManager.setCameraType(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: CameraType) => {
                resolve(result);
            }, cameraType);
        });

        return pr;
    }

    public async setPickRadius(radius: number): Promise<number> {
        let pr = new Promise<number>((resolve, reject) => {
            TangramEsManager.setPickRadius(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: number) => {
                resolve(result);
            }, radius);
        });

        return pr;
    }

    public async setGeoPositionEase(pos: LngLat, duration: number): Promise<LngLat>;
    public async setGeoPositionEase(pos: LngLat, duration: number, easeType?: EaseType): Promise<LngLat>;
    public async setGeoPositionEase(pos: LngLat, duration: number, easeType?: EaseType): Promise<LngLat> {
        let pr = new Promise<LngLat>((resolve, reject) => {
            TangramEsManager.setPositionEase(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: LngLat) => {
                resolve(result);
            }, pos, duration, easeType ? easeType : "CUBIC");
        });

        return pr;
    }

    public async setTiltEase(tilt: number, duration: number): Promise<number>;
    public async setTiltEase(tilt: number, duration: number, easeType?: EaseType): Promise<number>;
    public async setTiltEase(tilt: number, duration: number, easeType?: EaseType): Promise<number> {
        let pr = new Promise<number>((resolve, reject) => {
            TangramEsManager.setTiltEase(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: number) => {
                resolve(result);
            }, tilt, duration, easeType ? easeType : "CUBIC");
        });

        return pr;
    }

    public async setZoomEase(zoom: number, duration: number): Promise<number>;
    public async setZoomEase(zoom: number, duration: number, easeType?: EaseType): Promise<number>;
    public async setZoomEase(zoom: number, duration: number, easeType?: EaseType): Promise<number> {
        let pr = new Promise<number>((resolve, reject) => {
            TangramEsManager.setZoomEase(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: number) => {
                resolve(result);
            }, zoom, duration, easeType ? easeType : "CUBIC");
        });

        return pr;
    }

    public async setRotationEase(amount: number, duration: number): Promise<number>;
    public async setRotationEase(amount: number, duration: number, easeType?: EaseType): Promise<number>;
    public async setRotationEase(amount: number, duration: number, easeType?: EaseType): Promise<number> {
        let pr = new Promise<number>((resolve, reject) => {
            TangramEsManager.setRotationEase(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: number) => {
                resolve(result);
            }, amount, duration, easeType ? easeType : "CUBIC");
        });

        return pr;
    }


    public async pickFeature(posx: number, posy: number): Promise<void> {
        let pr = new Promise<void>((resolve, reject) => {
            TangramEsManager.pickFeature(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: void) => {
                resolve();
            }, posx, posy);
        });

        return pr;
    }

    public async pickLabel(posx: number, posy: number): Promise<void> {
        let pr = new Promise<void>((resolve, reject) => {
            TangramEsManager.pickLabel(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: void) => {
                resolve();
            }, posx, posy);
        });

        return pr;
    }

    public async pickMarker(posx: number, posy: number): Promise<void> {
        let pr = new Promise<void>((resolve, reject) => {
            TangramEsManager.pickMarker(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: void) => {
                resolve();
            }, posx, posy);
        });

        return pr;
    }


    public async addDataLayer(layername: string, jsonData?: string): Promise<number> {

        let pr = new Promise<number>((resolve, reject) => {
            TangramEsManager.addDataLayer(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: number) => {
                resolve(result);
            }, layername, jsonData);
        });

        return pr;
    }

    public async addPolygonMapDataLayer(layername: string, points: Set<Set<LngLat>>, propeties?: Map<string, string>): Promise<string> {

        let pr = new Promise<string>((resolve, reject) => {
            TangramEsManager.addPolygonMapDataLayer(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: string) => {
                resolve(result);
            }, layername, points, propeties);
        });

        return pr;
    }
    public async addPolylineMapDataLayer(layername: string, points: Set<LngLat>, propeties?: Map<string, string>): Promise<string> {

        let pr = new Promise<string>((resolve, reject) => {
            TangramEsManager.addPolylineMapDataLayer(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: string) => {
                resolve(result);
            }, layername, points, propeties);
        });

        return pr;
    }


    public async clearMapDataLayer(name: string): Promise<string> {
        let pr = new Promise<string>((resolve, reject) => {
            TangramEsManager.setPickRadius(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: string) => {
                resolve(result);
            }, name);
        });

        return pr;
    }

    public async removeMapDataLayer(name: string): Promise<string> {
        let pr = new Promise<string>((resolve, reject) => {
            TangramEsManager.setPickRadius(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: string) => {
                resolve(result);
            }, name);
        });

        return pr;
    }

    public async addMarker(): Promise<Marker> {
        const that = this;
        let pr = new Promise<Marker>((resolve, reject) => {
            TangramEsManager.addMarker(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (marker: MarkerData) => {
                const mk = new Marker(that, marker.markerID, marker.isVisible);
                that.markers.set(mk.ID, mk);
                resolve(mk);
            });
        });

        return pr;
    }

    public putMarker(isVisible: boolean, drawOrder?: number, point?: LngLat,
        polygon?: Polygon, polyline?: Polyline, style?: string,
        drawableID?: number, drawable?: BitmapData): Promise<Marker> {
        const that = this;
        let pr = new Promise<Marker>((resolve, reject) => {
            TangramEsManager.putMarker(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (marker: MarkerData) => {
                const mk = new Marker(that, marker.markerID, marker.isVisible);
                that.markers.set(mk.ID, mk);
                resolve(mk);
            }, isVisible, drawOrder,
                point, polygon, polyline,
                style, drawableID, drawable);
        });

        return pr;
    }



    public updateMarker(markerID: number, isVisible: boolean,
        drawOrder?: number, point?: LngLat,
        polygon?: Polygon, polyline?: Polyline, style?: string,
        drawableID?: number, drawable?: BitmapData): Promise<Marker> {
        const that = this;
        let pr = new Promise<Marker>((resolve, reject) => {
            TangramEsManager.updateMarker(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (marker: MarkerData) => {
                if (that.markers.has(marker.markerID)) {
                    resolve(that.markers.get(marker.markerID));
                }
                else {
                    const mk = new Marker(that, marker.markerID, marker.isVisible);
                    that.markers.set(mk.ID, mk);
                    resolve(mk);
                }

            }, markerID, isVisible, drawOrder,
                point, polygon, polyline,
                style, drawableID, drawable);
        });

        return pr;

    }

    /**
     * Remove a map marker by markerid
     * 
     * @param {number} markerID 
     * @returns {number} 
     * 
     * @memberOf MapView
     */
    public removeMarker(markerID: number): Promise<number> {
        const that = this;
        let pr = new Promise<number>((resolve, reject) => {
            TangramEsManager.removeMarker(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: number) => {
                that.markers.delete(result);
                resolve(result);
            }, markerID);
        });

        return pr;
    }


    /**
     * Remove all map markers .
     * 
     * @returns 
     * 
     * @memberOf MapView
     */
    public removeAllMarkers(): Promise<void> {
        const that = this;
        let pr = new Promise<void>((resolve, reject) => {
            TangramEsManager.removeAllMarkers(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: any) => {
                that.markers.clear();
                resolve(result);
            });
        });

        return pr;
    }

    public async setMarkerVisible(mid: number, visible: boolean): Promise<boolean> {
        let pr = new Promise<boolean>((resolve, reject) => {
            TangramEsManager.setMarkerVisible(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: MarkerData) => {
                resolve(result.result);
            }, mid, visible);
        });

        return pr;
    }

    public async setMarkerDrawOrder(mid: number, order: number): Promise<boolean> {
        let pr = new Promise<boolean>((resolve, reject) => {
            TangramEsManager.setMarkerDrawOrder(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: MarkerData) => {
                resolve(result.result);
            }, mid, order);
        });

        return pr;
    }

    public async setMarkerBitmap(mid: number, bitmapBase64: string): Promise<boolean> {
        let pr = new Promise<boolean>((resolve, reject) => {
            TangramEsManager.setMarkerBitmap(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: MarkerData) => {
                resolve(result.result);
            }, mid, bitmapBase64);
        });

        return pr;
    }

    public async setMarkerPoint(mid: number, point: LngLat): Promise<boolean> {
        let pr = new Promise<boolean>((resolve, reject) => {
            TangramEsManager.setMarkerPoint(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: MarkerData) => {
                resolve(result.result);
            }, mid, point);
        });

        return pr;
    }
    public async setMarkerPointEased(mid: number, point: LngLat, duration: number, easeType?: EaseType): Promise<boolean> {
        let pr = new Promise<boolean>((resolve, reject) => {
            TangramEsManager.setMarkerPoint(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: MarkerData) => {
                resolve(result.result);
            }, mid, point, duration, easeType ? easeType : "CUBIC");
        });

        return pr;

    }
    public async setMarkerPolygon(mid: number, polygon: Polygon): Promise<boolean> {
        let pr = new Promise<boolean>((resolve, reject) => {
            TangramEsManager.setMarkerPolygon(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: MarkerData) => {
                resolve(result.result);
            }, mid, polygon);
        });

        return pr;
    }
    public async setMarkerPolyline(mid: number, polyline: Polyline): Promise<boolean> {
        let pr = new Promise<boolean>((resolve, reject) => {
            TangramEsManager.setMarkerPolyline(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: MarkerData) => {
                resolve(result.result);
            }, mid, polyline);
        });

        return pr;
    }
    public async setMarkerStylingFromPath(mid: number, stylePath: string): Promise<boolean> {
        let pr = new Promise<boolean>((resolve, reject) => {
            TangramEsManager.setMarkerStylingFromPath(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: MarkerData) => {
                resolve(result.result);
            }, mid, stylePath);
        });

        return pr;
    }
    public async setMarkerStylingFromString(mid: number, style: string): Promise<boolean> {
        let pr = new Promise<boolean>((resolve, reject) => {
            TangramEsManager.setMarkerStylingFromString(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: MarkerData) => {
                resolve(result.result);
            }, mid, style);
        });

        return pr;
    }

    public async requestRender(): Promise<void> {
        let pr = new Promise<void>((resolve, reject) => {
            TangramEsManager.requestRender((err: any) => {
                reject(new Error(err));
            }, () => {
                resolve();
            });
        });
        return pr;
    }

    public async updateSceneAsync(scneUpdates?: object): Promise<void> {
        let pr = new Promise<void>((resolve, reject) => {
            TangramEsManager.updateSceneAsync((err: any) => {
                reject(new Error(err));
            }, () => {
                resolve();
            }, scneUpdates);
        });
        return pr;
    }

    public async loadSceneFile(scenePath: string, scneUpdates?: object): Promise<void> {
        let pr = new Promise<void>((resolve, reject) => {
            TangramEsManager.loadSceneFile((err: any) => {
                reject(new Error(err));
            }, () => {
                resolve();
            }, scenePath, scneUpdates);
        });
        return pr;
    }


    public async loadSceneFileAsync(scenePath: string, scneUpdates?: object): Promise<void> {
        let pr = new Promise<void>((resolve, reject) => {
            TangramEsManager.loadSceneFileAsync((err: any) => {
                reject(new Error(err));
            }, () => {
                resolve();
            }, scenePath, scneUpdates);
        });
        return pr;
    }


    public async useCachedGlState(cache: boolean): Promise<void> {
        let pr = new Promise<void>((resolve, reject) => {
            TangramEsManager.useCachedGlState((err: any) => {
                reject(new Error(err));
            }, () => {
                resolve();
            }, cache);
        });
        return pr;
    }

    /**
     * Convert a geo position to screen position
     * 
     * @param {PointF} pos 
     * @returns {LngLat} 
     * 
     * @memberOf MapView
     */
    public async screenToLngLat(pos: PointF): Promise<LngLat> {
        let pr = new Promise<LngLat>((resolve, reject) => {
            TangramEsManager.screenToLngLat(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: LngLat) => {
                resolve(result);
            }, pos);
        });

        return pr;
    }

    /**
     * Convert screen 
     * 
     * @param {LngLat} pos 
     * @returns {PointF} 
     * 
     * @memberOf MapView
     */
    public async lngLatToScreen(pos: LngLat): Promise<PointF> {
        let pr = new Promise<PointF>((resolve, reject) => {
            TangramEsManager.lngLatToScreen(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: PointF) => {
                resolve(result);
            }, pos);
        });

        return pr;
    }

    public async captureFrame(waiting?: boolean,compressRate?:number,width?:number,height?:number): Promise<BitmapData> {
        let result: any;
        waiting = waiting ? waiting : true;
        let pr = new Promise<BitmapData>((resolve, reject) => {
            TangramEsManager.captureFrame(findNodeHandle(this), (err: any) => {
                reject(new Error(err));
            }, (result: BitmapData) => {
                if(result){
                    result.data = `data:image/png;base64,${result.base64Data}`;
                }
                resolve(result);
            }, waiting,compressRate,width,height);
        });
        return pr;
    }

    public screenCenterPoint(): PointF {
        const { height, width } = Dimensions.get("window");
        return { y: height / 2, x: width / 2 };
    }

    public computeHeading(from:LngLat,to:LngLat): number {
        
        return null;
    }
    
    private  calcZoom(mapPx: number,  worldPx: number, fraction: number) : number {
        return Math.floor(Math.log(mapPx / worldPx / fraction) / LN2);
    }

    private  latRad(lat: number) : number {
        const sin = Math.sin(lat * Math.PI / 180);
        const radX2 = Math.log((1 + sin) / (1 - sin)) / 2;
        return Math.max(Math.min(radX2, Math.PI), -Math.PI) / 2;
    }

    public getBoundsZoomLevel(ne:LngLat,sw:LngLat, mapWidthPx:number,mapHeightPx:number): number{
        const latFraction = (this.latRad(ne.latitude) - this.latRad(sw.latitude)) / Math.PI;
        const lngDiff = ne.longitude - sw.longitude;
        const lngFraction = ((lngDiff < 0) ? (lngDiff + 360) : lngDiff) / 360;
    
        const latZoom = this.calcZoom(mapHeightPx, WORLD_PX_HEIGHT, latFraction);
        const lngZoom = this.calcZoom(mapWidthPx, WORLD_PX_WIDTH, lngFraction);
    
        const result = Math.min(latZoom, lngZoom);
        return Math.min(result, ZOOM_MAX);        
    }

    public async fitToBounds(positions: LngLat[], padding?: number,duration?: number): Promise<number> {
        const { height, width } = Dimensions.get("window");
        let minLat = Number.MIN_SAFE_INTEGER;
        let maxLat = Number.MAX_SAFE_INTEGER;
        let minLon = Number.MIN_SAFE_INTEGER;
        let maxLon = Number.MAX_SAFE_INTEGER;
        duration = duration || 1000;
        padding = padding || 0;
        for (const item of positions) {
            const lat = item.latitude;
            const lon = item.longitude;

            maxLat = Math.max(lat, maxLat);
            minLat = Math.min(lat, minLat);
            maxLon = Math.max(lon, maxLon);
            minLon = Math.min(lon, minLon);
        }
        const ne: LngLat = {latitude:maxLat,longitude:maxLon};
        const sw: LngLat = {latitude:minLat,longitude:minLon};
        const midpoint: LngLat = { latitude : (maxLat + minLat ) / 2, longitude: (maxLon + minLon ) / 2  }
        const zoomToFit = this.getBoundsZoomLevel(ne,sw,width - padding,height - padding);
        await this.setGeoPositionEase(midpoint,duration);
        return this.setZoomEase(zoomToFit, duration);
    }



    render() {

        const { style } = this.props;
        const TangramMapView = RCTTangramMapView;

        return (
            <TangramMapView
                ref={ref => { this.map = ref as any; }}
                style={[style && style]}
                onDoubleTap={this._onDoubleTap}
                onViewComplete={this._onViewComplete}
                onError={this._onError}
                onFeaturePick={this._onFeaturePick}
                onFling={this._onFling}
                onLabelPick={this._onLabelPick}
                onPan={this._onPan}
                onMarkerPick={this._onMarkerPick}
                onSceneReady={this._onSceneReady}
                onSingleTapUp={this._onSingleTapUp}
                onLongPress={this._onLongPress}
                onSingleTapConfirmed={this._onSingleTapConfirmed}
                onRotate={this._onRotate}
                onScale={this._onScale}
                onShove={this._onShove}
                {...this.props}
            ></TangramMapView>);



    }
}

const MapViewInterface: any = {
    name: "MapView",
    propTypes: MapView.propTypes
};

const RCTTangramMapView = requireNativeComponent<MapViewProps>('RCTTangramMapView', MapViewInterface, {
    nativeOnly: {
        onChange: true,
    },
});


