from __future__ import print_function
import string
from mvIMPACT import acquire

def supportsValue(prop, value):
    if prop.hasDict:
        validValues = []
        prop.getTranslationDictValues(validValues)
        return value in validValues

    if prop.hasMinValue and prop.getMinValue() > value:
        return False

    if prop.hasMaxValue and prop.getMaxValue() < value:
        return False

    return true

def conditionalSetProperty(prop, value, boSilent=False):
    if prop.isValid and prop.isWriteable and supportsValue(prop, value):
        prop.write(value)
        if boSilent == False:
            print("Property '" + prop.name() + "' set to '" + prop.readS() + "'.")

# Start the acquisition manually if this was requested(this is to prepare the driver for data capture and tell the device to start streaming data)
def manuallyStartAcquisitionIfNeeded(pDev, fi):
    if pDev.acquisitionStartStopBehaviour.read() == acquire.assbUser:
        result = fi.acquisitionStart()
        if result != acquire.DMR_NO_ERROR:
            print("'FunctionInterface.acquisitionStart' returned with an unexpected result: " + str(result) + "(" + ImpactAcquireException.getErrorCodeAsString(result) + ")")

# Stop the acquisition manually if this was requested
def manuallyStopAcquisitionIfNeeded(pDev, fi):
    if pDev.acquisitionStartStopBehaviour.read() == acquire.assbUser:
        result = fi.acquisitionStop()
        if result != acquire.DMR_NO_ERROR:
            print("'FunctionInterface.acquisitionStop' returned with an unexpected result: " + str(result) + "(" + ImpactAcquireException.getErrorCodeAsString(result) + ")")
