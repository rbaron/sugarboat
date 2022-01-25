import React, { useEffect, useRef, useState } from "react";
import * as THREE from "three";
import { ThreeMFLoader } from "three/examples/jsm/loaders/3MFLoader";
// import { VRMLLoader } from "three/examples/jsm/loaders/VRMLLoader";
import { OrbitControls } from "three/examples/jsm/controls/OrbitControls";
import { Quaternion } from "three";

import "./App.css";

type ModelProps = {
  orientation: Quaternion;
};

export function Model({ orientation }: ModelProps) {
  const container = useRef<HTMLDivElement>(null);
  useEffect(() => {
    const scene = new THREE.Scene();
    scene.background = new THREE.Color(0xeeeeee);
    const camera = new THREE.PerspectiveCamera(
      75,
      container.current!.clientWidth / container.current!.clientHeight,
      0.1,
      1000
    );

    scene.add(new THREE.AmbientLight(0xffffff, 0.8));

    const light = new THREE.PointLight(0xffffff, 1, 0);
    light.position.set(50, 0, 50);
    scene.add(light);
    const light2 = new THREE.PointLight(0xffffff, 1, 0);
    light2.position.set(50, 0, -50);
    scene.add(light2);

    const renderer = new THREE.WebGLRenderer();

    const controls = new OrbitControls(camera, renderer.domElement);

    container.current!.appendChild(renderer.domElement);
    renderer.setSize(
      container.current!.clientWidth,
      container.current!.clientHeight
    );

    const loader = new ThreeMFLoader();
    loader.load("./sugarboat.3mf", function (object: THREE.Group) {
      // const loader = new VRMLLoader(THREE.DefaultLoadingManager);
      // loader.load("./sugarboat.wrl", function (object) {
      setSugarboat(object);
      scene.add(object);
      renderer.render(scene, camera);
    });

    camera.position.z = 140;

    function animate() {
      requestAnimationFrame(animate);
      controls.update();
      renderer.render(scene, camera);
    }

    animate();
  }, []);

  const [sugarboat, setSugarboat] = useState<THREE.Group | null>(null);
  // const [sugarboat, setSugarboat] = useState<THREE.Scene | null>(null);

  useEffect(() => {
    if (!sugarboat) {
      return;
    }
    sugarboat.setRotationFromQuaternion(orientation);
  }, [sugarboat, orientation]);

  return (
    <div
      ref={container}
      className="Model-container"
      style={{ width: 512, height: 768 }}
    ></div>
  );
}
