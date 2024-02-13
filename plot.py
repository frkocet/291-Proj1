import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import matplotlib.colors as mcolors
import sys, time, math
import serial

ser = serial.Serial(
port='', # Port COM
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
       strin = ser.readline()       # Read from port
       parts = strin.split('.')     # Split temp.state
       val = int(parts[0])          # temp to val
       state = int(parts[1])        # state to state
       yield t, val, state

def run(data):
    # update the data
    t,y,state = data
    if t>-1:
        xdata.append(t)
        ydata.append(y)
        statedata.append(state)
        if t>(xsize-50): # Scroll to the left.
            ax.set_xlim(t-xsize+50, t+50)

        if y>50: # Scroll to the left.
            ax.set_ylim(y-50, y+50)
        
        # Gradient stuff
        norm_y = round((220.1-(y)))
        color = mcolors.to_rgba(plt.cm.RdBu(norm_y))
        ax.plot(xdata[-3:], ydata[-3:], color=color, linewidth=4)
        ax.set_facecolor(mcolors.to_rgba(plt.cm.RdBu(norm_y+60)))
        
        if(state == 0): # chillin state
            ax.set_title('BING CHILLING', fontdict={'fontsize': 14, 'fontweight': 'bold','fontfamily':'Times New Roman'})

        if(state == 1): # ramp
            ax.set_title('Ramp')

        if(state == 2): # soak
            ax.set_title('Soak')

        if(state == 3): # peak
            ax.set_title('Peak')

        if(state == 4): # reflow
            ax.set_title('Reflow')

        if(state == 5): # coolin
            ax.set_title('Cooling')

        line.set_data(xdata, ydata)
            
    return line,

def on_close_figure(event):
    sys.exit(0)

data_gen.t = -1
fig = plt.figure()
fig.canvas.mpl_connect('close_event', on_close_figure)
ax = fig.add_subplot(111)
line, = ax.plot([], [], lw=4)
line.set_color(mcolors.to_rgba(plt.cm.RdBu(230)))
ax.set_ylim(0, 100)
ax.set_xlim(0, xsize)
ax.grid()
xdata, ydata, statedata = [], [], []
ax.set_xlabel('Time',font='Helvetica')
ax.set_ylabel('Temp',font='Helvetica')
ax.set_facecolor('darkgray')

# Important: Although blit=True makes graphing faster, we need blit=False to prevent
# spurious lines to appear when resizing the stripchart.
ani = animation.FuncAnimation(fig, run, data_gen, blit=False, interval=100, repeat=False)
plt.show()
