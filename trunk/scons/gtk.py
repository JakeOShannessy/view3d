import os, platform,sys
from SCons.Script import *

def generate(env):
	"""
	Detect GTK+ settings and add them to the environment.
	"""
	try:
		if platform.system()=="Windows":
			pathsep = os.environ['PATH'].split(";")
			pp = [os.path.abspath(os.path.join(os.path.expanduser(p),"pkg-config.exe")) for p in pathsep]
			Path = None
			for p in pp:
				if os.path.exists(p):
					#print "pkg-config at",p
					Path = p
			if not Path:
				raise RuntimeError("Could not find 'pkg-config.exe' in your PATH")
				
			#print "PATH =",Path
			cmd = [Path,'gtk+-2.0','--cflags','--libs']
			env1 = env.Copy()
			env1.ParseConfig(cmd)
			env.Append(
				GTK_CPPPATH = env1.get('CPPPATH') or []
				,GTK_CPPDEFINES = env1.get('CPPDEFINES') or []
				,GTK_LIBPATH = env1.get('LIBPATH') or []
				,GTK_LIBS = env1.get('LIBS') or []
				,GTK_CCFLAGS = env1.get('CCFLAGS') or []
			)
		else:
			cmd = ['pkg-config','gtk+-2.0','--cflags','--libs']
			env1 = env.Clone()
			env1.ParseConfig(cmd)
			env.Append(
				GTK_CPPPATH = env1.get('CPPPATH') or []
				,GTK_CPPDEFINES = env1.get('CPPDEFINES') or []
				,GTK_LIBPATH = env1.get('LIBPATH') or []
				,GTK_LIBS = env1.get('LIBS') or []
				,GTK_CCFLAGS = env1.get('CCFLAGS') or []
			)

		#print "GTK_LIBS =",env.get('GTK_LIBS')
		#print "GTK_LIBPATH =",env.get('GTK_LIBPATH')
		#print "GTK_CPPPATH =",env.get('GTK_CPPPATH')

	except Exception,e:
		print "FAILED TO SET UP GTK (%s)" % str(e)
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

