/// <reference types="react" />
import * as React from "react";
import { ViewStyle, StyleProp, ViewProperties } from "react-native";
export interface Bitmap extends ArrayBuffer {
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
export declare type EaseType = "LINEAR" | "CUBIC" | "QUINT" | "SINE";
export interface Marker {
    markerID: number;
    isVisible?: boolean;
}
export interface LngLat {
    longitude: number;
    latitude: number;
}
export declare type CameraType = "PERSPECTIVE" | "ISOMETRIC" | "FLAT";
export interface MapViewProps extends ViewProperties {
    style?: StyleProp<ViewStyle>;
    scenePath?: string;
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
    onMapReloadingScene?(event?: Event): void;
    onMapReloadScene?(event?: Event): void;
    onMapReady?(event?: Event): void;
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
export declare class MapView extends React.Component<MapViewProps, MapViewStates> {
    private map;
    static defaultProps: {
        scenePath: string;
        cameraType: string;
        cacheName: string;
        cacheSize: number;
        zoom: number;
        minZoom: number;
        maxZoom: number;
        handleDoubleTap: boolean;
        handleSingleTapUp: boolean;
        handleSingleTapConfirmed: boolean;
        handlePan: boolean;
        handleFling: boolean;
    };
    static propTypes: any;
    constructor(props: MapViewProps, states: MapViewStates);
    private getNativeName();
    componentDidMount(): void;
    private _uiManagerCommand(name);
    private _mapManagerCommand(name);
    private _getHandle();
    private _runCommand(name, args);
    private _updateStyle();
    private _onMapReady(event);
    private _onPan(event);
    private _onFling(event);
    private _onScale(event);
    private _onShove(event);
    private _onRotate(event);
    private _onFeaturePick(event);
    private _onLabelPick(event);
    private _onMarkerPick(event);
    private _onViewComplete(event);
    private _onError(event);
    private _onDoubleTap(event);
    private _onLongPress(event);
    private _onSingleTapUp(event);
    private _onSingleTapConfirmed(event);
    getGeoPosition(): Promise<LngLat>;
    getRotation(): Promise<number>;
    getTilt(): Promise<number>;
    getZoom(): Promise<number>;
    getCameraType(): Promise<CameraType>;
    setGeoPosition(pos: LngLat): Promise<LngLat>;
    setTilt(tilt: number): Promise<number>;
    setZoom(zoom: number): Promise<number>;
    setRotation(amount: number): Promise<number>;
    setCameraType(cameraType: CameraType): Promise<CameraType>;
    setPickRadius(radius: number): Promise<number>;
    setGeoPositionEase(pos: LngLat, duration: number): Promise<LngLat>;
    setGeoPositionEase(pos: LngLat, duration: number, easeType?: EaseType): Promise<LngLat>;
    setTiltEase(tilt: number, duration: number): Promise<number>;
    setTiltEase(tilt: number, duration: number, easeType?: EaseType): Promise<number>;
    setZoomEase(zoom: number, duration: number): Promise<number>;
    setZoomEase(zoom: number, duration: number, easeType?: EaseType): Promise<number>;
    setRotationEase(amount: number, duration: number): Promise<number>;
    setRotationEase(amount: number, duration: number, easeType?: EaseType): Promise<number>;
    pickFeature(posx: number, posy: number): Promise<void>;
    pickLabel(posx: number, posy: number): Promise<void>;
    pickMarker(posx: number, posy: number): Promise<void>;
    addDataLayer(layername: string, jsonData?: string): Promise<number>;
    addPolygonMapDataLayer(layername: string, points: Set<Set<LngLat>>, propeties?: Map<string, string>): Promise<string>;
    addPolylineMapDataLayer(layername: string, points: Set<LngLat>, propeties?: Map<string, string>): Promise<string>;
    clearMapDataLayer(name: string): Promise<string>;
    removeMapDataLayer(name: string): Promise<string>;
    addMarker(isVisible: boolean, drawOrder?: number, point?: LngLat, polygon?: Polygon, polyline?: Polyline, style?: string, drawableID?: number, drawable?: Bitmap): Promise<Marker>;
    updateMarker(markerID: number, isVisible: boolean, drawOrder?: number, point?: LngLat, polygon?: Polygon, polyline?: Polyline, style?: string, drawableID?: number, drawable?: Bitmap): Promise<Marker>;
    /**
     * Remove a map marker by markerid
     *
     * @param {number} markerID
     * @returns {number}
     *
     * @memberOf MapView
     */
    removeMarker(markerID: number): Promise<number>;
    /**
     * Remove all map markers .
     *
     * @returns
     *
     * @memberOf MapView
     */
    removeAllMarkers(): Promise<void>;
    requestRender(): Promise<void>;
    applySceneUpdates(): Promise<void>;
    useCachedGlState(cache: boolean): Promise<void>;
    /**
     * Convert a geo position to screen position
     *
     * @param {PointF} pos
     * @returns {LngLat}
     *
     * @memberOf MapView
     */
    screenToLngLat(pos: PointF): Promise<LngLat>;
    /**
     * Convert screen
     *
     * @param {LngLat} pos
     * @returns {PointF}
     *
     * @memberOf MapView
     */
    lngLatToScreen(pos: LngLat): Promise<PointF>;
    captureFrame(waiting?: boolean): Promise<Bitmap>;
    render(): JSX.Element;
}
