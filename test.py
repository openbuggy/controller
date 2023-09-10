import serial, time

with serial.Serial('/dev/ttyACM1', 115200, timeout=1) as ser:
    throttle = 127
    steering = 127
    delta = 1
    while True:
        print(throttle, steering)
        ser.write(bytes([int(throttle),0,int(steering),0]))
        throttle += delta
        steering += delta
        if steering == 255 or steering == 0:
            delta *= -1
        time.sleep(0.05)
