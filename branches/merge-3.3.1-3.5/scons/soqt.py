import os, platform
from SCons.Script import *

def generate(env):
	"""
	Detect SOQT settings and add them to the environment.
	"""
	try:
		if platform.system()=="Windows":
			import _winreg
			x=_winreg.ConnectRegistry(None,_winreg.HKEY_LOCAL_MACHINE)
			y= _winreg.OpenKey(x,r"SOFTWARE\coin3d")
			LIB,t = _winreg.QueryValueEx(y,"INSTALL_LIB")
			BIN,t = _winreg.QueryValueEx(y,"INSTALL_BIN")
			INCLUDE,t = _winreg.QueryValueEx(y,"INSTALL_INCLUDE")
			
			env.AppendUnique(
				SOQT_CPPPATH = [INCLUDE]
				,SOQT_LIBPATH = [LIB]
				,SOQT_LIBS = ['SoQt', 'Coin']
				,SOQT_CPPDEFINES = ['COIN_DLL','SOQT_DLL']
			)
			
			env.AppendUnique(
				QT_PREFIX = r'c:/Qt/2010.05/qt'
				,SOQT_CPPPATH = ["$QT_PREFIX/include","$QT_PREFIX/include/Qt"]
				,SOQT_LIBPATH = ["$QT_PREFIX/lib"]
			)
			
		else:
			cmd = ['soqt-config','--cppflags','--ldflags','--libs']
			env1 = env.Clone()
			env1.ParseConfig(cmd)
			env['SOQT_CPPPATH'] = env1.get('CPPPATH')
			env['SOQT_LIBPATH'] = env1.get('LIBPATH')
			env['SOQT_LIBS'] = env1.get('LIBS')
			env['SOQT_CPPDEFINES'] = env1.get('CPPDEFINES')

		print "SOQT_LIBS =",env.get('SOQT_LIBS')
		print "SOQT_LIBPATH =",env.get('SOQT_LIBPATH')
		print "SOQT_CPPPATH =",env.get('SOQT_CPPPATH')
		print "SOQT_CPPDEFINES =",env.get('SOQT_CPPDEFINES')

	except Exception,e:
		print "FAILED TO SET UP SOQT",str(e)
		pass

def exists(env):
	"""
	Make sure this tool exists.
	"""
	if platform.system()=="Windows":
		try:
			import _winreg
			x=_winreg.ConnectRegistry(None,_winreg.HKEY_LOCAL_MACHINE)
			y= _winreg.OpenKey(x,r"SOFTWARE\coin3d")
			INCLUDE,t = _winreg.QueryValueEx(y,'INSTALL_INCLUDE')
			return True
		except:
			return False
	else:
		if env.Which('soqt-config'):
			return True
		return False

