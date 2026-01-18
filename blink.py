from gpiozero import LED
from time import sleep

led = LED(17)

print('LED blink start!')
for i in range(10):
    led.on()
    print('ON')
    sleep(0.5)
    led.off()
    print('OFF')
    sleep(0.5)

print('Done!')
