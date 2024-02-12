import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import matplotlib.colors as mcolors
import sys, time, math
import serial

ser = serial.Serial(
port='/dev/cu.usbserial-D30HLDBJ',
baudrate=115200,
parity=serial.PARITY_NONE,
stopbits=serial.STOPBITS_TWO,
bytesize=serial.EIGHTBITS
)
ser.isOpen()

xsize=100
   
def data_gen():
    t = data_gen.t
    while True:
       t+=1
       #val=abs(250.0*math.sin(t*2.0*3.1415/100.0))
       #val=t
       strin = ser.readline()
       strin = strin.rstrip()
       strin = strin.decode()
       val=float(strin)
       yield t, val

def run(data):
    # update the data
    t,y = data
    if t>-1:
        xdata.append(t)
        ydata.append(y)
        if t>(xsize-50): # Scroll to the left.
            ax.set_xlim(t-xsize+50, t+50)

        if y>50: # Scroll to the left.
            ax.set_ylim(y-50, y+50)
            ax.set
        
        norm_y = round((220.1-(y)))
        color = mcolors.to_rgba(plt.cm.RdBu(norm_y))

        ax.plot(xdata[-3:], ydata[-3:], color=color, linewidth=5)
        # line.set_data(xdata, ydata)

    return line,

def on_close_figure(event):
    sys.exit(0)

data_gen.t = -1
fig = plt.figure()
fig.canvas.mpl_connect('close_event', on_close_figure)
ax = fig.add_subplot(111)
line, = ax.plot([], [], lw=5)
line.set_color(mcolors.to_rgba(plt.cm.RdBu(230)))
ax.set_ylim(0, 100)
ax.set_xlim(0, xsize)
ax.grid()
xdata, ydata = [], []
colors = []
ax.set_xlabel('Time',font='Helvetica')
ax.set_ylabel('Temp',font='Helvetica')


# Important: Although blit=True makes graphing faster, we need blit=False to prevent
# spurious lines to appear when resizing the stripchart.
ani = animation.FuncAnimation(fig, run, data_gen, blit=False, interval=100, repeat=False)
plt.show()
