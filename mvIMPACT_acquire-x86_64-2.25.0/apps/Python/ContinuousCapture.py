from __future__ import print_function
import os
import platform
import string
import sys
# import all the stuff from mvIMPACT Acquire into the current scope
from mvIMPACT import acquire
# import all the mvIMPACT Acquire related helper function such as 'conditionalSetProperty' into the current scope
# If you want to use this module in your code feel free to do so but make sure the 'Common' folder resides in a sub-folder of your project then
from Common import *

# For systems with NO mvDisplay library support
#import ctypes
#import Image
#import numpy

devMgr = acquire.DeviceManager()
for i in range(devMgr.deviceCount()):
    pDev = devMgr.getDevice(i)
    print("[" + str(i) + "]: " + pDev.serial.read() + "(" + pDev.product.read() + ", " + pDev.family.read(), end='')
    if pDev.interfaceLayout.isValid:
        conditionalSetProperty(pDev.interfaceLayout, acquire.dilGenICam)
        print(", interface layout: " + pDev.interfaceLayout.readS(), end='')
    if pDev.acquisitionStartStopBehaviour.isValid:
        conditionalSetProperty(pDev.acquisitionStartStopBehaviour, acquire.assbUser)
        print(", acquisition start/stop behaviour: " + pDev.acquisitionStartStopBehaviour.readS(), end='')
    if pDev.isInUse():
        print(", !!!ALREADY IN USE!!!", end='')
    print(")")

print("Please enter the number in front of the listed device followed by [ENTER] to open it: ", end='')
devNr = int(raw_input())
if (devNr < 0) or (devNr >= devMgr.deviceCount()):
    print("Invalid selection!")
    sys.exit(-1)

pDev = devMgr.getDevice(devNr)
pDev.open()

print("Please enter the number of buffers to capture followed by [ENTER]: ", end='')
framesToCapture = int(raw_input())
if framesToCapture < 1:
    print("Invalid input! Please capture at least one image")
    sys.exit(-1)

# The mvDisplay library is only available on Windows systems for now
isDisplayModuleAvailable = platform.system() == "Windows"
if isDisplayModuleAvailable:
    display = acquire.ImageDisplayWindow("A window created from Python")
else:
    print("The mvIMPACT Acquire display library is not available on this('" + platform.system() + "') system. Consider using the PIL(Python Image Library) and numpy(Numerical Python) packages instead. Have a look at the source code of this application to get an idea how.")

# For systems with NO mvDisplay library support
#channelType = numpy.uint16 if channelBitDepth > 8 else numpy.uint8

fi = acquire.FunctionInterface(pDev)
statistics = acquire.Statistics(pDev)

while fi.imageRequestSingle() == acquire.DMR_NO_ERROR:
    print("Buffer queued")
pPreviousRequest = None

manuallyStartAcquisitionIfNeeded(pDev, fi)
for i in range(framesToCapture):
    requestNr = fi.imageRequestWaitFor(-1)
    if fi.isRequestNrValid(requestNr):
        pRequest = fi.getRequest(requestNr)
        if pRequest.isOK():
            if i%100 == 0:
                print("Info from " + pDev.serial.read() +
                         ": " + statistics.framesPerSecond.name() + ": " + statistics.framesPerSecond.readS() +
                         ", " + statistics.errorCount.name() + ": " + statistics.errorCount.readS() +
                         ", " + statistics.captureTime_s.name() + ": " + statistics.captureTime_s.readS())
            if isDisplayModuleAvailable:
                display.GetImageDisplay().SetImage(pRequest)
                display.GetImageDisplay().Update()
            # For systems with NO mvDisplay library support
            #cbuf = (ctypes.c_char * imageSize).from_address(long(req.imageData.read()))
            #arr = numpy.fromstring(cbuf, dtype = channelType)
            #arr.shape = (height, width, channelCount)

            #if channelCount == 1:
            #    img = Image.fromarray(arr)
            #else:
            #    img = Image.fromarray(arr, 'RGBA' if alpha else 'RGB')
        if pPreviousRequest != None:
            pPreviousRequest.unlock()
        pPreviousRequest = pRequest
        fi.imageRequestSingle()
    else:
        print("imageRequestWaitFor failed (" + str(requestNr) + ", " + ImpactAcquireException.getErrorCodeAsString(requestNr) + ")")
manuallyStopAcquisitionIfNeeded(pDev, fi)
raw_input("Press Enter to continue...")