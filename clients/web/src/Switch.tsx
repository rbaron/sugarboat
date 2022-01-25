import React from "react";
import ReactSwitch from "react-switch";

import "./App.css";

type SwitchProps = {
  disabled?: boolean;
  checked: boolean;
  onChange: (checked: boolean) => void;
  label?: string;
};

export default function Switch({
  disabled,
  checked,
  onChange,
  label,
}: SwitchProps) {
  return (
    <div className="Switch-container">
      <label>
        {label}
        <div>
          <ReactSwitch
            disabled={disabled}
            checked={checked}
            onChange={onChange}
            checkedIcon={false}
            uncheckedIcon={false}
            borderRadius={1}
            height={24}
            onColor="#2797ff"
            offColor="#aaa"
          />
        </div>
      </label>
    </div>
  );
}
