#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "jsd/jsd_fit.h"

static void print_usage(char *prog)
{
	printf("USAGE:\n"
	"%s <dir> <beam[n]> <fit_order> <lowchan> <highchan> <decmin> <decmax> <num_bins> <nsigma>\n\n"
	"Description of command line argume  nts:\n"
	"dir\t\t\t - the base directory with input as well as output files\n"
	"beam<n>\t\t\t - the beam string (e.g. beam0, beam1, multibeam)\n"
	"fit_order\t\t - the polynomial fit order\n"
	"lowchan, highchan\t - the channel range to use\n"
	"decmin, decmax\t\t - the declination range in degrees\n"
	"num_bins\t\t - the number of bins\n"
	"nsigma\t\t\t - the number of sigmas to use for source clipping\n"
	"\n",prog);
}

static void dec_polyfit(double *binI, int num_bins, int fit_order, float decmin, float decmax, float nsigma,FILE *polyfile)
{
	int i;
	double chisq;
	double *c;
	double min, max;
	double *binDEC;

	c = malloc((fit_order+1)* sizeof(double));
		
	binDEC = calloc(num_bins, sizeof(double));
	for (i=0; i<num_bins; i++) {
		binDEC[i] = (i*(decmax-decmin)/num_bins)+decmin;
	}

	//jsd_minmax(binDEC, num_bins, &min, &max);
	//jsd_normalize(binDEC, num_bins, min, max);
	
	jsd_poly_fit(binDEC, binI, num_bins, nsigma, c, fit_order, &chisq);
	for(i = 0;i < fit_order+1;i++)
	{
		fprintf(polyfile," %2.8f x%d ",c[i],i);
	}
	fprintf(polyfile,"\n");
	free(binDEC);
}

int main(int argc, char * argv[])
{
	char * wapp;
	int lowchan;
	int highchan;
	int multibeam;
	int num_bins;
	float nsigma;
	int fit_order;
	int chan;
	float decmax,decmin;
	int i;
	int beam;
	FILE *datafileI,*datafileQ,*datafileU,*datafileV;
	FILE *polyfileI,*polyfileQ,*polyfileU,*polyfileV;
	char filename[32+1];
	char * dir;
	double *binI,*binQ,*binU,*binV;
	/* Process command line arguments */ 
	if (argc != 10) 
	{
		print_usage(argv[0]);
		return EXIT_FAILURE;
	} 
	else 
	{
		dir =argv[1];
		wapp = argv[2];
		fit_order = atoi(argv[3]);
		lowchan = atoi(argv[4]);
		highchan = atoi(argv[5]);
		decmin = (float) atof(argv[6]);
		decmax = (float) atof(argv[7]);
		num_bins = (float) atoi(argv[8]);
		nsigma = (float) atof(argv[9]);		
	}
	
	printf("\nDeclination dependent polynomial fits program\n\n");
	printf("Channels [%d,%d]\n",lowchan,highchan);
	printf("Number of declination points: %d\n",num_bins);
	printf("Order of polynomial fit: %d\n",fit_order);
	printf("Number of sigmas for fits: %f\n\n",nsigma);
	
	if(fit_order > num_bins)
	{
		printf("ERROR: fit order greater than the number of points.\n");
		return EXIT_FAILURE;
	}

	binI = malloc(num_bins * sizeof(double));
	binQ = malloc(num_bins * sizeof(double));
	binU = malloc(num_bins * sizeof(double));
	binV = malloc(num_bins * sizeof(double));
	
	printf("Processing data\n");

	if (!strcmp(wapp,"multibeam"))
		multibeam = 1;
	else
		multibeam = 0;


	if(multibeam)
	{
		for(beam =0;beam < 7;beam++)
		{
			printf("Beam %d\n",beam);
			sprintf(filename,"%s/I_beam%d_dec_spectra.dat",dir,beam);
			datafileI = fopen(filename,"r");
			sprintf(filename,"%s/Q_beam%d_dec_spectra.dat",dir,beam);
			datafileQ = fopen(filename,"r");
			sprintf(filename,"%s/U_beam%d_dec_spectra.dat",dir,beam);
			datafileU = fopen(filename,"r");
			sprintf(filename,"%s/V_beam%d_dec_spectra.dat",dir,beam);
			datafileV = fopen(filename,"r");
			
			sprintf(filename,"%s/I_beam%d_polyfits.dat",dir,beam);
			polyfileI = fopen(filename,"w");	
			sprintf(filename,"%s/Q_beam%d_polyfits.dat",dir,beam);
			polyfileQ = fopen(filename,"w");
			sprintf(filename,"%s/U_beam%d_polyfits.dat",dir,beam);
			polyfileU = fopen(filename,"w");	
			sprintf(filename,"%s/V_beam%d_polyfits.dat",dir,beam);
			polyfileV = fopen(filename,"w");
														
			for(chan=lowchan;chan<=highchan;chan++) 
			{
				//read channel data files for all the days
				int readchan;
				fscanf(datafileI,"%d",&readchan);
				fscanf(datafileQ,"%d",&readchan);
				fscanf(datafileU,"%d",&readchan);
				fscanf(datafileV,"%d",&readchan);
				
				for(i = 0;i < num_bins;i++)
				{
					fscanf(datafileI," %lf",&binI[i]);
					fscanf(datafileQ," %lf",&binQ[i]);
					fscanf(datafileU," %lf",&binU[i]);
					fscanf(datafileV," %lf",&binV[i]);
				//	printf("%lf ",binI[i]);			
				}							
				
				if(readchan != chan)
					continue;
				
				fprintf(polyfileI,"Channel %d:",chan);	
				dec_polyfit(binI,num_bins,fit_order,decmin,decmax,nsigma,polyfileI);
				fprintf(polyfileQ,"Channel %d:",chan);					
				dec_polyfit(binQ,num_bins,fit_order,decmin,decmax,nsigma,polyfileQ);
				fprintf(polyfileU,"Channel %d:",chan);				
				dec_polyfit(binU,num_bins,fit_order,decmin,decmax,nsigma,polyfileU);
				fprintf(polyfileV,"Channel %d:",chan);				
				dec_polyfit(binV,num_bins,fit_order,decmin,decmax,nsigma,polyfileV);					
						
			}
			
			fclose(polyfileI);
			fclose(polyfileQ);
			fclose(polyfileU);
			fclose(polyfileV);
			fclose(datafileI);
			fclose(datafileQ);
			fclose(datafileU);
			fclose(datafileV);
		}
	}
	else
	{
		beam = 	atoi(&wapp[4]);
		sprintf(filename,"%s/I_beam%d_dec_spectra.dat",dir,beam);
		datafileI = fopen(filename,"r");
		sprintf(filename,"%s/Q_beam%d_dec_spectra.dat",dir,beam);
		datafileQ = fopen(filename,"r");
		sprintf(filename,"%s/U_beam%d_dec_spectra.dat",dir,beam);
		datafileU = fopen(filename,"r");
		sprintf(filename,"%s/V_beam%d_dec_spectra.dat",dir,beam);
		datafileV = fopen(filename,"r");

		sprintf(filename,"%s/I_beam%d_polyfits.dat",dir,beam);
		polyfileI = fopen(filename,"w");	
		sprintf(filename,"%s/Q_beam%d_polyfits.dat",dir,beam);
		polyfileQ = fopen(filename,"w");
		sprintf(filename,"%s/U_beam%d_polyfits.dat",dir,beam);
		polyfileU = fopen(filename,"w");	
		sprintf(filename,"%s/V_beam%d_polyfits.dat",dir,beam);
		polyfileV = fopen(filename,"w");
														
		for(chan=lowchan;chan<=highchan;chan++) 
		{
			//read channel data files for all the days
			int readchan;
			fscanf(datafileI,"%d",&readchan);
			fscanf(datafileQ,"%d",&readchan);
			fscanf(datafileU,"%d",&readchan);
			fscanf(datafileV,"%d",&readchan);
			for(i = 0;i < num_bins;i++)
			{
				fscanf(datafileI," %lf",&binI[i]);
				fscanf(datafileQ," %lf",&binQ[i]);
				fscanf(datafileU," %lf",&binU[i]);
				fscanf(datafileV," %lf",&binV[i]);
			}							
				
			if(readchan != chan)
				continue;

			fprintf(polyfileI,"Channel %d:",chan);	
			dec_polyfit(binI,num_bins,fit_order,decmin,decmax,nsigma,polyfileI);
			fprintf(polyfileQ,"Channel %d:",chan);					
			dec_polyfit(binQ,num_bins,fit_order,decmin,decmax,nsigma,polyfileQ);
			fprintf(polyfileU,"Channel %d:",chan);				
			dec_polyfit(binU,num_bins,fit_order,decmin,decmax,nsigma,polyfileU);
			fprintf(polyfileV,"Channel %d:",chan);				
			dec_polyfit(binV,num_bins,fit_order,decmin,decmax,nsigma,polyfileV);				
		}
		fclose(polyfileI);
		fclose(polyfileQ);
		fclose(polyfileU);
		fclose(polyfileV);
		fclose(datafileI);
		fclose(datafileQ);
		fclose(datafileU);
		fclose(datafileV);
	}
	
	printf("\nProcessing complete\n");
	return EXIT_SUCCESS;
}
