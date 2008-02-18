import os, platform
from SCons.Script import *

def generate(env):
	"""
	Detect GTK+ settings and add them to the environment.
	"""
	try:
		if platform.system()=="Windows":
			import _winreg
			x=_winreg.ConnectRegistry(None,_winreg.HKEY_LOCAL_MACHINE)
			y= _winreg.OpenKey(x,r"SOFTWARE\soqt")
			LIB,t = _winreg.QueryValueEx(y,"INSTALL_LIB")
			BIN,t = _winreg.QueryValueEx(y,"INSTALL_BIN")
			INCLUDE,t = _winreg.QueryValueEx(y,"INSTALL_INCLUDE")

			env['GTK_CPPPATH'] = [INCLUDE]
			env['GTK_LIBPATH'] = [LIB]
			env['GTK_LIBS'] = ['gtk']
		else:
			cmd = ['pkg-config','gtk+-2.0','--cflags','--libs']
			env1 = env.Copy()
			env1.ParseConfig(cmd)
			env['GTK_CPPPATH'] = env1.get('CPPPATH')
			env['GTK_LIBPATH'] = env1.get('LIBPATH') or []
			env['GTK_LIBS'] = env1.get('LIBS')

		#print "GTK_LIBS =",env.get('GTK_LIBS')
		#print "GTK_LIBPATH =",env.get('GTK_LIBPATH')
		#print "GTK_CPPPATH =",env.get('GTK_CPPPATH')

	except:
		print "FAILED TO SET UP GTK"
		pass

def exists(env):
	"""
	Make sure this tool exists.
	"""
	if platform.system()=="Windows":
		try:
			import _winreg
			x=_winreg.ConnectRegistry(None,_winreg.HKEY_LOCAL_MACHINE)
			y= _winreg.OpenKey(x,r"SOFTWARE\gtk")
			INCLUDE,t = _winreg.QueryValueEx(y,'INSTALL_INCLUDE')
			return True
		except:
			return False
	else:
		if env.Execute(['pkg-config','gtk+-2.0']):
			return True
		return False

