from gpiozero import PWMLED
from time import sleep

led = PWMLED(17)

print('LED fade start!')

# ゆっくり明るくする（0%から100%へ）
print('Fading in...')
for brightness in range(0, 101, 1):
    led.value = brightness / 100.0
    sleep(0.02)

# ゆっくり暗くする（100%から0%へ）
print('Fading out...')
for brightness in range(100, -1, -1):
    led.value = brightness / 100.0
    sleep(0.02)

print('Done!')
