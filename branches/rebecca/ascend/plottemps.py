import extpy
browser = extpy.getbrowser()

import ascpy
import re

try:
	from pylab import *
except:
	pass

def plotwindowtemps(self):
	"""Plot temperatures across the cavity window"""

	assert self.__class__.__name__=="Instance"
	assert self.isModel()

	import loading
	loading.load_matplotlib(throw=True)

	A = float(self.A_window)
	n = 12
	x = (array(range(n)) - n/2. + 1./2) * A / n;
	y = [float(self.T_window[i]) for i in range(n)]

	ioff()
	figure()
	hold(1)

	plot(x,y,"y-")
	plot(x,y,"ro")
	xlabel('Position of slice midpoint (m)')
	ylabel('Temperature (K)')

	extpy.getbrowser().reporter.reportNote("Plotting completed")
	ion()
	show()

def plottoptemps(self):
	"""Plot temperatures across the cavity window"""

	assert self.__class__.__name__=="Instance"
	assert self.isModel()

	import loading
	loading.load_matplotlib(throw=True)

	A = float(self.A_window)
	n = 12
	x = (array(range(n)) - n/2. + 1./2) * A / n;
	y = [float(self.T_top[i]) for i in range(n)]

	ioff()
	figure()
	hold(1)

	plot(x,y,"y-")
	plot(x,y,"ro")
	xlabel('Position of slice midpoint (m)')
	ylabel('Temperature (K)')

	extpy.getbrowser().reporter.reportNote("Plotting completed")
	ion()
	show()

extpy.registermethod(plotwindowtemps)
extpy.registermethod(plottoptemps)

