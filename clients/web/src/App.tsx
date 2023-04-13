import React, { ReactNode, useCallback, useEffect, useRef, useState } from "react";
import "./App.css";
import { Model } from "./Model";
import {
  calibrateIMU,
  onConnectRequest,
  setCoeffs,
  Config,
  SensorData,
  Coeffs,
  setRealtimeRun,
  setIMUOffsets,
  setName,
  setSleepMS,
  reset,
  IMUOffsets,
} from "./ble";
// import Switch from "react-switch";
import { Quaternion } from "three";

// @ts-ignore
import logo from "./sugarboat.png";
// @ts-ignore
import bluetoothIcon from "./bluetooth.svg";
import Switch from "./Switch";

function downloadConfig(config: Config) {
  const date = new Date().toISOString();
  const filename = `sugarboat-config-${config.name}-${date}.json`;
  const element = document.createElement("a");
  const data = {
    filename,
    date,
    config: config,
  };
  const json = JSON.stringify(data, null, 2);
  const file = new Blob([json], { type: "text/json" });
  element.href = URL.createObjectURL(file);
  element.download = filename;
  element.click();
}

type HeaderProps = {
  connected: boolean;
  onConnectClick: () => void;
  name: string;
};
function Header({ connected, onConnectClick, name }: HeaderProps) {
  const [checked, setChecked] = useState(false);
  const onRealtimeRunChange = useCallback((checked: boolean) => {
    setChecked(checked);
    setRealtimeRun(checked);
  }, []);
  return (
    <div className="Header-container">
      <img src={logo} alt="sugarboat Logo" className="Logo" />
      <button
        onClick={onConnectClick}
        disabled={connected}
        className="Header-connectButton"
      >
        <img src={bluetoothIcon} alt="Bluetooth icon" width={28} />
        {connected ? "Connected!" : "Connect"}
      </button>
      <label>
        <Switch
          disabled={!connected}
          checked={checked}
          onChange={onRealtimeRunChange}
          label="Real time"
        />
      </label>
      <div>{name}</div>
    </div>
  );
}

type ValueBoxProps = {
  name: string;
  children: ReactNode;
};
function ValueBox({ name, children }: ValueBoxProps) {
  return (
    <div className="ValueBox-container">
      <div className="ValueBox-title">{name}</div>
      <div className="ValueBox-content">{children}</div>
    </div>
  );
}

type DataSectionProps = {
  sensorData: SensorData;
};
function DataSection({ sensorData }: DataSectionProps) {
  return (
    <div className="ValueBoxes-container">
      <ValueBox name="Tilt Angle">
        <p className="DataBox-content">{sensorData.angle.toFixed(1) + " °"}</p>
      </ValueBox>
      <ValueBox name="Temperature">
        <p className="DataBox-content">
          {sensorData.tempCelcius.toFixed(1) + " °C"}
        </p>
      </ValueBox>
      <ValueBox name="Humidity">
        <p className="DataBox-content">
          {(100 * sensorData.relHumidity).toFixed(0) + " %"}
        </p>
      </ValueBox>
      <ValueBox name="Battery">
        <p className="DataBox-content">
          {sensorData.battVoltage.toFixed(1) + " V"}
        </p>
      </ValueBox>
    </div>
  );
}

type CalibrationSectionProps = {
  connected: boolean;
  config: Config;
};
function CalibrationSection({
  connected,
  config: { has_imu_offsets, has_coeffs, coeffs, imu_offsets },
}: CalibrationSectionProps) {
  const [stateCoeffs, setStateCoeffs] = useState<Coeffs>(coeffs);
  const [stateIMUOffsets, setStateIMUOffsets] = useState<IMUOffsets>(imu_offsets);

  useEffect(() => {
    setStateCoeffs(coeffs);
  }, [coeffs]);

  useEffect(() => {
    setStateIMUOffsets(imu_offsets);
  }, [imu_offsets]);

  // React.ChangeEventHandler<HTMLInputElement>
  const onCoeffChange = useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      const degree = event.target.getAttribute("data-degree");
      // const value = parseFloat(event.target.value);
      const value = event.target.value;
      setStateCoeffs({ ...stateCoeffs, [degree!]: value });
    },
    [stateCoeffs]
  );

  const onIMUOffsetsChange = useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      const desc = event.target.getAttribute("data-desc");
      const value =  event.target.value;
      setStateIMUOffsets({ ...stateIMUOffsets, [desc!]: value });
    },
    [stateIMUOffsets]
  );

  const renderCoeffs = useCallback(() => {
    if (!connected) {
      return <p style={{ fontSize: "2rem" }}>🤷‍♂️</p>;
    }
    if (has_coeffs) {
      return (
        <table>
          <thead>
            <tr>
              <th>Poly Degree</th>
              <th>Coefficient</th>
            </tr>
          </thead>
          <tbody>
            <tr>
              <td>2</td>
              <td>
                <input
                  type="text"
                  value={stateCoeffs.a2}
                  onChange={onCoeffChange}
                  data-degree="a2"
                />
              </td>
            </tr>
            <tr>
              <td>1</td>
              <td>
                <input
                  type="text"
                  value={stateCoeffs.a1}
                  onChange={onCoeffChange}
                  data-degree="a1"
                />
              </td>
            </tr>
            <tr>
              <td>0</td>
              <td>
                <input
                  type="text"
                  value={stateCoeffs.a0}
                  onChange={onCoeffChange}
                  data-degree="a0"
                />
              </td>
            </tr>
          </tbody>
        </table>
      );
    } else {
      return null;
    }
  }, [connected, has_coeffs, stateCoeffs, onCoeffChange]);

  const renderIMUOffsets = useCallback(() => {
    if (!connected) {
      return <p style={{ fontSize: "2rem" }}>🤷‍♂️</p>;
    } else if (!has_imu_offsets) {
      return <p style={{ fontSize: "2rem" }}>👎</p>;
    }
    return (
        <table>
          <thead>
            <tr>
              <th>Desc</th>
              <th>Coeff</th>
            </tr>
          </thead>
          <tbody>
            {
              ["accel_x", "accel_y", "accel_z", "gyro_x", "gyro_y", "gyro_z"].map((desc) => (
                <tr key={desc}>
                  <td>{desc}</td>
                  <td>
                    <input
                      type="text"
                      value={stateIMUOffsets[desc as keyof IMUOffsets]}
                      onChange={onIMUOffsetsChange}
                      data-desc={desc}
                    />
                  </td>
                </tr>
              ))
            }
          </tbody>
        </table>
    );
  }, [connected, has_imu_offsets, stateIMUOffsets, onIMUOffsetsChange]);

  return (
    <div>
      <h1>Calibration</h1>
      <div className="ValueBoxes-container">
        <ValueBox name="Accel / Gyro">
          <p className="DataBox-content"></p>
          {renderIMUOffsets()}
          <button disabled={!connected} onClick={() => setIMUOffsets(stateIMUOffsets).then(() => alert('Done, but will take effect only after reset'))}>
            Upload IMU Offsets
          </button>
          <br/>
          <button className="red" disabled={!connected} onClick={() => window.confirm("Calibrate? It will override the IMU offsets.") ? calibrateIMU() : null}>
            Calibrate
          </button>
        </ValueBox>
        <ValueBox name="Brix Scale">
          <p className="DataBox-content"></p>
          {renderCoeffs()}
          <button disabled={!connected} onClick={() => setCoeffs(stateCoeffs)}>
            Upload Coefficients
          </button>
        </ValueBox>
      </div>
    </div>
  );
}

type EstimatesSectionProps = {
  connected: boolean;
  sensorData: SensorData;
};

function EstimatesSection({ connected, sensorData }: EstimatesSectionProps) {
  return (
    <div>
      <h1>Estimates</h1>
      <div className="ValueBoxes-container">
        <ValueBox name="Brix">
          <p className="DataBox-content">{sensorData.brix} °Bx</p>
        </ValueBox>
        <ValueBox name="Specific Gravity">
          <p className="DataBox-content">{sensorData.sg}</p>
        </ValueBox>
      </div>
    </div>
  );
}
type ConfigSectionProps = {
  connected: boolean;
  name: string;
  setName: (name: string) => void;
  sleepMS: number;
  setSleepMS: (sleepMS: number) => void;
};

function ConfigSection({
  name,
  setName,
  connected,
  sleepMS,
  setSleepMS,
}: ConfigSectionProps) {
  const [nameState, setNameState] = useState(name);
  const [sleepMSState, setSleepMSState] = useState(sleepMS);

  useEffect(() => {
    setNameState(name);
  }, [name]);

  useEffect(() => {
    setSleepMSState(sleepMS);
  }, [sleepMS]);

  const setNameCB = useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      const value = event.target.value;
      setNameState(value);
    },
    []
  );

  const setSleepMSCB = useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      const value = event.target.value;
      setSleepMSState(parseInt(value));
    },
    []
  );
  return (
    <div>
      <h1>Config</h1>
      <div className="ValueBoxes-container">
        <ValueBox name="Name">
          <input
            type="text"
            value={nameState}
            onChange={setNameCB}
            disabled={!connected}
          />
          <button disabled={!connected} onClick={() => setName(nameState)}>
            Update
          </button>
        </ValueBox>
        <ValueBox name="Sleep (ms)">
          <input
            type="text"
            value={sleepMSState}
            onChange={setSleepMSCB}
            disabled={!connected}
          />
          <button
            disabled={!connected}
            onClick={() => setSleepMS(sleepMSState)}
          >
            Update
          </button>
        </ValueBox>
        <ValueBox name="Reset">
          <button disabled={!connected} onClick={reset}>
            Reset
          </button>
        </ValueBox>
      </div>
    </div>
  );
}

function BackupSection({ connected, config, setConfig }: { connected: boolean, config: Config, setConfig: (config: Config) => void }) {
  const fileInputRef = useRef<HTMLInputElement>(null);

  const readFile = useCallback((file: File | undefined) => {
    if (!file) return;
    const reader = new FileReader();
    reader.onload = (e) => {
      const json = JSON.parse(e.target?.result as string);
      console.log('Got config: ', json.config);
      setConfig(json.config);
    };
    reader.readAsText(file);
  }, [setConfig]);
  return (
    <div>
      <h1>Backup/Restore</h1>
      <div className="ValueBoxes-container">
        <ValueBox name="Backup">
          <button disabled={!connected} onClick={() => downloadConfig(config)}>Backup Config</button>
        </ValueBox>
        <ValueBox name="Restore">
          <input type="file" ref={fileInputRef} disabled={!connected} accept=".json" onChange={evt => {console.log(evt)}} />
          <button disabled={!connected} onClick={() => {readFile(fileInputRef.current?.files?.[0])}}>Load</button>
        </ValueBox>
      </div>
    </div>
  )
}

function App() {
  const orientation = useState<Quaternion>(new Quaternion());

  const [sensorData, setSensorData] = useState<SensorData>({
    angle: 0,
    brix: 0,
    sg: 0,
    tempCelcius: 0,
    relHumidity: 0,
    battVoltage: 0,
  });

  const [config, setConfig] = useState<Config>({
    version: 0,
    has_imu_offsets: false,
    has_coeffs: false,
    coeffs: {
      a2: 0,
      a1: 0,
      a0: 0,
    },
    imu_offsets: {
      accel_x: 0,
      accel_y: 0,
      accel_z: 0,
      gyro_x: 0,
      gyro_y: 0,
      gyro_z: 0,
    },
    name: "",
    sleep_ms: 0,
  });

  // const [name, setName] = useState("");

  const [connected, setConnected] = useState(false);
  const onConnect = useCallback((name: string) => {
    console.log("Connected!");
    setConnected(true);
    // setName(name);
  }, []);

  const onDisconnect = useCallback(() => {
    console.log("Disconnected!");
    setConnected(false);
    setName("");
  }, []);

  const onSensorData = useCallback(setSensorData, [setSensorData]);

  const onConfig = useCallback(
    (config: Config) => {
      setConfig(config);
    },
    [setConfig]
  );

  // @ts-ignore
  const onConnectClickCB = useCallback(
    onConnectRequest(onConnect, onDisconnect, onSensorData, onConfig),
    []
  );

  return (
    <div className="App">
      <Header
        onConnectClick={onConnectClickCB}
        connected={connected}
        name={""}
      />
      <div className="Container">
        <div className="Container-left">
          <DataSection sensorData={sensorData} />
          <EstimatesSection connected={connected} sensorData={sensorData} />
          <ConfigSection
            name={config.name}
            setName={setName}
            sleepMS={config.sleep_ms}
            setSleepMS={setSleepMS}
            connected={connected}
          />
          <CalibrationSection connected={connected} config={config} />
          <BackupSection connected={connected} config={config} setConfig={setConfig}/>
        </div>
        <div className="Container-right">
          <Model orientation={orientation[0]} />
        </div>
      </div>
    </div>
  );
}

export default App;
