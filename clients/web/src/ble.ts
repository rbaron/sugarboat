import { ServerResponse } from "http";

const kUARTServiceUUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
// const kUARTTXCharUUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";
const kSensorServiceUUID = "9b7d5c6f-a8ca-4080-9290-d4afb5ac64a3";
const kSensorCharacteristicUUID = "527d0f9b-db66-48c5-9089-071e1a795b6f";
const kConfigServiceUUID = "e94989ee-7b22-4b34-b71d-a33459aea9ae";
const kConfigCharacteristicUUID = "e65785ce-955b-42aa-95a2-b7d96806d3da";
const kCommandCharacteristicUUID = "97e3d772-0d23-45a6-b52f-36ea304fba5d";

let configChar: BluetoothRemoteGATTCharacteristic | null = null;
let commandChar : BluetoothRemoteGATTCharacteristic | null = null;

export type Callback = () => void;

export type SensorData = {
  angle: number;
  brix: number;
  sg: number;
  tempCelcius: number;
  relHumidity: number;
  battVoltage: number;
};

export type onSensor = (sensorData: SensorData) => void;

export type Coeffs = {
  a2: number;
  a1: number;
  a0: number;
};

export type IMUOffsets = {
  accel_x: number;
  accel_y: number;
  accel_z: number;
  gyro_x: number;
  gyro_y: number;
  gyro_z: number;
};

export type Config = {
  version: number;
  has_imu_offsets: boolean;
  imu_offsets: IMUOffsets;
  has_coeffs: boolean;
  coeffs: Coeffs;
  name: string;
  sleep_ms: number;
};

export type onConfig = (config: Config) => void;

function onConnectRequest(
  onConnect: (name: string) => void,
  onDisconnect: Callback,
  onSensor: onSensor,
  onConfig: onConfig
) {
  function onSensorEvent(event: any) {
    const characteristic = event.target;
    const data: DataView = characteristic.value;
    return onSensorData(data);
  }

  function onSensorData(data: DataView) {
    if (data.byteLength < 14) {
      console.log(
        "[ble sensor data] Received less bytes than expected: ",
        data
      );
      return;
    }
    onSensor({
      angle: data.getInt16(2, true) / 10.0,
      brix: data.getInt16(4, true) / 100.0,
      sg: data.getInt16(6, true) / 1000.0,
      tempCelcius: data.getInt16(8, true) / 100.0,
      relHumidity: data.getUint16(10, true) / (0xffff - 1),
      battVoltage: data.getUint16(12, true) / 1000.0,
    });
  }

  function onConfigEvent(event: any) {
    const characteristic = event.target;
    const dataView: DataView = characteristic.value;
    return onConfigData(dataView);
  }

  function onConfigData(dataView: DataView) {
    console.log("[ble config data] Raw: ", dataView);
    const str = String.fromCharCode.apply(
      null,
      // @ts-ignore
      new Uint8Array(dataView.buffer)
    );
    console.log("[ble config data] Str: ", str);
    const jsonCfg = JSON.parse(str);
    console.log("[ble] Config: ", jsonCfg);
    return onConfig(jsonCfg);
  }

  function handleSensorChar(characteristic: BluetoothRemoteGATTCharacteristic) {
    // sensorChar = characteristic;
    return Promise.all([
      characteristic.readValue().then(onSensorData),
      characteristic.startNotifications().then((char) => {
        characteristic.addEventListener(
          "characteristicvaluechanged",
          onSensorEvent
        );
      }),
    ]);
  }

  function handleSensorService(service: BluetoothRemoteGATTService) {
    return Promise.all([
      service
        .getCharacteristic(kSensorCharacteristicUUID)
        .then(handleSensorChar),
    ]);
  }

  function handleConfigService(service: BluetoothRemoteGATTService) {
    return Promise.all([
      service
        .getCharacteristic(kConfigCharacteristicUUID)
        .then((char) => {
          console.log('[ble] Got config char', char);
          configChar = char;
          return char;
        })
        .then(async (char) => {
          await char.readValue().then(onConfigData);
          return char;
        })
        .then((char) => char.startNotifications())
        .then((char) =>
          char.addEventListener("characteristicvaluechanged", onConfigEvent)
        ),
      service
        .getCharacteristic(kCommandCharacteristicUUID)
        .then((char) => {
          console.log('[ble] Got command char', char);
          commandChar = char;
          return char;
        }),
    ]);
  }

  return async () => {
    navigator.bluetooth
      .requestDevice({
        filters: [
          {
            // services: [kSensorServiceUUID, kConfigServiceUUID],
            services: [kSensorServiceUUID],
            // name: "sugarboat",
          },
        ],
        optionalServices: [
          kSensorServiceUUID,
          kConfigServiceUUID,
          kCommandCharacteristicUUID,
          kUARTServiceUUID,
        ],
      })
      .then((device) => {
        device.addEventListener("gattserverdisconnected", function (ev) {
          configChar = null;
          onDisconnect();
        });
        return device.gatt!.connect();
      })
      .then((server) => {
        console.log("[ble] Connected! Name:", server.device.name);
        onConnect(server.device.name!);
        return server;
      })
      .then((server) => Promise.all([
          server
            .getPrimaryService(kSensorServiceUUID)
            .then(handleSensorService),
          server
            .getPrimaryService(kConfigServiceUUID)
            .then(handleConfigService),
        ]));
  };
}

function writeBleCommand(buffer: ArrayBuffer) {
  if (!commandChar) {
    return Promise.reject("Not connected");
  }
  // return configChar.writeValueWithoutResponse(buffer);
  return commandChar.writeValue(buffer);
}

async function calibrateIMU() {
  const buffer = new ArrayBuffer(1);
  const view = new DataView(buffer);
  view.setInt8(0, 0x01);
  return await writeBleCommand(buffer);
}

async function setCoeffs(coeffs: Coeffs) {
  console.log("[ble] setCoeffs", coeffs);
  const buffer = new ArrayBuffer(1 + 3 * 4);
  const view = new DataView(buffer);
  view.setInt8(0, 0x02);
  view.setFloat32(1, coeffs.a2, true);
  view.setFloat32(1 + 4, coeffs.a1, true);
  view.setFloat32(1 + 8, coeffs.a0, true);
  return await writeBleCommand(buffer);
}

async function setIMUOffsets(imu_offsets: IMUOffsets) {
  const buffer = new ArrayBuffer(1 + 6 * 2);
  const view = new DataView(buffer);
  console.log("setIMUOffsets", imu_offsets);
  view.setInt8(0, 0x07);
  view.setInt16(1, parseInt(imu_offsets.accel_x as any), true);
  view.setInt16(3, imu_offsets.accel_y, true);
  view.setInt16(5, imu_offsets.accel_z, true);
  view.setInt16(7, imu_offsets.gyro_x, true);
  view.setInt16(9, imu_offsets.gyro_y, true);
  view.setInt16(11, imu_offsets.gyro_z, true);
  return await writeBleCommand(buffer);
}

async function setRealtimeRun(value: boolean) {
  const buffer = new ArrayBuffer(2);
  const view = new DataView(buffer);
  view.setInt8(0, 0x03);
  view.setInt8(1, value ? 0x01 : 0x00);
  return await writeBleCommand(buffer);
}

async function setName(name: string) {
  const buffer = new ArrayBuffer(name.length + 1);
  const view = new Uint8Array(buffer);
  view[0] = 0x04;
  for (let i = 0; i < name.length; i++) {
    // Will most likely break with non-ASCII chars.
    view[i + 1] = name.charCodeAt(i);
  }
  return await writeBleCommand(buffer);
}

async function setSleepMS(sleepMS: number) {
  const buffer = new ArrayBuffer(3);
  const view = new DataView(buffer);
  view.setInt8(0, 0x05);
  view.setUint16(1, sleepMS);
  return await writeBleCommand(buffer);
}

async function reset() {
  console.log('Will reset');
  const buffer = new ArrayBuffer(1);
  const view = new DataView(buffer);
  view.setInt8(0, 0x06);
  // view.setInt8(0, 0x08);
  const res = await writeBleCommand(buffer);
  console.log('Reset done: ', res);
}

export {
  onConnectRequest,
  calibrateIMU,
  setCoeffs,
  setIMUOffsets,
  setRealtimeRun,
  setName,
  setSleepMS,
  reset,
};
