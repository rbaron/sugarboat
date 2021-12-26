# MPU-6050
* [Datasheet](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf)
* [Register map](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf)
* It's a 6 degrees of freedom IMU
  * Accelerometer with 3 axes => Reads
  * Gyroscope with 3 axes
* But also has a built-in "MotionFusion" Digital Motion Processor (DMP)
  * "Offloads computation of motion processing algorithms" from the host processor (?)
  * Generates "sensor fusion data" (?)
  * So it becomes a 9-axis sensor
  * Somehow lets us extract the orientation from the accelerometer + gyro
  * Apparently [not publicly documented](https://github.com/jrowberg/i2cdevlib/blob/master/Arduino/MPU6050/MPU6050.h#L731)
  * Seems to have some sort of internal gravity sensor. Does this mean that it has some reference for pitch/roll, but not for yaw?
  * [This GitHub comment](https://github.com/jrowberg/i2cdevlib/issues/528#issuecomment-611634501) links to a PDF containing the specs of the firmware that powers the DMP
  * [Clear explanation of the DMP](https://github.com/jrowberg/i2cdevlib/issues/190#issuecomment-144270345) by the author of i2cdevlib

## Articles
* [Geek Mom Projects - DMP data from i2cdevlib](http://www.geekmomprojects.com/mpu-6050-dmp-data-from-i2cdevlib/)
* [The MPU6050 Explained](https://mjwhite8119.github.io/Robots/mpu6050)
  * Awesome resource for calibrating and getting the orientation using the DMP feature. It uses the i2cdevlib linked below

## Code
* [i2cdevlib](https://github.com/jrowberg/i2cdevlib)
  * [Code for reading the DMP](https://github.com/jrowberg/i2cdevlib/blob/master/Arduino/MPU6050/MPU6050_6Axis_MotionApps20.cpp#L118)
    * Apparently makes use of undocumented functionality!
    * There are multiple versions of the
  * [examples/MPU6050-DMP6](https://github.com/jrowberg/i2cdevlib/tree/master/Arduino/MPU6050/examples/MPU6050_DMP6)
    * Computes orientation using the DMP feature
  * [examples/IMU_Zero](https://github.com/jrowberg/i2cdevlib/tree/master/Arduino/MPU6050/examples/IMU_Zero)
    * Calibrates the gyroscope and accelerometer offsets
  * [Issues with PlatoformIO](https://community.platformio.org/t/i2cdev-incompatible-with-teensy-i2c-t3/11537/4)
* [MPU6050_light](https://github.com/rfetick/MPU6050_light)
  * A library for the MPU-6050 that estimates the angles in software (no DMP?)
* [MPU605-tockn](https://github.com/tockn/MPU6050_tockn)
  * Also calculates angles in software

 ## Calibration
 ```
 [-1743,-1742] --> [-7,11]  [719,720] --> [-7,5]    [1101,1101] --> [16381,16386]   [100,101] --> [0,3]     [60,61] --> [0,4]       [1,1] --> [0,1]

 [-1743,-1742]
 [719,720]
 [1101,1101]
 [100,101]
 [60,61]
 [1,1]
 ```

 ## How to calculate the tilt?
 Tilt: the angle between the sensor and the original's frame Z axis (or equivalently the gravity)

 The sensor gives us a quaternion.

### Euler angles
* [Converting quaternions to Euler angles](https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles)
*

### Row pitch yaw
