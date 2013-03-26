#!/usr/bin/env python

version = '3.5'

import os, platform
if platform.system()=="Windows":
	deftool = ['mingw','nsis']
else:
	deftool = ['default']

env = Environment(
	tools=deftool + ['disttar','soqt','gtk','substinfile']
	,toolpath=['scons']
)

#-------------
# OPTIONS

opts = Variables()

opts.Add(BoolVariable(
	"DEBUG"
	,"Debugging mode (for gdb symbols, plus extra output)"
	,1
))

opts.Add(
	"CC"
	,"C Compiler"
	,"gcc"
)

opts.Add(BoolVariable(
	'WITH_GCCVISIBILITY'
	,"Whether to use GCC Visibility features (only applicable if available)"
	,True
))

opts.Add(
	'WIN_INSTALLER_NAME'
	,'Windows Installer name'
	,'view3d-%s.exe' % version
)


opts.Update(env)

Help(opts.GenerateHelpText(env))

#------------
# Configuration tests

# GCC

gcc_test_text = """
#ifndef __GNUC__
# error "Not using GCC"
#endif

int main(void){
	return __GNUC__;
}
"""

def CheckGcc(context):
	context.Message("Checking for GCC... ")
	is_ok = context.TryCompile(gcc_test_text,".c")
	context.Result(is_ok)
	return is_ok

# GCC VISIBILITY feature

gccvisibility_test_text = """
#if __GNUC__ < 4
# error "Require GCC version 4 or newer"
#endif

__attribute__ ((visibility("default"))) int x;

int main(void){
	extern int x;
	x = 4;
}
"""

def CheckGccVisibility(context):
	context.Message("Checking for GCC 'visibility' capability... ")
	if not context.env.has_key('WITH_GCCVISIBILITY') or not env['WITH_GCCVISIBILITY']:
		context.Result("disabled")
		return 0
	is_ok = context.TryCompile(gccvisibility_test_text,".c")
	context.Result(is_ok)
	return is_ok

soqt_test_code = """
#include <Inventor/Qt/SoQt.h>
#include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
#include <Inventor/nodes/SoCube.h>

int main(int argc, char **argv){
	QWidget * mainwin = SoQt::init(argv[0]);
	SoCube * cube = new SoCube;
   return 0;
}
"""
	
def CheckSoQt(context):
	context.Message("Checking for SoQt... ")
	if not context.env.get("SOQT_LIBS"):		
		context.Result(False)
		return False
	old_env = context.env.Clone()
	context.env.Append(
		CPPPATH = env.get('SOQT_CPPPATH')
		, LIBS = env.get('SOQT_LIBS')
		, LIBPATH = env.get('SOQT_LIBPATH')
		, CPPDEFINES = env.get('SOQT_CPPDEFINES')
	)
	res = context.TryLink(soqt_test_code,".cpp")
	context.Result(res)
	for i in ['LIBS','CPPPATH','LIBPATH','CPPDEFINES']:
		if old_env.get(i) is not None:
			context.env[i] = old_env[i]
		else:
			del context.env[i]
	return res

gtk_test_code = """
#include <gtk/gtk.h>

int main (int argc, char *argv[]){
  GtkWidget *window;
  gtk_init (&argc, &argv);
  gtk_main ();
  return 0;
}
"""

def CheckGTK(context):
	context.Message("Checking for GTK+... ")
	if not context.env.get("GTK_LIBS"):
		context.Result(False)
		return False
	old_env = context.env.Clone()
	context.env.Append(
		CPPPATH = env.get('GTK_CPPPATH')
		, LIBS = env.get('GTK_LIBS')
		, LIBPATH = env.get('GTK_LIBPATH')
		, CPPDEFINES = env.get('GTK_CPPDEFINES')
	)
	res = context.TryLink(gtk_test_code,".c")
	context.Result(res)
	for i in ['LIBS','CPPPATH','LIBPATH','CPPDEFINES']:
		if old_env.get(i) is not None:
			context.env[i] = old_env[i]
		else:
			del context.env[i]
	return res

conf = Configure(env
	, custom_tests = { 
		'CheckGcc' : CheckGcc
		, 'CheckGccVisibility' : CheckGccVisibility
		, 'CheckSoQt' : CheckSoQt
		, 'CheckGTK' : CheckGTK
	}
)

if conf.CheckGcc():
	conf.env['HAVE_GCC']=True;
	if env['WITH_GCCVISIBILITY'] and conf.CheckGccVisibility():
		conf.env['HAVE_GCCVISIBILITY']=True;
		conf.env.Append(CCFLAGS=['-fvisibility=hidden'])
		conf.env.Append(CPPDEFINES=['HAVE_GCCVISIBILITY'])
	conf.env.Append(CCFLAGS=['-Wall'])

conf.env['HAVE_SOQT'] = conf.CheckSoQt()

conf.env['HAVE_GTK'] = conf.CheckGTK()

conf.Finish()

#------------
# Create 'config.h'

env.Append(SUBST_DICT={
	"@V3D_VERSION@":str(version)
})

config = env.SubstInFile('config.h.in')

env['PROGS'] = []
env['SKIPPED'] = []

#------------
# Create the program

srcs = Split("""
	ctrans.c  heap.c  polygn.c  savevf.c  viewobs.c  viewunob.c
	getdat.c  misc.c  readvf.c  readvs.c  test3d.c  view3d.c  viewpp.c
	common.c 

	view2d.c test2d.c
""")

env.Append(
	CPPDEFINES=['ANSI']
	,LIBS=['m']
)

if env.get('DEBUG'):
	env.Append(CPPDEFINES=['_DEBUG'])
	env.Append(CPPFLAGS=['-g'])

lib = env.SharedLibrary('view3d',srcs)

prog = env.Program('view3d', ['v3main.c'], LIBS=['view3d'], LIBPATH=['#'])

prog2d = env.Program('view2d', ['v2main.c'], LIBS=['view3d'], LIBPATH=['#'])

env['PROGS'] += [prog,prog2d]

#------------
# ViewHT heat transfer calculation program

v3d_env = env.Clone()
v3d_env.Append(
	LIBS = ['view3d']
	, LIBPATH = ['#']
)

prog_viewht = v3d_env.Program('viewht', ['viewht.c'])

env['PROGS'].append(prog_viewht)

#------------
# 3D viewer program

if env['HAVE_SOQT']:
	soqt_env = v3d_env.Clone()
	soqt_env.Append(
		CPPPATH = env.get('SOQT_CPPPATH')
		, LIBS = env.get('SOQT_LIBS')
		, LIBPATH = env.get('SOQT_LIBPATH')
		, CPPDEFINES = env.get('SOQT_CPPDEFINES')
	)

	viewer = soqt_env.Program('viewer',['viewer.cpp','render.cpp'])

	env['PROGS'].append(viewer)
else:
	env['SKIPPED'].append(('viewer','SoQT not found'))

#------------
# 2D viewer program

if env['HAVE_GTK']:
	gtk_env = v3d_env.Clone()
	gtk_env.Append(
		CPPPATH = env.get('GTK_CPPPATH')
		, LIBS = env.get('GTK_LIBS')
		, LIBPATH = env.get('GTK_LIBPATH')
		, CPPDEFINES = env.get('GTK_CPPDEFINES')
		, CCFLAGS = env.get('GTK_CCFLAGS')
	)

	viewer2d = gtk_env.Program('viewer2d',['viewer2d.c'])

	env['PROGS'].append(viewer2d)
else:
	env['SKIPPED'].append(('viewer2d','GTK+ not found'))

#------------
# examples

env.SConscript('examples/SConscript','env');

#------------
# ASCEND tools for use with View3D/View2D.

env.SConscript('ascend/SConscript','env');

#------------
# create distribution tarball

env['DISTTAR_FORMAT']='bz2'
env.Append(
	DISTTAR_EXCLUDEEXTS=['.o','.os','.so','.a','.dll','.cc','.cache','.pyc'
		,'.cvsignore','.dblite','.log', '.gz', '.bz2', '.zip', '.pdf']
	,DISTTAR_EXCLUDEDIRS=['CVS','.svn','.sconf_temp', 'dist','build']
)

tar = env.DistTar("dist/view3d-"+version
        , [env.Dir('#')]
)

#-----------
# Installer

if platform.system()=="Windows":
	env.Append(NSISDEFINES={
		'OUTFILE':"#dist/"+env['WIN_INSTALLER_NAME']
		,'VERSION':version
	})
	
	installer = env.Installer('nsis/installer.nsi')
	Depends(installer,env['PROGS'])
	env.Alias('installer',installer)
	
#-----------
# Default build target

env.Default(env['PROGS'] + ['ascend'])

if len(env['SKIPPED']):
	print "\nWARNING: Unable to build the following items:"
	print "  %-20s\t%s" % ("ITEM","REASON")
	for i in env['SKIPPED']:
		print "  %-20s\t%s" % (i[0],i[1])
	print

