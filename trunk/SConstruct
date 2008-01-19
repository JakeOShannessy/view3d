env = Environment()

srcs = Split("""
	Ctrans.c  Heap.c  Polygn.c  SaveVF.c  V3Main.c  ViewObs.c  ViewUnob.c
	Getdat.c  Misc.c  ReadVF.c  Test3D.c  View3D.c  ViewPP.c
""")

env.Append(CPPFLAGS=['-Wall'])

prog = env.Program(srcs)


