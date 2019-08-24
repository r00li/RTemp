//
//  BLEPeripheralManager.swift
//  RTemp
//
//  Created by Andrej Rolih on 28/01/16.
//  Copyright © 2016 Andrej Rolih. All rights reserved.
//

import UIKit
import CoreBluetooth


protocol BLEPeripheralManagerDelegate: class {
    func scanningComplete(error: String?)
    func scanningDiscoveredDevice(device: BLEPeripheralManager.BLESensorDevice)
    func sensorStatusChanged(newStatus: BLEPeripheralManager.SensorState)
    func temperatureValueUpdated(newValue: Double)
    func humidityValueUpdated(newValue: Int)
    func batteryValueUpdated(newValue: Int)
    func temperatureLogUpdated(newValues: [Float])
    func humidityLogUpdated(newValues: [Float])
}


class BLEPeripheralManager: NSObject, CBCentralManagerDelegate, CBPeripheralDelegate {
    
    enum SensorState {
        case notConnected
        case connecting
        case connected
        case scanning
        case errorBLEGeneric
        case errorBLEOff
    }
    
    class BLESensorDevice {
        var name: String?
        var peripheral: CBPeripheral?
    }
    
    static let sharedInstance = BLEPeripheralManager()
    
    private(set) var BLEDevices: [BLESensorDevice] = []
    
    lazy var centralManager : CBCentralManager = {
        let manager = CBCentralManager(delegate: self, queue: nil)
        return manager
    } ()
    
    var currentSensorState: SensorState {
        get {
            if self.centralManager.state == .poweredOff {
                return .errorBLEOff
            } else if self.centralManager.state != .poweredOn {
                return .errorBLEGeneric
            }
            
            if scanInProgress {
                return .scanning
            } else if deviceConnecting {
                return .connecting
            } else if deviceConnected {
                return .connected
            }
            
            return .notConnected
        }
    }
    
    weak var delegate: BLEPeripheralManagerDelegate?
    
    var scanInProgress: Bool = false {
        didSet {
            delegate?.sensorStatusChanged(newStatus: currentSensorState)
        }
    }
    var deviceConnected: Bool = false {
        didSet {
            delegate?.sensorStatusChanged(newStatus: currentSensorState)
            
            if deviceConnected {
                if dataCheckTimer == nil {
                    dataCheckTimer = Timer.scheduledTimer(timeInterval: 60, target: self, selector: #selector(refreshData), userInfo: nil, repeats: true)
                }
                UserDefaults.standard.set(self.currentPeripheral?.identifier.uuidString ?? "", forKey: "lastConnectedPeripheral")
            } else {
                dataCheckTimer?.invalidate()
                dataCheckTimer = nil
            }
        }
    }
    var deviceConnecting: Bool = false {
        didSet {
            delegate?.sensorStatusChanged(newStatus: currentSensorState)
        }
    }
    
    var currentPeripheral : CBPeripheral?
    var temperatureService: CBService?
    var batteryService: CBService?
    var temperatureCharacteristic: CBCharacteristic?
    var humidityCharacteristic: CBCharacteristic?
    var batteryCharacteristic: CBCharacteristic?
    var temperatureLogCharacteristic: CBCharacteristic?
    var humidityLogCharacteristic: CBCharacteristic?
    
    var dataCheckTimer: Timer?
    var previousTemperatureLogIndex: Int?
    var previousHumidityLogIndex: Int?
    
    var reconnectScan: Bool = false
    var connectTimer: Timer?
    var waitingForConnection: Bool = false
    
    let RTempSensorServiceUUID = CBUUID(string: "1BC50000-0200-3180-E511-9DA1608C7B7B")
    let batteryServiceUUID = CBUUID(string: "180F")
    
    let temperatureCharacteristicUUID = CBUUID(string: "1BC50001-0200-3180-E511-9DA1608C7B7B")
    let humidityCharacteristicUUID = CBUUID(string: "1BC50002-0200-3180-E511-9DA1608C7B7B")
    let temperatureLogCharacteristicUUID = CBUUID(string: "1BC50003-0200-3180-E511-9DA1608C7B7B")
    let humidityLogCharacteristicUUID = CBUUID(string: "1BC50004-0200-3180-E511-9DA1608C7B7B")
    let batteryCharacteristicUUID = CBUUID(string: "2A19")
    
    
    func beginScanningForSensors() -> Bool {
        if self.centralManager.state == .poweredOn {
            // Scan for peripherals if BLE is turned on
            
            self.BLEDevices = []
            
            self.centralManager.scanForPeripherals(withServices: [RTempSensorServiceUUID], options: nil)
            self.scanInProgress = true
            
            print("Scanning started")
            
            DispatchQueue.main.asyncAfter(deadline: .now() + 10, execute: {
                self.centralManager.stopScan()
                self.scanInProgress = false
                
                self.delegate?.scanningComplete(error: nil)
            })
            
            return true
        }
        else {
            // Can have different conditions for all states if needed - print generic message for now
            print("Bluetooth switched off or not initialized")
            
            self.BLEDevices = []
            
            return false
        }
    }
    
    func reconnectLastPeripheral() {
        if let lastPeripheralUUIDString = UserDefaults.standard.string(forKey: "lastConnectedPeripheral"), let lastPeripheralUUID = UUID(uuidString: lastPeripheralUUIDString) {
            // Needs a bit of a delay after app starts
            DispatchQueue.main.asyncAfter(deadline: .now()+2, execute: {
                print("Reconnecting device:", lastPeripheralUUIDString)
                let list = self.centralManager.retrievePeripherals(withIdentifiers: [lastPeripheralUUID])
                
                if let peripheral = list.first {
                    let sensor = BLEPeripheralManager.BLESensorDevice()
                    sensor.peripheral = peripheral
                    sensor.name = peripheral.name
                    self.connectToSensor(sensor: sensor)
                }
            })
        }
    }
    
    @objc func refreshData() {
        if let temperatureCharacteristic = self.temperatureCharacteristic {
            self.currentPeripheral?.readValue(for: temperatureCharacteristic)
        }
        
        if let humidityCharacteristic = self.humidityCharacteristic {
            self.currentPeripheral?.readValue(for: humidityCharacteristic)
        }
        
        if let batteryCharacteristic = self.batteryCharacteristic {
            self.currentPeripheral?.readValue(for: batteryCharacteristic)
        }
        
        if let temperatureLogCharacteristic = self.temperatureLogCharacteristic {
            self.currentPeripheral?.readValue(for: temperatureLogCharacteristic)
        }
        
        if let humidityLogCharacteristic = self.humidityLogCharacteristic {
            self.currentPeripheral?.readValue(for: humidityLogCharacteristic)
        }
    }
    
    func connectToSensor(sensor: BLESensorDevice) {
        guard let peripheral = sensor.peripheral else {
            return
        }
        
        if self.scanInProgress {
            self.centralManager.stopScan()
            self.scanInProgress = false
        }
        
        self.deviceConnecting = true
        
        self.currentPeripheral = peripheral
        self.currentPeripheral?.delegate = self
        
        self.waitingForConnection = true
        self.centralManager.connect(peripheral, options: nil)
        
        if connectTimer != nil {
            connectTimer?.invalidate()
            connectTimer = nil
        }
        
        connectTimer = Timer.scheduledTimer(timeInterval: 10, target: self, selector: #selector(connectTimerFired), userInfo: nil, repeats: false)
    }
    
    @objc func connectTimerFired() {
        if self.waitingForConnection {
            if let peripheral = self.currentPeripheral {
                centralManager.cancelPeripheralConnection(peripheral)
            }
            clearConnection(stopScan: false)
        }
    }
    
    func clearConnection(stopScan: Bool = true) {
        if self.scanInProgress && stopScan {
            self.centralManager.stopScan()
            self.scanInProgress = false
        }
        
        deviceConnected = false
        deviceConnecting = false
        
        currentPeripheral = nil
        temperatureService = nil
        batteryService = nil
        temperatureCharacteristic = nil
        humidityCharacteristic = nil
        batteryCharacteristic = nil
        
        previousHumidityLogIndex = nil
        previousTemperatureLogIndex = nil
        
        waitingForConnection = false
    }
    
    func checkDiscoveryComplete() -> Bool {
        return currentPeripheral != nil && temperatureService != nil && batteryService != nil && temperatureCharacteristic != nil && humidityCharacteristic != nil && batteryCharacteristic != nil && humidityLogCharacteristic != nil && temperatureLogCharacteristic != nil
    }
    
    // Check status of BLE hardware
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        delegate?.sensorStatusChanged(newStatus: currentSensorState)
    }
    
    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String : Any], rssi RSSI: NSNumber)
    {
        let nameOfDeviceFound = (advertisementData as NSDictionary).object(forKey: CBAdvertisementDataLocalNameKey) as? String
        
        let newSensor = BLESensorDevice()
        newSensor.name = nameOfDeviceFound ?? "Unnamed device"
        newSensor.peripheral = peripheral
        
        self.BLEDevices.append(newSensor)
        
        print(advertisementData)
        
        delegate?.scanningDiscoveredDevice(device: newSensor)
    }
    
    // Discover services of the peripheral
    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        //self.statusLabel.text = "Discovering services..."
        peripheral.discoverServices(nil)
        self.waitingForConnection = false
    }
    
    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral, error: Error?) {
        self.deviceConnecting = false
    }
    
    // Check if the service discovered is a valid IR Temperature Service
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        
        for service in peripheral.services! {
            let thisService = service as CBService
            
            if service.uuid == RTempSensorServiceUUID {
                // Discover characteristics of IR Temperature Service
                peripheral.discoverCharacteristics(nil, for: thisService)
                //self.statusLabel.text = "Getting characteristic info"
                self.temperatureService = thisService
            }
            else if service.uuid == self.batteryServiceUUID {
                peripheral.discoverCharacteristics(nil, for: thisService)
                self.batteryService = thisService
            }
            print(thisService.uuid)
        }
    }
    
    // Enable notification and sensor for each characteristic of valid service
    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        
        if (error != nil)
        {
            print(error)
            return
        }
        
        //self.statusLabel.text = "Connected"
        self.deviceConnected = true
        //self.connectedDeviceValueLabel.text = self.currentPeripheral.name
        
        // check the uuid of each characteristic to find config and data characteristics
        for charateristic in service.characteristics ?? [] {
            if let currentPeripheral = currentPeripheral {
                
                // check for data characteristic
                if charateristic.uuid == self.temperatureCharacteristicUUID {
                    self.temperatureCharacteristic = charateristic
                    currentPeripheral.setNotifyValue(true, for: charateristic)
                }
                else if charateristic.uuid == self.humidityCharacteristicUUID {
                    self.humidityCharacteristic = charateristic
                    currentPeripheral.setNotifyValue(true, for: charateristic)
                }
                else if charateristic.uuid == self.batteryCharacteristicUUID {
                    self.batteryCharacteristic = charateristic
                    currentPeripheral.setNotifyValue(true, for: charateristic)
                } else if charateristic.uuid == self.temperatureLogCharacteristicUUID {
                    self.temperatureLogCharacteristic = charateristic
                } else if charateristic.uuid == self.humidityLogCharacteristicUUID {
                    self.humidityLogCharacteristic = charateristic
                }
                
                print("Characteristic: ", charateristic.uuid)
                
                if self.checkDiscoveryComplete() {
                    self.deviceConnected = true
                    self.deviceConnecting = false
                    
                    refreshData()
                }
            }
        }
        
    }
    
    // Get data values when they are updated
    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        
        if (error != nil)
        {
            print(error)
            return
        }
        
        if characteristic.uuid == self.temperatureCharacteristicUUID {
            let dataBytes = characteristic.value
            let dataLength = dataBytes!.count
            var dataArray = [Int8](repeating: 0, count: dataLength)
            (dataBytes! as NSData).getBytes(&dataArray, length: dataLength * MemoryLayout<Int16>.size)
            
            let sign = Int(dataArray[3]) == 0 ? 1.0 : -1.0
            let value = (sign * Double(dataArray[2])) + (Double(dataArray[1]) * 0.1)
            
            delegate?.temperatureValueUpdated(newValue: value)
            //NSLog("%@%d.%d°C", (dataArray[3] == 0 ? "": "-") , dataArray[2], dataArray[1])
        }
        else if characteristic.uuid == self.humidityCharacteristicUUID {
            let dataBytes = characteristic.value
            let dataLength = dataBytes!.count
            var dataArray = [Int8](repeating: 0, count: dataLength)
            (dataBytes! as NSData).getBytes(&dataArray, length: dataLength * MemoryLayout<Int16>.size)
            
            delegate?.humidityValueUpdated(newValue: Int(dataArray[0]))
            //NSLog("%d%%", dataArray[0])
        }
        else if characteristic.uuid == self.batteryCharacteristicUUID {
            let dataBytes = characteristic.value
            let dataLength = dataBytes!.count
            var dataArray = [Int8](repeating: 0, count: dataLength)
            (dataBytes! as NSData).getBytes(&dataArray, length: dataLength * MemoryLayout<Int16>.size)
            
            //self.batteryLevelLabel.text = String(format: "%d%%", dataArray[0])
            delegate?.batteryValueUpdated(newValue: Int(dataArray[0]))
        } else if characteristic.uuid == self.temperatureLogCharacteristicUUID {
            let dataBytes = characteristic.value
            let dataLength = dataBytes!.count
            var dataArray = [UInt8](repeating: 0, count: dataLength)
            (dataBytes! as NSData).getBytes(&dataArray, length: dataLength * MemoryLayout<Int16>.size)
            
            print("Temperature data array: ", dataArray)
            
            if dataArray.count > 0 {
                let currentIndex = Int(dataArray[0]) - 1 - 1
                dataArray.removeFirst()
                
                var newArray: [Float] = []
                
                var index = currentIndex
                for _ in 0..<dataArray.count {
                    let value = dataArray[index]
                    
                    var newNumber = Float(value & 0b00111111)
                    
                    if value & 0b10000000 != 0 {
                        newNumber *= -1
                    }
                    
                    if value & 0b01000000 != 0 {
                        newNumber += 0.5
                    }
                    
                    if value != 0xFF {
                        newArray.append(newNumber)
                    }
                    
                    if index-1 < 0 {
                        index = dataArray.count-1
                    } else {
                        index -= 1
                    }
                }
                
                print("Transformed temperature log: ", newArray)
                
                let indexChanged = currentIndex != previousTemperatureLogIndex
                if indexChanged {
                    delegate?.temperatureLogUpdated(newValues: newArray)
                }
                
                previousTemperatureLogIndex = currentIndex
            }
        } else if characteristic.uuid == self.humidityLogCharacteristicUUID {
            let dataBytes = characteristic.value
            let dataLength = dataBytes!.count
            var dataArray = [UInt8](repeating: 0, count: dataLength)
            (dataBytes! as NSData).getBytes(&dataArray, length: dataLength * MemoryLayout<Int16>.size)
            
            print("Humidity data array: ", dataArray)
            
            if dataArray.count > 0 {
                let currentIndex = Int(dataArray[0]) - 1 - 1
                dataArray.removeFirst()
                
                var newArray: [Float] = []
                
                var index = currentIndex
                for _ in 0..<dataArray.count {
                    let value = dataArray[index]
                    
                    if value != 0xFF {
                        newArray.append(Float(value))
                    }
                    
                    if index-1 < 0 {
                        index = dataArray.count-1
                    } else {
                        index -= 1
                    }
                }
                
                print("Transformed humidity log: ", newArray)
                
                let indexChanged = currentIndex != previousHumidityLogIndex
                if indexChanged {
                    delegate?.humidityLogUpdated(newValues: newArray)
                }
                
                previousHumidityLogIndex = currentIndex
            }
        }
    }
    
    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        if (self.currentPeripheral == peripheral)
        {
            self.clearConnection()
        }
    }

}
