import RPi.GPIO as GPIO
import time

GPIO.setmode(GPIO.BCM)
GPIO.setup(4, GPIO.IN)   # PIRセンサ（入力）
GPIO.setup(17, GPIO.OUT) # LED（出力）

try:
    print('PIR sensor monitoring start!')
    while True:
        if GPIO.input(4):  # 人を検知したら
            GPIO.output(17, True)  # LED点灯
            print('Motion detected! LED ON')
        else:
            GPIO.output(17, False) # LED消灯
        time.sleep(0.1)
except KeyboardInterrupt:
    print('\nStopping...')
    GPIO.cleanup()
