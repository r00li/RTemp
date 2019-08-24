//
//  ViewController.swift
//  RTemp
//
//  Created by Andrej Rolih on 13/01/16.
//  Copyright © 2016 Andrej Rolih. All rights reserved.
//

import UIKit
import Charts


class ViewController: UIViewController {
    
    @IBOutlet weak var mainStatusLabel: UILabel!
    @IBOutlet weak var secondaryStatusLabel: UILabel!
    @IBOutlet weak var temperatureLabel: UILabel!
    @IBOutlet weak var humidityLabel: UILabel!
    
    @IBOutlet weak var searchTableView: UITableView!
    
    @IBOutlet weak var settingsPanel: UIView!
    @IBOutlet weak var batteryValueLabel: UILabel!
    
    @IBOutlet weak var panelCancelButton: UIButton!
    
    @IBOutlet var settingsPanelLeadingConstraint: NSLayoutConstraint!
    @IBOutlet var chartPaneTrailingConstraint: NSLayoutConstraint!
    @IBOutlet var searchViewBottomConstraint: NSLayoutConstraint!
    
    @IBOutlet weak var chartPanel: UIView!
    @IBOutlet weak var chartTemperatureButton: UIButton!
    @IBOutlet weak var chartHumidityButton: UIButton!

    @IBOutlet weak var lineChartView: LineChartView!
    
    fileprivate var statusBarHidden: Bool = false
    fileprivate var humidityDataset: [ChartDataEntry] = []
    fileprivate var temperatureDataset: [ChartDataEntry] = []
    fileprivate var displayingTemperatureDataset: Bool = true
    

    override func viewDidLoad() {
        super.viewDidLoad()
        
        BLEPeripheralManager.sharedInstance.delegate = self
        
        refreshStatusDisplays()
        
        searchTableView.tableFooterView = UIView()
        
        lineChartView.transform = CGAffineTransform(rotationAngle: 90*CGFloat.pi/180)
        
        chartTemperatureButton.layer.borderColor = UIColor.white.cgColor
        chartTemperatureButton.transform = CGAffineTransform(rotationAngle: 90*CGFloat.pi/180)
        
        chartHumidityButton.layer.borderColor = UIColor.white.cgColor
        chartHumidityButton.transform = CGAffineTransform(rotationAngle: 90*CGFloat.pi/180)
        chartHumidityButton.alpha = 0.6
        
        setChart(dataEntries: self.temperatureDataset)
    }
    
    override func viewDidAppear(_ animated: Bool) {
        super.viewDidAppear(animated)
        
        BLEPeripheralManager.sharedInstance.reconnectLastPeripheral()
    }
    
    func refreshStatusDisplays() {
        if BLEPeripheralManager.sharedInstance.currentSensorState == .notConnected {
            secondaryStatusLabel.text = ""
            mainStatusLabel.text = "Not connected"
        } else if BLEPeripheralManager.sharedInstance.currentSensorState == .errorBLEGeneric {
            secondaryStatusLabel.text = ""
            mainStatusLabel.text = "BLE Error"
        } else if BLEPeripheralManager.sharedInstance.currentSensorState == .errorBLEOff {
            secondaryStatusLabel.text = ""
            mainStatusLabel.text = "BLE offline"
        } else if BLEPeripheralManager.sharedInstance.currentSensorState == .scanning {
            secondaryStatusLabel.text = ""
            mainStatusLabel.text = "Scanning..."
        } else if BLEPeripheralManager.sharedInstance.currentSensorState == .connecting {
            secondaryStatusLabel.text = ""
            mainStatusLabel.text = "Connecting..."
        } else if BLEPeripheralManager.sharedInstance.currentSensorState == .connected {
            secondaryStatusLabel.text = BLEPeripheralManager.sharedInstance.currentPeripheral?.name ?? ""
            mainStatusLabel.text = "Connected"
        }
        
        if !BLEPeripheralManager.sharedInstance.deviceConnected {
            self.temperatureLabel.text = "--"
            self.humidityLabel.text = "--"
            self.batteryValueLabel.text = "--"
        }
    }
    
    override var preferredStatusBarUpdateAnimation: UIStatusBarAnimation {
        return UIStatusBarAnimation.slide
    }
    override var prefersStatusBarHidden: Bool {
        return statusBarHidden
    }
    
    override var preferredStatusBarStyle: UIStatusBarStyle {
        return .lightContent
    }
    
    @IBAction func chartHumidityButtonPressed(_ sender: AnyObject) {
        chartHumidityButton.alpha = 1
        chartTemperatureButton.alpha = 0.6
        displayingTemperatureDataset = false
        
        setChart(dataEntries: self.humidityDataset)
    }
    
    @IBAction func chartTemperatureButtonPressed(_ sender: AnyObject) {
        chartHumidityButton.alpha = 0.6
        chartTemperatureButton.alpha = 1
        displayingTemperatureDataset = true
        
        setChart(dataEntries: self.temperatureDataset)
    }
    
    func setChart(dataEntries: [ChartDataEntry]) {
        
        let lineChartDataSet = LineChartDataSet(entries: dataEntries, label: "")
        lineChartDataSet.setColor(.white)
        lineChartDataSet.mode = .horizontalBezier
        lineChartDataSet.drawCirclesEnabled = false
        lineChartDataSet.lineWidth = 1
        lineChartDataSet.drawFilledEnabled = true
        lineChartDataSet.drawValuesEnabled = false
        //lineChartDataSet.fillColor = UIColor(colorLiteralRed: 32/255, green: 172/255, blue: 215/255, alpha: 1.0)
        lineChartDataSet.fillColor = .white
        lineChartDataSet.fillAlpha = 1
        lineChartDataSet.circleRadius = 5.0
        lineChartDataSet.highlightColor = UIColor.red
        lineChartDataSet.drawHorizontalHighlightIndicatorEnabled = true
        
        
        let leftAxis = lineChartView.leftAxis
        leftAxis.enabled = false
        
        lineChartView.rightAxis.labelTextColor = .white
        lineChartView.rightAxis.gridColor = UIColor(red: 32/255, green: 172/255, blue: 215/255, alpha: 1.0)
        lineChartView.rightAxis.granularity = displayingTemperatureDataset ? 0.5 : 1
        lineChartView.rightAxis.granularityEnabled = true
        lineChartView.rightAxis.drawAxisLineEnabled = false
        lineChartView.rightAxis.labelFont = UIFont(name: "HannotateSC-W5", size: 12) ?? UIFont.systemFont(ofSize: 11)
        
        var dataSets = [IChartDataSet]()
        dataSets.append(lineChartDataSet)
        
        let lineChartData = LineChartData(dataSets: dataSets)
        
        lineChartView.xAxis.labelRotationAngle = -45
        lineChartView.legend.enabled = false
        lineChartView.chartDescription?.text = ""
        lineChartView.noDataTextColor = .white
        lineChartView.noDataFont = UIFont(name: "HannotateSC-W5", size: 17) ?? UIFont.systemFont(ofSize: 17)
        lineChartView.noDataText = "Not enough data to display a chart."
        lineChartView.scaleYEnabled = false
        
        let xaxis = lineChartView.xAxis
        xaxis.labelPosition = .top
        xaxis.labelTextColor = .white
        xaxis.valueFormatter = self
        lineChartView.setViewPortOffsets(left: -1, top: 60, right: 100, bottom: 0)
        //xaxis.centerAxisLabelsEnabled = true
        xaxis.granularityEnabled = true
        xaxis.granularity = Double(60*30)
        xaxis.labelCount = dataEntries.count
        xaxis.gridColor = UIColor(red: 32/255, green: 172/255, blue: 215/255, alpha: 1.0)
        xaxis.labelFont = UIFont(name: "HannotateSC-W5", size: 12) ?? UIFont.systemFont(ofSize: 11)
        xaxis.centerAxisLabelsEnabled = false

        lineChartView.xAxis.labelCount = 8
        
        xaxis.drawAxisLineEnabled = false
        xaxis.forceLabelsEnabled = true
        
        let marker = BalloonMarker(color: UIColor(red: 32/255, green: 172/255, blue: 215/255, alpha: 1.0), font: UIFont(name: "HannotateSC-W5", size: 11) ?? UIFont.systemFont(ofSize: 11), textColor: .white, insets: UIEdgeInsets(top: 3, left: 8, bottom: 3, right: 8))
        marker.chartView = lineChartView
        marker.minimumSize = CGSize(width: 80, height: 50)
        marker.displayUnit = displayingTemperatureDataset ? "°C" : "%"
        lineChartView.marker = marker
        
        lineChartDataSet.highlightColor = UIColor(red: 32/255, green: 172/255, blue: 215/255, alpha: 1.0)
        lineChartDataSet.highlightLineWidth = 2
        
        if lineChartData.entryCount > 0 {
            lineChartView.data = lineChartData
        }
    }

    
    @IBAction func settingsButtonPressed(_ sender: AnyObject) {
        UIView.animate(withDuration: 0.4) {
            if self.settingsPanelLeadingConstraint.constant == 0 {
                self.settingsPanelLeadingConstraint.constant = -self.settingsPanel.frame.size.width
                self.panelCancelButton.isHidden = false
                self.statusBarHidden = true
            } else {
                self.settingsPanelLeadingConstraint.constant = 0
                self.panelCancelButton.isHidden = true
                self.statusBarHidden = false
            }
            self.view.layoutIfNeeded()
            self.setNeedsStatusBarAppearanceUpdate()
        }
    }
    
    @IBAction func chartButtonPressed(_ sender: AnyObject) {
        UIView.animate(withDuration: 0.4) {
            if self.chartPaneTrailingConstraint.constant == 0 {
                self.chartPaneTrailingConstraint.constant = -self.chartPanel.frame.size.width
                self.panelCancelButton.isHidden = false
                self.statusBarHidden = true
            } else {
                self.chartPaneTrailingConstraint.constant = 0
                self.panelCancelButton.isHidden = true
                self.statusBarHidden = false
            }
            self.view.layoutIfNeeded()
            self.setNeedsStatusBarAppearanceUpdate()
        }
    }
    
    @IBAction func panelCancelButtonPressed(_ sender: AnyObject) {
        if self.settingsPanelLeadingConstraint.constant != 0 {
            settingsButtonPressed(self)
        } else if self.chartPaneTrailingConstraint.constant != 0 {
            chartButtonPressed(self)
        }
    }
    
    @IBAction func centerButtonPressed(_ sender: Any) {
        let shouldStartScan = !self.searchViewBottomConstraint.isActive
        
        if shouldStartScan && !BLEPeripheralManager.sharedInstance.scanInProgress {
            if !BLEPeripheralManager.sharedInstance.beginScanningForSensors() {
                // TODO: Show BLE Scan error
            }
            self.searchTableView.reloadData()
        }
        refreshStatusDisplays()
        
        UIView.animate(withDuration: 0.4, animations: {
            self.searchViewBottomConstraint.isActive = !self.searchViewBottomConstraint.isActive
            self.view.layoutIfNeeded()
        })
    }
    
    func prepareChartDatasetFrom(data: [Float]) -> [ChartDataEntry] {
        var dataEntries: [ChartDataEntry] = []
        
        for i in (0..<data.count).reversed() {
            let date = Date().addingTimeInterval(-1*(60*15)*Double(i)).timeIntervalSince1970
            let dataEntry = ChartDataEntry(x: date, y: Double(data[i]))
            dataEntries.append(dataEntry)
        }
        
        return dataEntries
    }

}

extension ViewController: UITableViewDelegate, UITableViewDataSource {
    
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return BLEPeripheralManager.sharedInstance.BLEDevices.count
    }
    
    func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
        return 44
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        var cell = tableView.dequeueReusableCell(withIdentifier: "sensorCell")
        if cell == nil {
            cell = UITableViewCell(style: .default, reuseIdentifier: "sensorCell")
            cell?.backgroundColor = UIColor(red: 19/255, green: 96/255, blue: 140/255, alpha: 1.0)
            cell?.textLabel?.font = UIFont(name: "HannotateSC-W5", size: 17) ?? UIFont.systemFont(ofSize: 17)
            cell?.textLabel?.textColor = .white
            cell?.separatorInset = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: 15)
        }
        
        if let cell = cell {
            cell.textLabel?.text = BLEPeripheralManager.sharedInstance.BLEDevices[indexPath.row].name
            return cell
        } else {
            return UITableViewCell(style: .default, reuseIdentifier: "sensorCell")
        }
    }
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        UIView.animate(withDuration: 0.4, animations: {
            self.searchViewBottomConstraint.isActive = !self.searchViewBottomConstraint.isActive
            self.view.layoutIfNeeded()
        })
        
        tableView.deselectRow(at: indexPath, animated: true)
        
        BLEPeripheralManager.sharedInstance.connectToSensor(sensor: BLEPeripheralManager.sharedInstance.BLEDevices[indexPath.row])
    }
    
}

extension ViewController: BLEPeripheralManagerDelegate {
    
    func scanningComplete(error: String?) {
        refreshStatusDisplays()
    }
    
    func scanningDiscoveredDevice(device: BLEPeripheralManager.BLESensorDevice) {
        searchTableView.reloadData()
    }
    
    func sensorStatusChanged(newStatus: BLEPeripheralManager.SensorState) {
        refreshStatusDisplays()
    }
    
    func temperatureValueUpdated(newValue: Double) {
        temperatureLabel.text = String(newValue) + "°C"
    }
    
    func humidityValueUpdated(newValue: Int) {
        humidityLabel.text = String(newValue) + "%"
    }
    
    func batteryValueUpdated(newValue: Int) {
        batteryValueLabel.text = String(newValue) + "%"
    }
    
    func temperatureLogUpdated(newValues: [Float]) {
        self.temperatureDataset = prepareChartDatasetFrom(data: newValues)
        
        if displayingTemperatureDataset {
            chartTemperatureButtonPressed(self)
        }
    }
    
    func humidityLogUpdated(newValues: [Float]) {
        self.humidityDataset = prepareChartDatasetFrom(data: newValues)
        
        if !displayingTemperatureDataset {
            chartHumidityButtonPressed(self)
        }
    }
    
}

extension ViewController: IAxisValueFormatter {
    
    func stringForValue(_ value: Double, axis: AxisBase?) -> String {
        if let index = axis?.entries.firstIndex(of: value), index == 0 {
            return ""
        }
        
        if let lastEntry = axis?.entries.last, value == lastEntry {
            return ""
        }
        
        let dateFormatter = DateFormatter()
        dateFormatter.dateFormat = "HH:mm\n(dd.MM)"
        return dateFormatter.string(from: Date(timeIntervalSince1970: value))
    }
    
}
