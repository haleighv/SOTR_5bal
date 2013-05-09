############################################
#
# AVRConstants.py
# Written by Doug Gallatin and Andrew Lehmer 2012
#
# Code provided as is.  Use and Modify at your own risk.
# Packaged and tested using python2.7 32 bit, pygame 1.9.1, pySerial 2.6
#
############################################

CREATE_SPRITE = 0x01
SET_POS = 0x2
SET_ROT = 0x03
SET_ORDER = 0x0C
SET_SIZE = 0x0D
DELETE_SPRITE = 0x04

CREATE_GROUP = 0x05
ADD_TO_GROUP = 0x06
REMOVE_FROM_GROUP = 0x07
DELETE_GROUP = 0x08

COLLIDE = 0x09
CREATE_WINDOW = 0x0A

PRINT = 0x0B

INT8 = 0x01
INT16 = 0x02
STRING = 0x03

ALL_GROUP = 0x00
HANDLE_ERROR = 0xFF

BAUD_RATE = 38400
