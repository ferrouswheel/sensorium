import platform         # os identification

import serial           # serial output
import time             # delays

from PIL import Image   # PIL
import numpy as np      # arrays
from scipy import ndimage

from threading import Thread
from Queue import Queue
from DemoTransmitter import DemoTransmitter
import socket


class Transform(object):

    def transform(self, data):
        return data


class HemisphereTransform(object):

    def transform(self, data):
        pass


class Teensy(object):

    def __init__(self):
        self.transform = None


class OctoSerial(object):

    def get_serial_location(self):
        os = platform.platform()
        if ('Linux' in os):
            serialPort = '/dev/ttyACM0'
        elif ('Windows' in os):
            serialPort = 'COM6'
        else:
            # should not get here
            serialPort = 'dummy'
        return serialPort

    def __init__(self):
        self.boards = []
        self.serial_init(serial_port)

    def serial_init(self, serial_port):




