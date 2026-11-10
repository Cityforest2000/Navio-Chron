import pystray
import PIL.Image
import subprocess

global NavioNexus
bStarted = False

image = PIL.Image.open("icon.png")
strLogFile = "NavioNexus.log"
logFile = open(strLogFile, "w", encoding="utf-8")


def on_clicked(icon, item):
    print ("Hello")
    
def miStartDriver(icon, item):
    global NavioNexus
    global bStarted
    if not bStarted:
        NavioNexus = subprocess.Popen('python "NavioNexus3b.py"', stdout=logFile)
        bStarted = True
        print ("Start Navio Nexus Driver")
    else:
        print ("The Navio Nexus driver has been started.")

def miStopDriver(icon, item):
    global NavioNexus
    global bStarted
    if bStarted:
        NavioNexus.terminate()
        print ("Stop Navio Nexus Driver")
        bStarted = False
    else:
        print ("The Navio Nexus driver has been stopped.")

def miReStartDriver(icon, item):
    miStopDriver(icon, item)
    miStartDriver(icon, item)

def miSetting(icon, item):
    subprocess.Popen(['python', 'NavioConsole1b.py'])

def miExit(icon, item):
    if bStarted:
        print ("Navio Nexus Driver is running, please stop the driver first.")
    else:
        print ("Exit")
        logFile.close()
        icon.stop()

def miStopExit(icon, item):
    miStopDriver(icon, item)
    miExit(icon, item)

icon = pystray.Icon("NavioChron", image, menu=pystray.Menu(
    pystray.MenuItem("Start NavioNexus Driver", miStartDriver),
    pystray.MenuItem("Stop NavioNexus Driver", miStopDriver),
    pystray.MenuItem("Restart NavioNexus Driver", miReStartDriver),
    pystray.Menu.SEPARATOR,
    pystray.MenuItem("Navio Console", miSetting),
    pystray.Menu.SEPARATOR,
    pystray.MenuItem("Stop Driver && &Exit", miStopExit)
))

icon.run()
