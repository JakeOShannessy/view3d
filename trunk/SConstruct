
version = '0.20080119'

env = Environment(
	tools=['default','disttar']
	,toolpath=['scons']
)

#------------
# Create the program

srcs = Split("""
	Ctrans.c  Heap.c  Polygn.c  SaveVF.c  V3Main.c  ViewObs.c  ViewUnob.c
	Getdat.c  Misc.c  ReadVF.c  Test3D.c  View3D.c  ViewPP.c
	strcmpi.c
""")

env.Append(
	CPPFLAGS=['-Wall']
	,LIBS=['m']
)

prog = env.Program('view3d', srcs)

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
