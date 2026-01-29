import RPi.GPIO as GPIO
from time import sleep

# ピン番号の設定（BCMモード）
IN1 = 23
IN2 = 24
IN3 = 27
IN4 = 22

# 初期化
GPIO.setmode(GPIO.BCM)
GPIO.setup(IN1, GPIO.OUT)
GPIO.setup(IN2, GPIO.OUT)
GPIO.setup(IN3, GPIO.OUT)
GPIO.setup(IN4, GPIO.OUT)

def forward():
    """前進"""
    print("Forward")
    GPIO.output(IN1, GPIO.HIGH)
    GPIO.output(IN2, GPIO.LOW)
    GPIO.output(IN3, GPIO.HIGH)
    GPIO.output(IN4, GPIO.LOW)

def backward():
    """後退"""
    print("Backward")
    GPIO.output(IN1, GPIO.LOW)
    GPIO.output(IN2, GPIO.HIGH)
    GPIO.output(IN3, GPIO.LOW)
    GPIO.output(IN4, GPIO.HIGH)

def stop():
    """停止"""
    print("Stop")
    GPIO.output(IN1, GPIO.LOW)
    GPIO.output(IN2, GPIO.LOW)
    GPIO.output(IN3, GPIO.LOW)
    GPIO.output(IN4, GPIO.LOW)

def turn_left():
    """左旋回"""
    print("Turn Left")
    GPIO.output(IN1, GPIO.LOW)
    GPIO.output(IN2, GPIO.HIGH)
    GPIO.output(IN3, GPIO.HIGH)
    GPIO.output(IN4, GPIO.LOW)

def turn_right():
    """右旋回"""
    print("Turn Right")
    GPIO.output(IN1, GPIO.HIGH)
    GPIO.output(IN2, GPIO.LOW)
    GPIO.output(IN3, GPIO.LOW)
    GPIO.output(IN4, GPIO.HIGH)

try:
    print("Motor control start!")

    # テスト走行
    forward()
    sleep(2)

    stop()
    sleep(1)

    backward()
    sleep(2)

    stop()
    sleep(1)

    turn_left()
    sleep(1)

    stop()
    sleep(1)

    turn_right()
    sleep(1)

    stop()
    print("Done!")

finally:
    GPIO.cleanup()
