//
//  ViewController.swift
//  TangramDemoSwift
//
//  Created by Matt Blair on 1/11/20.
//  Copyright © 2020 Matt Blair. All rights reserved.
//

import UIKit
import TangramMap

class ViewController: UIViewController, TGMapViewDelegate, TGRecognizerDelegate {

    override func viewDidLoad() {
        super.viewDidLoad()

        let mapView = self.view as! TGMapView

        mapView.mapViewDelegate = self
        mapView.gestureDelegate = self
    }

    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)

        let apiKey = Bundle.main.infoDictionary?["NextzenApiKey"] as! String

        let sceneUpdates = [TGSceneUpdate(path: "global.sdk_api_key", value: apiKey)]

        let mapView = self.view as! TGMapView

        let sceneUrl = URL(string: "https://www.nextzen.org/carto/bubble-wrap-style/9/bubble-wrap-style.zip")!

        mapView.loadSceneAsync(from: sceneUrl, with: sceneUpdates)
    }

    func mapView(_ mapView: TGMapView, didLoadScene sceneID: Int32, withError sceneError: Error?) {
        print("MapView did complete loading")

        let newYork = CLLocationCoordinate2DMake(40.70532700869127, -74.00976419448854)

        mapView.cameraPosition = TGCameraPosition(center: newYork, zoom: 15, bearing: 0, pitch: 0)
    }
}
