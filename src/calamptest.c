#include <stdio.h>
#include <string.h>
#include "galfactsLib.h"
#include "spec.h"
#include "dtm2spec.h"
#include "jsd/jsd_futil.h"

#define MAX_ROWS 109
static inline void cnvrt_end_sint(short int *x)
{
	short int result;
	((unsigned char*)&result)[0] = ((unsigned char*)x)[1];
	((unsigned char*)&result)[1] = ((unsigned char*)x)[0];
	*x = result;
}

static inline void cnvrt_end_int(int *x)
{
	int result;
	((unsigned char*)&result)[0] = ((unsigned char*)x)[3];
	((unsigned char*)&result)[1] = ((unsigned char*)x)[2];
	((unsigned char*)&result)[2] = ((unsigned char*)x)[1];
	((unsigned char*)&result)[3] = ((unsigned char*)x)[0];
	*x = result;
}

static inline void cnvrt_end_db(double *x)
{
	double result;
	((unsigned char*)&result)[0] = ((unsigned char*)x)[7];
	((unsigned char*)&result)[1] = ((unsigned char*)x)[6];
	((unsigned char*)&result)[2] = ((unsigned char*)x)[5];
	((unsigned char*)&result)[3] = ((unsigned char*)x)[4];
	((unsigned char*)&result)[4] = ((unsigned char*)x)[3];
	((unsigned char*)&result)[5] = ((unsigned char*)x)[2];
	((unsigned char*)&result)[6] = ((unsigned char*)x)[1];
	((unsigned char*)&result)[7] = ((unsigned char*)x)[0];
	*x = result;
}

int main(int argc,char* argv[])
{
	int num_files;
	int beam;
	char *proj_code;
	char *date;
	char *datadir;
	char *filelistname;
	int band;
	int start_file;
	int config_not_written=1;


	if(argc !=7)
	{
		printf("usage: dtm2spec <proj_code> <date> <band> <beam> <datadir> <filelist>\n");
		return 0;
	}
	else
	{
		proj_code = argv[1];
		date = argv[2];
		band = atoi(argv[3]);
		beam = atoi(argv[4]);
		datadir = argv[5];
 		filelistname = argv[6];
	}

	FILE *datafile,*specfile,*cfg_file,*listfile;
	char datafilename[100+1],specfilename[40+1],cfgfilename[40+1];
	listfile = fopen(filelistname,"r");
	num_files = jsd_line_count(listfile);
	printf("Number of files to be processed:%d\n",num_files);

	//sprintf(specfilename,"%s.%s.b%1ds%1d.lths.spec",proj_code,date,beam,band);
	sprintf(specfilename,"%s.%s.b%1ds%1d.lths.cal",proj_code,date,beam,band);
	if ( (specfile = fopen(specfilename, "wb") ) == NULL )
	{
		printf("ERROR: can't open data file for writing '%s'\n", specfilename);
		return 0;
	}

	fprintf(specfile,"CalI CalU\n");
	char buf[LINELEN+1];
	long int offset = MAIN_HEADER + BINTABLE_HEADER;
	int f,g,k,l,found = FALSE;
	for(f=0;f<num_files;f++)
	{
		int naxis1,naxis2;
		GFLIB_ROW row1,row2;
		SpecPointingBlock SPBlock;
		float Aon[MAX_CHANNELS],Aoff[MAX_CHANNELS],Bon[MAX_CHANNELS],Boff[MAX_CHANNELS];
		float XXon[MAX_CHANNELS],XXoff[MAX_CHANNELS],YYon[MAX_CHANNELS],YYoff[MAX_CHANNELS];
		float Uon[MAX_CHANNELS],Uoff[MAX_CHANNELS],Von[MAX_CHANNELS],Voff[MAX_CHANNELS];
		float XYon[MAX_CHANNELS],XYoff[MAX_CHANNELS],YXon[MAX_CHANNELS],YXoff[MAX_CHANNELS];

		fscanf(listfile,"%s",datafilename);
		char datfile[128];
		sprintf(datfile,"%s/%s",datadir,datafilename);
		if ( (datafile = fopen(datfile, "r") ) == NULL )
		{
			printf("ERROR: can't open data file for reading '%s'\n", datfile);
			return 0;
		}
		printf("Opened the datafile:%s\n",datfile);

		do
		{
			fread(buf,sizeof(char),LINELEN,datafile);
			buf[LINELEN] = '\0';
			if(strncmp(buf,"NAXIS1  ",8)==0)
			{
				sscanf(&buf[10],"%d",&naxis1);
				printf("NAXIS1: %d\n",naxis1);
				found = TRUE;
			}
		}while(!found);

		found = FALSE;
		do
		{
			fread(buf,sizeof(char),LINELEN,datafile);
			buf[LINELEN] = '\0';
			if(strncmp(buf,"NAXIS2  ",8)==0)
			{
				sscanf(&buf[10],"%d",&naxis2);
				printf("NAXIS2: %d\n",naxis2);
				found = TRUE;
			}
		}while(!found);


		for(g=0;g < naxis2-1;g++)
		{
			printf("Reading row :%d\n",g+1);
			fseek(datafile,offset+g*naxis1,SEEK_SET);
			fread(&row1,sizeof(GFLIB_ROW),1,datafile);
			fread(&row2,sizeof(GFLIB_ROW),1,datafile);
			cnvrt_end_db(&row1.RA);
			cnvrt_end_db(&row2.RA);
			cnvrt_end_db(&row1.DEC);
			cnvrt_end_db(&row2.DEC);
			cnvrt_end_db(&row1.UTC);
			cnvrt_end_db(&row2.UTC);
			cnvrt_end_db(&row1.mjdxxobs);
			cnvrt_end_db(&row1.azimuth);
			cnvrt_end_db(&row1.elevatio);
			cnvrt_end_db(&row2.azimuth);
			cnvrt_end_db(&row2.elevatio);
			cnvrt_end_db(&row1.req_raj);
			cnvrt_end_db(&row1.req_decj);
			cnvrt_end_db(&row2.req_raj);
			cnvrt_end_db(&row2.req_decj);
			cnvrt_end_db(&row1.alfa_ang);

/*			if(config_not_written)
			{
				:sprintf(cfgfilename,"%s.%s.b%ds%d.lths.spec_cfg",proj_code,date,beam,band);
				if ( (cfg_file = fopen(cfgfilename, "w") ) == NULL )
				{
					printf("ERROR: can't open config file '%s'\n", cfgfilename);
					return 0;
				}

				cnvrt_end_db(&row1.tdelt);
				fprintf(cfg_file,"%f\n",row1.tdelt*1000);
				fprintf(cfg_file,"%i\n",RAW_CHANNELS);
				cnvrt_end_db(&row1.cf);
				fprintf(cfg_file,"%f\n",row1.cf/1000000);
				cnvrt_end_db(&row1.fdelt);
				fprintf(cfg_file,"%f\n",-1*row1.fdelt*RAW_CHANNELS/1000);
				fprintf(cfg_file,"1 %i 4 2 0\n",RAW_CHANNELS);
				fprintf(cfg_file,"%s\n",proj_code);
				fprintf(cfg_file,"%f\n",row1.mjdxxobs);
				fprintf(cfg_file,"AO\n");
				fprintf(cfg_file,"Integration time (ms):%f\n",row1.tdelt*1000);
				fprintf(cfg_file,"MJD: %f\n",row1.mjdxxobs);
				fprintf(cfg_file,"Center freq (MHz): %f\n",row1.cf/1000000);
				fprintf(cfg_file,"Channel band (kHz): %f\n",-1*row1.fdelt/1000);
				fprintf(cfg_file,"Number of channels/record: %d\n",RAW_CHANNELS);
				fprintf(cfg_file,"RA at start (degrees): %f\n",row1.RA);
				fprintf(cfg_file,"DEC at start (degrees): %f\n",row1.DEC);
				fprintf(cfg_file,"UTC at start (seconds): %f\n",row1.UTC);
				fprintf(cfg_file,"ALFA angle (degrees) at start: %f\n",row1.alfa_ang);
				fprintf(cfg_file,"Project ID: %s\n",proj_code);
				fclose(cfg_file);
				config_not_written = 0;
			}
*/
			for(k=0;k<DUMPS_PER_ROW;k++)
			{
				if(!beam)
				{
					SPBlock.centralBeam.raj_true_in_hours=row1.RA/15 + k*(row2.RA-row1.RA)/(DUMPS_PER_ROW)/15;
					SPBlock.centralBeam.decj_true_in_degrees=row1.DEC + k*(row2.DEC-row1.DEC)/(DUMPS_PER_ROW);
					SPBlock.centralBeam.atlantic_solar_time_now_in_sec=\
					row1.UTC + k*(row2.UTC-row1.UTC)/(DUMPS_PER_ROW);
				}
				else
				{
					SPBlock.outerBeams[beam-1].raj_true_in_hours=row1.RA/15 + k*(row2.RA-row1.RA)/(DUMPS_PER_ROW)/15;
					SPBlock.outerBeams[beam-1].decj_true_in_degrees=row1.DEC + k*(row2.DEC-row1.DEC)/(DUMPS_PER_ROW);
					SPBlock.centralBeam.atlantic_solar_time_now_in_sec=\
					row1.UTC + k*(row2.UTC-row1.UTC)/(DUMPS_PER_ROW);
				}
				cnvrt_end_sint(&row1.staton[k].fftAccum);
				cnvrt_end_sint(&row1.statoff[k].fftAccum);
				long double calI = 0.0,calU = 0.0;
				//fwrite(&SPBlock,sizeof(SpecPointingBlock),1,specfile);
				for(l=0;l<RAW_CHANNELS;l++)
				{
					//byteswap
					cnvrt_end_int(&row1.dataon[k].A[l]);
					cnvrt_end_int(&row1.dataon[k].B[l]);
					cnvrt_end_int(&row1.dataon[k].U[l]);
					cnvrt_end_int(&row1.dataon[k].V[l]);
					cnvrt_end_int(&row1.dataoff[k].A[l]);
					cnvrt_end_int(&row1.dataoff[k].B[l]);
					cnvrt_end_int(&row1.dataoff[k].U[l]);
					cnvrt_end_int(&row1.dataoff[k].V[l]);
					printf("chan %d\n",l);
					//normalize
					Aon[l] = (float)(row1.dataon[k].A[l])/row1.staton[k].fftAccum;
					Bon[l] = (float)(row1.dataon[k].B[l])/row1.staton[k].fftAccum;
					Uon[l] = (float)((int)row1.dataon[k].U[l])/row1.staton[k].fftAccum;
					Von[l] = (float)((int)row1.dataon[k].V[l])/row1.staton[k].fftAccum;
					Aoff[l] = (float)(row1.dataoff[k].A[l])/row1.statoff[k].fftAccum;
					Boff[l] = (float)(row1.dataoff[k].B[l])/row1.statoff[k].fftAccum;
					Uoff[l] = (float)((int)row1.dataoff[k].U[l])/row1.statoff[k].fftAccum;
					Voff[l] = (float)((int)row1.dataoff[k].V[l])/row1.statoff[k].fftAccum;
					calI += (Aon[l] + Bon[l] -Boff[l]  - Aoff[l])/2.0;
					calU += (Uon[l] - Uoff[l]);
					//convert to xx yy xy yx
					//XXon[l] = Aon[l]/2;
					//XXoff[l] = Aoff[l]/2;
					//YYon[l] = Bon[l]/2;
					//YYoff[l] = Boff[l]/2;
					//XYon[l] = (Uon[l]+Von[l])/2;
					//XYoff[l] = (Uoff[l]+Voff[l])/2;
					//YXon[l] = (Uon[l]-Von[l])/2;
					//YXoff[l] = (Uoff[l]-Voff[l])/2;
//					XXon[l] = Aon[l];
//					XXoff[l] = Aoff[l];
//					YYon[l] = Bon[l];
//					YYoff[l] = Boff[l];
//					XYon[l] = Uon[l];
//					XYoff[l] = Uoff[l];
//					YXon[l] = Von[l];
//					YXoff[l] = Voff[l];
					

				}//l loop for each channel
				fprintf(specfile,"%2.6f %2.6f\n",calI,calU);
				//fwrite(&XXon,sizeof(float),MAX_CHANNELS,specfile);
				//fwrite(&YYon,sizeof(float),MAX_CHANNELS,specfile);
				//fwrite(&XYon,sizeof(float),MAX_CHANNELS,specfile);
				//fwrite(&YXon,sizeof(float),MAX_CHANNELS,specfile);
				//fwrite(&XXoff,sizeof(float),MAX_CHANNELS,specfile);
				//fwrite(&YYoff,sizeof(float),MAX_CHANNELS,specfile);
				//fwrite(&XYoff,sizeof(float),MAX_CHANNELS,specfile);
				//fwrite(&YXoff,sizeof(float),MAX_CHANNELS,specfile);
			}//k loop num dumps
			cnvrt_end_db(&row2.RA);
			cnvrt_end_db(&row2.DEC);
			cnvrt_end_db(&row2.UTC);

		}//naxis2 loop g
		fclose(datafile);
	}//num files loop f
	fclose(specfile);
	return 1;
}
