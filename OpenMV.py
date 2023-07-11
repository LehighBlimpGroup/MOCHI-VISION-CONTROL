# Mochi Control - By: karenli - Wed Jul 5 2023

import sensor, image, time, network, pyb, rpc, omv
from pyb import UART

#omv.disable_fb(True)

#Network credentials
SSID='AIRLab-BigLab'
KEY='Airlabrocks2022'

#Threshold for balloon in LAB color space
#This is the threshold for red ball under sufficient lighting
blob_threshold = (30, 60, 45, 80, 45, 70)


# Image sensor initialization
def init_sensor(pixformat=sensor.RGB565, framesize=sensor.QQVGA, windowsize=None,
                autogain=False,  gain=2,  exposure=10000, contrast=0, saturation=0,
                vflip=False, hmirror=False):
    sensor.reset()
    sensor.set_pixformat(pixformat)
    sensor.set_framesize(framesize)
    if windowsize:
        sensor.set_windowing(windowsize)
    sensor.set_auto_gain(autogain, gain_db=gain)
    sensor.set_auto_whitebal(False) # must be turned off for color tracking
    sensor.set_auto_exposure(False, exposure)
    sensor.set_contrast(contrast)
    sensor.set_saturation(saturation)
    sensor.set_vflip(vflip)
    sensor.set_hmirror(hmirror)
    sensor.skip_frames(time = 1000)
    sensor.__write_reg(0x0E, 0b00000000)
    sensor.__write_reg(0x3E, 0b00000000)
    #sensor.__write_reg(0x01, 0b11000000)
    #sensor.__write_reg(0x02, 0b01111000)
    #sensor.__write_reg(0x03, 0b01110100)
    sensor.__write_reg(0x2D, 0b00000000)
    sensor.__write_reg(0x2E, 0b00000000)
    sensor.__write_reg(0x35, 0b10000000)
    sensor.__write_reg(0x36, 0b10000000)
    sensor.__write_reg(0x37, 0b10000000)
    sensor.__write_reg(0x38, 0b10000000)
    sensor.__write_reg(0x39, 0b10000000)
    sensor.__write_reg(0x3A, 0b10000000)
    sensor.__write_reg(0x3B, 0b10000000)
    sensor.__write_reg(0x3C, 0b10000000)
    sensor.skip_frames(time = 1000)


#Setting up netwaork interface for streaming
def setup_network(ssid, key):
    network_if = network.WLAN(network.STA_IF)
    network_if.active(True)
    network_if.connect(ssid, key)
    print("Trying to connect... (may take a while)...")
    print(network_if.ifconfig)
    interface = rpc.rpc_network_slave(network_if)
    return interface


#Blink LED indicating resetting
def setting_up():
    print("setting up the device...")
    green_led.on()
    time.sleep_ms(1000)
    green_led.off()
    time.sleep_ms(500)
    green_led.on()
    time.sleep_ms(1000)
    green_led.off()


#iBus protocol checksum
def checksum(arr, initial= 0):
    sum = initial
    for a in arr:
        sum += a
    checksum = 0xFFFF - sum
    chA = checksum >> 8
    chB = checksum & 0xFF
    return chA, chB


def uart_message(half_frame=sensor.width()/2):
    img = sensor.snapshot()     # Take a snapshot
    img.lens_corr(strength=1.6) # Make camera lens less distorted
    #iBus protocol
    msg = bytearray(32)         # 16 pairs of bytes
    msg[0] = 0x20               # start bytes (the other end must synchronize to this pattern)
    msg[1] = 0x40
    #Blob detection to find colored ballons
    blobs = img.find_blobs([blob_threshold], merge=True, area_threshold=25, pixels_threshold=10)
    #If there's any detection
    if blobs:
        red_led.on()
        max_blob = find_max(blobs)
        # These values are stable all the time.
        img.draw_rectangle(max_blob.rect())
        img.draw_cross(max_blob.cx(), max_blob.cy())
        x_value = max_blob.cx()
        y_value = max_blob.cy()
        print(x_value)
        print(y_value)
        cx_msg = bytearray(x_value.to_bytes(2, 'little'))
        msg[2] = cx_msg[0]
        msg[3] = cx_msg[1]
        cy_msg = bytearray(y_value.to_bytes(2, 'little'))
        msg[4] = cy_msg[0]
        msg[5] = cy_msg[1]
        time.sleep_ms(20)
    #If there's no detections
    else:
        msg[2] = 0x0
        msg[3] = 0x0
        msg[4] = 0x0
        msg[5] = 0x0
        red_led.off()
    #iBus protocol checksum
    chA, chB = checksum(msg[:-2], 0)
    msg[-1] = chA
    msg[-2] = chB
    uart.write(msg)             # send 32 byte message
    print(msg)


#Live streaming callback functions
def stream_generator_cb():
    uart_message()
    return sensor.snapshot().compress(quality=90).bytearray()

def jpeg_image_stream_cb():
    interface.stream_writer(stream_generator_cb)

def jpeg_image_stream(data):
    pixformat, framesize = bytes(data).decode().split(",")
    sensor.set_pixformat(eval(pixformat))
    sensor.set_framesize(eval(framesize))
    interface.schedule_callback(jpeg_image_stream_cb)
    return bytes()


#Finding the biggest detected blob
def find_max(blobs):
    max_size = 0;
    for blob in blobs:
        if blob[2]*blob[3] > max_size:
            max_blob = blob
            max_size = blob[2]*blob[3]
    return max_blob


if __name__ == "__main__":

    #Initialize LED
    red_led = pyb.LED(1)
    green_led = pyb.LED(2)
    blue_led = pyb.LED(3)

    #Initialize UART on OpenMV H7
    uart = UART(1, 115200, timeout_char=2000) # (TX, RX) = (P1, P0) = (PB14, PB15)

    #Initialize clock
    clock = time.clock()

    #Sensor setup
    setting_up()
    init_sensor(exposure=20000, saturation=2)

    #Network setup
    #interface = setup_network(SSID, KEY)

    #Loop
    while(True):
        uart_message()
        ##Video streaming ***Comment out when not needed
        #interface.register_callback(jpeg_image_stream)
        #interface.loop()

