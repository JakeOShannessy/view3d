
version = '0.20080215'

import os
if os.environ.get('TERM')=="msys":
	deftool = ['mingw']
else:
	deftool = ['default']

env = Environment(
	tools=deftool + ['disttar','soqt']
	,toolpath=['scons']
)

#-------------
# OPTIONS

opts = Options()

opts.Add(BoolOption(
	"DEBUG"
	,"Debugging mode (for gdb symbols, plus extra output)"
	,1
))

opts.Add(
	"CC"
	,"C Compiler"
	,"gcc"
)

opts.Add(BoolOption(
	'WITH_GCCVISIBILITY'
	,"Whether to use GCC Visibility features (only applicable if available)"
	,True
))

opts.Update(env)

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

conf = Configure(env
	, custom_tests = { 
		'CheckGcc' : CheckGcc
		, 'CheckGccVisibility' : CheckGccVisibility
	}
)

if conf.CheckGcc():
	conf.env['HAVE_GCC']=True;
	if env['WITH_GCCVISIBILITY'] and conf.CheckGccVisibility():
		conf.env['HAVE_GCCVISIBILITY']=True;
		conf.env.Append(CCFLAGS=['-fvisibility=hidden'])
		conf.env.Append(CPPDEFINES=['HAVE_GCCVISIBILITY'])
	conf.env.Append(CCFLAGS=['-Wall'])

conf.Finish()

#------------
# Create the program

srcs = Split("""
	ctrans.c  heap.c  polygn.c  savevf.c  viewobs.c  viewunob.c
	getdat.c  misc.c  readvf.c  test3d.c  view3d.c  viewpp.c
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

#------------
# viewer program

soqt_env = env.Copy()
soqt_env.Append(
	CPPPATH = env.get('SOQT_CPPPATH')
	, LIBS = ['view3d'] + env.get('SOQT_LIBS')
	, LIBPATH = ['#'] + env.get('SOQT_LIBPATH')
	, CPPDEFINES = env.get('SOQT_CPPDEFINES')
)

viewer = soqt_env.Program('viewer',['viewer.cpp','render.cpp'])

#------------
# examples

env.SConscript('examples/SConscript','env');

#------------
# create distribution tarball

env['DISTTAR_FORMAT']='bz2'
env.Append(
	DISTTAR_EXCLUDEEXTS=['.o','.os','.so','.a','.dll','.cc','.cache','.pyc'
		,'.cvsignore','.dblite','.log', '.gz', '.bz2', '.zip']
	,DISTTAR_EXCLUDEDIRS=['CVS','.svn','.sconf_temp', 'dist','build']
)

tar = env.DistTar("dist/view3d-"+version
        , [env.Dir('#')]
)
