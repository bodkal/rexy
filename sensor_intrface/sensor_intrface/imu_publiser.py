import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Imu

from digitalio import DigitalInOut

import board
#import busio
from adafruit_extended_bus import ExtendedI2C as I2C

from adafruit_bno08x.i2c import BNO08X_I2C
from adafruit_bno08x import (
    BNO_REPORT_ACCELEROMETER,
    BNO_REPORT_GYROSCOPE,
    BNO_REPORT_MAGNETOMETER,
    BNO_REPORT_ROTATION_VECTOR,
)


class IMUPublisher(Node):

    def __init__(self):
        super().__init__('imu_publisher')
      
        self.init_sensor()
        self.i=0
        reset_pin = DigitalInOut(board.D5)

        self.imu = Imu()
        self.publisher_ = self.create_publisher(Imu, 'imu', 10)
        self.timer = self.create_timer(0.02, self.timer_callback)
        print("start timers",flush=True)

    def init_sensor(self,reset=None):
        i2c =I2C(1)
        self.bno = BNO08X_I2C(i2c,address=0x4b)
        self.bno.enable_feature(BNO_REPORT_ACCELEROMETER)
        self.bno.enable_feature(BNO_REPORT_GYROSCOPE)
        self.bno.enable_feature(BNO_REPORT_MAGNETOMETER)
        self.bno.enable_feature(BNO_REPORT_ROTATION_VECTOR)
        print("success with connection to the board",flush=True)

    

    def timer_callback(self):
        # try:
            # print("Acceleration:")
            # accel_x, accel_y, accel_z = self.bno.acceleration  # pylint:disable=no-member
            # print("X: %0.6f  Y: %0.6f Z: %0.6f  m/s^2" % (accel_x, accel_y, accel_z))
            # print("")

            # print("Gyro:")
            # gyro_x, gyro_y, gyro_z = self.bno.gyro  # pylint:disable=no-member
            # print("X: %0.6f  Y: %0.6f Z: %0.6f rads/s" % (gyro_x, gyro_y, gyro_z))
            # print("")

            # print("Magnetometer:")
            # mag_x, mag_y, mag_z = self.bno.magnetic  # pylint:disable=no-member
            # print("X: %0.6f  Y: %0.6f Z: %0.6f uT" % (mag_x, mag_y, mag_z))
            # print("")

            # print("Rotation Vector Quaternion:")
        quat_i, quat_j, quat_k, quat_real = self.bno.quaternion  # pylint:disable=no-member
        print( "i:%d   x: %0.6f  y: %0.6f z: %0.6f  w: %0.6f" % ( self.i,quat_i, quat_j, quat_k, quat_real),flush=True)

            # ----------------------------

            # accel_x, accel_y, accel_z = self.bno.acceleration  # pylint:disable=no-member
        self.i+=1
                
            # self.imu.linear_acceleration.x=accel_x
            # self.imu.linear_acceleration.y=accel_y
            # self.imu.linear_acceleration.z=accel_z
            # self.publisher_.publish(self.imu)
            # print("I:%d   X: %0.6f  Y: %0.6f Z: %0.6f  m/s^2" % (self.i,accel_x, accel_y, accel_z))
            

        # except Exception as e:
        #     print("-----------") # An exception occurred: division by zero
        #     print("An error occurred:", e) # An exception occurred: division by zero
        #     print("-----------") # An exception occurred: division by zero

        #     #self.init_sensor()
        #     #self.bno.soft_reset()

        

def main(args=None):
    print("insilize ... ",flush=True)
    rclpy.init(args=args)

    imu_publisher = IMUPublisher()
    rclpy.spin(imu_publisher)
    imu_publisher.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()