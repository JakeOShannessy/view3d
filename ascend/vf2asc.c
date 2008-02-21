#include "readvf.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

const char defaultoutfile[] = "view3d.a4c";

void usage(const char *progname){
	fprintf(stderr,
			"Usage: %s [-o [OUTFILE]] INFILE\n"
			"Load a view factor file and convert it to initialisation code\n"
			"to allow it to be read into ASCEND for the purpose of equation-\n"
			"based modelling.\n"
			"  INFILE        View2D/View3D output file (areas and view factors,\n"
			"                eg 'out.txt')\n"
			"  -v VS2FILE    Vertex/Surface definition file. If provided, will\n"
			"                be used to label surfaces in the ASCEND model. If\n"
			"                not provided, integer surface numbers are used.\n"
			"  -o [OUTFILE]  ASCEND model file to be output. This file will\n"
			"                be set up as a refinement of the 'cavity' model\n"
			" 	             and needs to be further refines to add the\n"
			"                desired boundary conditions to define the problem.\n" 
			"                If no '-o' option is given, output goes to stdout.\n"
			"                If '-o' but no OUTFILE, we use OUTFILE = '%s'.\n"
		, progname, defaultoutfile
	);	
}

int main(int argc, char **argv){

	short infotext = 0;
	const char *outfile = NULL;
	const char *vs2file = NULL;
	int i,j;

	char c;	
	while((c=getopt(argc,argv,"v:o::"))!=-1){
		switch(c){
			case 'o':
				if(optarg){
					outfile = optarg;
				}else{
					outfile = defaultoutfile;
				}
				break;
			case 'v':
				vs2file = optarg;
				break;
			case '?':
				usage(argv[0]);
				exit(1);
		}
	}

	if(optind != argc-1){
		fprintf(stderr,"%s: missing command-line argument (need a filename)\n",argv[0]);
		usage(argv[0]);
		exit(2);
	}
	const char *filename = argv[optind];

	fprintf(stderr,"Opening file '%s' of calculated view factors...",filename);

	ViewFactors *V = NULL;
	V = read_view_factors(filename);

	if(V==NULL)return 1;
	fprintf(stderr,"done\n");

	fprintf(stderr,"Got view factors for %d surfaces.\n",V->nsrf);

	if(vs2file){
		fprintf(stderr,"Opening file '%s' of vertex/surface data...",vs2file);


	}


	FILE *of;
	if(outfile){
		of = fopen(outfile,"w");
		if(of==NULL){
			fprintf(stderr,"Unable to open '%s' for writing.\n",outfile);
			exit(2);
		}
	}else{
		of = stdout;
	}
	
	fprintf(of,"REQUIRE \"johnpye/cavity.a4c\";\n\n");
	fprintf(of,"MODEL view3d REFINES cavity_base;\n");

	fprintf(of,"\t(* surface names *)\n");
	fprintf(of,"\tn :== [");
	for(i=1; i<=V->nsrf; ++i){
		if(i>1)fprintf(of,",");
		fprintf(of,"'%d'",i);
	}
	fprintf(of,"];\n");

	fprintf(of,"METHODS\n");
	fprintf(of,"METHOD on_load;\n");
	fprintf(of,"\tRUN reset; RUN values;\n");
	fprintf(of,"END on_load;\n\n");
	fprintf(of,"METHOD specify;\n");
	fprintf(of,"\tFIX A[n];\n");
	fprintf(of,"\tFIX eps[n];\n");
	fprintf(of,"\tFIX F[n][n];\n");
#if 0
	fprintf(of,"\tFIX ");
	for(i=1; i<=V->nsrf; ++i){
		if(i>1)fprintf(of,", ");
		fprintf(of,"A['%d']",i);
	}
	fprintf(of,";\n");
	fprintf(of,"\tFIX ");
	for(i=1; i<=V->nsrf; ++i){
		if(i>1)fprintf(of,", ");
		fprintf(of,"eps['%d']",i);
	}
	for(i=1; i<=V->nsrf; ++i){
		fprintf(of,";\n\tFIX ");
		for(j=1; j<=V->nsrf; ++j){
			if(j>1)fprintf(of,", ");
			fprintf(of,"F['%d']['%d']",i,j);
		}
	}
	fprintf(of,";\n");
#endif
	fprintf(of,"END specify;\n");
	fprintf(of,"METHOD values;\n");
	fprintf(of,"\t(* areas *)\n");
	for(i=1; i<=V->nsrf; ++i){
		fprintf(of,"\tA['%d'] := %0.8g {m};\n",i,V->area[i]);
	}
	fprintf(of,"\n\t(* view factors *)\n");
	fprintf(of,"\tF[n][n] := 0;\n");
	for(i=1; i<=V->nsrf; ++i){
		for(j=1; j<=V->nsrf; ++j){
			if(V->F[i][j]!=0.0){
				fprintf(of,"\tF['%d']['%d'] := %0.8g;\n",i,j,V->F[i][j]);
			}
		}
	}
	fprintf(of,"\n\t(* emissivities *)\n");
	for(i=1; i<=V->nsrf; ++i){
		fprintf(of,"\teps['%d'] := %0.7g;\n",i,V->emissivity[i]);
	}
	fprintf(of,"END values;\n");
	fprintf(of,"END view3d;\n");

	if(outfile){
		fclose(of);
		fprintf(stderr,"Finished writing to '%s'.\n",outfile);
	}

	return 0;
}
