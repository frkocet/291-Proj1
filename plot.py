import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import matplotlib.colors as mcolors
import sys, time, math
import serial
from matplotlib import gridspec

'''
ser = serial.Serial(
port='', # Port COM
baudrate=115200,
parity=serial.PARITY_NONE,
stopbits=serial.STOPBITS_TWO,
bytesize=serial.EIGHTBITS
)
ser.isOpen()
'''

xsize=100

def data_gen():
    t = data_gen.t
    while True:
       t+=1
       # strin = ser.readline()       # Read from port
       strin = '0020.00'          # For testing
       parts = strin.split('.')     # Split temp.state
       val = t # int(parts[0])          # temp to val
       #val = abs(250.0*math.sin(t*2.0*3.1415/100.0))
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

        if y>50: # Scroll vertically.
            ax.set_ylim(y-50, y+50)
        
        # Gradient stuff
        norm_y = round((220.1-(y)))
        color = mcolors.to_rgba(plt.cm.RdBu(norm_y))
        ax.plot(xdata[-2:], ydata[-2:], color=color, linewidth=4)
        ax.set_facecolor(mcolors.to_rgba(plt.cm.RdBu(norm_y+70)))
        #ax.set_facecolor('darkgrey')
        
        if(state == 0): # chillin state
            ax.set_title('BING CHILLING', fontdict={'fontsize': 16, 'fontweight': 'bold','fontfamily':'Helvetica'})

        if(state == 1): # ramp
            ax.set_title('Ramp', fontdict={'fontsize': 16, 'fontweight': 'bold','fontfamily':'Helvetica'})

        if(state == 2): # soak
            ax.set_title('Soak', fontdict={'fontsize': 16, 'fontweight': 'bold','fontfamily':'Helvetica'})

        if(state == 3): # peak
            ax.set_title('Peak', fontdict={'fontsize': 16, 'fontweight': 'bold','fontfamily':'Helvetica'})

        if(state == 4): # reflow
            ax.set_title('Reflow', fontdict={'fontsize': 16, 'fontweight': 'bold','fontfamily':'Helvetica'})

        if(state == 5): # coolin
            ax.set_title('Cooling', fontdict={'fontsize': 16, 'fontweight': 'bold','fontfamily':'Helvetica'})

        line.set_data(xdata, ydata)

        #ax_temp_bar.fill_between([0, 1], 0, y, color=mcolors.to_rgba(plt.cm.RdBu(norm_y)))
        rect = plt.Rectangle((0, y-25), 1, 25, color=mcolors.to_rgba(plt.cm.RdBu(norm_y)))
        ax_temp_bar.add_patch(rect)
        ax_temp_bar.figure.canvas.draw()  # Draw the updated temperature bar
        
        for patch in ax_temp_bar.patches:
            if patch.get_y() > y-25:
                patch.remove()  # Remove the previous patch
            
    return line,

def on_close_figure(event):
    sys.exit(0)

data_gen.t = -1
fig = plt.figure(figsize=(12,8.5))
fig.canvas.mpl_connect('close_event', on_close_figure)

gs = gridspec.GridSpec(1, 2, width_ratios=[10, 1])  # 1 row, 2 columns

ax = fig.add_subplot(gs[0])
line, = ax.plot([], [], lw=4)
line.set_color(mcolors.to_rgba(plt.cm.RdBu(230)))
ax.set_ylim(0, 100)
ax.set_xlim(0, xsize)
ax.grid()
xdata, ydata, statedata = [], [], []
ax.set_xlabel('Time',fontdict={'fontsize': 14, 'fontweight': 'bold','fontfamily':'Helvetica'})
ax.set_ylabel('Temp',fontdict={'fontsize': 14, 'fontweight': 'bold','fontfamily':'Helvetica'})
ax.set_facecolor('darkgray')

# Temperature bar subplot
ax_temp_bar = plt.subplot(gs[1])
ax_temp_bar.set_ylim(0, 250)  # Adjust the ylim based on your temperature range
ax_temp_bar.set_xlim(0, 1)  # Adjust the xlim based on your temperature bar requirements
ax_temp_bar.set_xlabel('Bar', fontdict={'fontsize': 12, 'fontweight': 'bold','fontfamily':'Helvetica'}, labelpad=10)
ax_temp_bar.set_xticks([])
yticks_positions = np.arange(0, 260, 10) # Adjust as needed
ax_temp_bar.set_yticks(yticks_positions)

# Important: Although blit=True makes graphing faster, we need blit=False to prevent
# spurious lines to appear when resizing the stripchart.
ani = animation.FuncAnimation(fig, run, data_gen, blit=False, interval=100, repeat=False)
plt.show()

