#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include "fluxdata.h"
#include "stats.h"
#include "common.h"
#include "jsd/jsd_futil.h"

int multibeam;

static void print_usage(char *prog)
{
	printf("USAGE:\n"
	"%s <beam[n]> <lowchan> <highchan> <decmin> <decmax> <num_bins> <nsigma>\n\n"
	"Description of command line arguments:\n"
	"beam<n>\t\t\t - the beam string (e.g. beam0, beam1, multibeam)\n"
	"lowchan, highchan\t - the channel range to use\n"
	"decmin, decmax\t\t - the declination range in degrees\n"
	"num_bins\t\t - the number of bins\n"
	"nsigma\t\t\t - the number of sigmas to use for source clipping\n"
	"\n",prog);
	printf("\tThe program writes out per beam per stokes binned spectral data files in the directory \"dec_spectra\". The sources are rejected based \
on the sigma width entered by the user. The output file format is one row per channel, with each row \
containing the channel number then a space followed by avg value of the first declination bin and so on. \
The files are ascii and can be opened with any regular text editor. \
Rerunning the program deletes the existing data files, so those need to be moved before running \
this program again. The program first looks for a file \"Days.list\", if found it reads in the directory names \
to process from this file, else it looks for numeric directory names in the current directory and processes those \
directories.\n\ne.g.\n ./spectral_analysis multibeam 0 2048 28 37 20 2.5\n\n");
}

static void compute_clean_bins (double **binarrayX, int *countX, double *binX, int num_bins, float nsigma, int *outliercounts)
{
	int b, i;
	double mean, sigma;
	int outlier;
	double *binarray;
	int count;

	memset(outliercounts, 0, num_bins*sizeof(int));
	for (b=0; b<num_bins; b++)
	{
		//repeat until no more outliers
		do
		{
			outlier = 0;
			binarray = binarrayX[b];
			count = countX[b];
			mean = compute_mean(binarray, 0,count);
			sigma = compute_sigma(binarray, count, mean);

			//do the cleaning by setting outliers to NAN
			for (i=0; i<count; i++)
			{
				if (fabs(mean-binarray[i]) > nsigma*sigma)
				{
					binarray[i] = NAN;
					outlier = 1;
					outliercounts[b]++;
				}
			}
		} while (outlier);
		binX[b] = mean;
	}
}

static void create_dec_bins(FluxWappData * wappdata, double **binarrayI,  double **binarrayQ,  double **binarrayU,  double **binarrayV, int *counts, int beam, float decmin, float decgrain, int num_bins, int binsize)
{
	int d, r;

	memset(counts, 0, num_bins*sizeof(int));

	for (d=0; d<wappdata->numDays; d++)
	{
		FluxDayData * daydata = &wappdata->daydata[d];

		if (multibeam && d%7!=beam)
			continue; //TODO: this is a workaround for non-beam aware datastructures

		for (r=0; r<daydata->numRecords; r++)
		{
			double I = daydata->records[r].stokes.I;
			double Q = daydata->records[r].stokes.Q;
			double U = daydata->records[r].stokes.U;
			double V = daydata->records[r].stokes.V;
			double DEC = daydata->records[r].DEC;
			int bin = (int) floor((DEC - decmin) / decgrain);

			if (bin<0 || bin>=num_bins)
				continue;

			if (counts[bin] == binsize)
			{
				printf("ERROR: maximum bin size value exceeded for bin array.\n");
				exit(1);
			}

			if (isfinite(I))
			{
				double *binarray;
				binarray = binarrayI[bin];
				binarray[counts[bin]] = I;
				binarray = binarrayQ[bin];
				binarray[counts[bin]] = Q;
				binarray = binarrayU[bin];
				binarray[counts[bin]] = U;
				binarray = binarrayV[bin];
				binarray[counts[bin]] = V;
				counts[bin]++;
			}
		}
	}
}

static void analyze_channel(FluxWappData *wappdata, int beam, float dec_min, float dec_max, int num_bins, float nsigma, int chan, int binsize)
{
	int i;
	double **binarrayI;
	double **binarrayQ;
	double **binarrayU;
	double **binarrayV;
	double *cleanbinI;
	double *cleanbinQ;
	double *cleanbinU;
	double *cleanbinV;
	int * counts, *outliercounts;
	FILE * spectralfileI;
	FILE * spectralfileQ;
	FILE * spectralfileU;
	FILE * spectralfileV;
	char filename[100+1];
	float decgrain = (dec_max-dec_min)/num_bins; //actual grain (bin) size

	//allocate requisite memory for data structures
	binarrayI = (double**) malloc(num_bins * sizeof(double*));
	binarrayQ = (double**) malloc(num_bins * sizeof(double*));
	binarrayU = (double**) malloc(num_bins * sizeof(double*));
	binarrayV = (double**) malloc(num_bins * sizeof(double*));
	for (i=0; i<num_bins; i++)
	{
		binarrayI[i] = (double*) malloc(binsize * sizeof(double));
		binarrayQ[i] = (double*) malloc(binsize * sizeof(double));
		binarrayU[i] = (double*) malloc(binsize * sizeof(double));
		binarrayV[i] = (double*) malloc(binsize * sizeof(double));
	}
	counts = malloc(num_bins * sizeof(int));
	outliercounts = malloc(num_bins * sizeof(double));
	cleanbinI = malloc(num_bins * sizeof(double));
	cleanbinQ = malloc(num_bins * sizeof(double));
	cleanbinU = malloc(num_bins * sizeof(double));
	cleanbinV = malloc(num_bins * sizeof(double));

	//bin up the values in declination
	create_dec_bins(wappdata, binarrayI, binarrayQ, binarrayU, binarrayV, counts, beam, dec_min, decgrain, num_bins,binsize);
	compute_clean_bins(binarrayI, counts, cleanbinI, num_bins, nsigma, outliercounts);
	compute_clean_bins(binarrayQ, counts, cleanbinQ, num_bins, nsigma, outliercounts);
	compute_clean_bins(binarrayU, counts, cleanbinU, num_bins, nsigma, outliercounts);
	compute_clean_bins(binarrayV, counts, cleanbinV, num_bins, nsigma, outliercounts);

	//write data to file
	sprintf(filename,"dec_spectra/I_beam%d_dec_spectra.dat",beam);
	spectralfileI = fopen(filename,"a");
	sprintf(filename,"dec_spectra/Q_beam%d_dec_spectra.dat",beam);
	spectralfileQ = fopen(filename,"a");
	sprintf(filename,"dec_spectra/U_beam%d_dec_spectra.dat",beam);
	spectralfileU = fopen(filename,"a");
	sprintf(filename,"dec_spectra/V_beam%d_dec_spectra.dat",beam);
	spectralfileV = fopen(filename,"a");

	fprintf(spectralfileI,"%04i  ",chan);
	fprintf(spectralfileQ,"%04i  ",chan);
	fprintf(spectralfileU,"%04i  ",chan);
	fprintf(spectralfileV,"%04i  ",chan);

	for (i=0; i<num_bins; i++)
	{
		fprintf(spectralfileI,"%2.8f ",cleanbinI[i]);
		fprintf(spectralfileQ,"%2.8f ",cleanbinQ[i]);
		fprintf(spectralfileU,"%2.8f ",cleanbinU[i]);
		fprintf(spectralfileV,"%2.8f ",cleanbinV[i]);
	}
	fprintf(spectralfileI,"\n");
	fprintf(spectralfileQ,"\n");
	fprintf(spectralfileU,"\n");
	fprintf(spectralfileV,"\n");
	fclose(spectralfileI);
	fclose(spectralfileQ);
	fclose(spectralfileU);
	fclose(spectralfileV);

	//cleanup
	for (i=0; i<num_bins; i++)
	{
		free(binarrayI[i]);
		free(binarrayQ[i]);
		free(binarrayU[i]);
		free(binarrayV[i]);
	}
	free(binarrayI);
	free(binarrayQ);
	free(binarrayU);
	free(binarrayV);
	free(cleanbinI);
	free(cleanbinQ);
	free(cleanbinU);
	free(cleanbinV);
	free(counts);
}

int main(int argc, char * argv[])
{
	FluxWappData *wappdata;
	char ** files;
	int numDays;
	char * wapp;
	int lowchan;
	int highchan;
	float dec_min;
	float dec_max;
	int num_bins;
	float nsigma;
	int chan;
	float maxDEC,minDEC;
	int i;
	FILE * datafile;
	char filename[100+1];
	int binsize = 0;

	/* Process command line arguments */
	if (argc != 8)
	{
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}
	else
	{
		wapp = argv[1];
		lowchan = atoi(argv[2]);
		highchan = atoi(argv[3]);
		dec_min = (float) atof(argv[4]);
		dec_max = (float) atof(argv[5]);
		num_bins = (float) atoi(argv[6]);
		nsigma = (float) atof(argv[7]);
	}

	printf("\nDeclination dependent spectral analysis program\n\n");
	printf("Channels [%d,%d]\n",lowchan,highchan);
	printf("Declination range [%f,%f] degrees\n",dec_min,dec_max);
	printf("Number of declination bins: %d\n",num_bins);
	printf("Number of sigmas for source rejection: %f\n\n",nsigma);

	//remove files from the previous run
	struct dirent *d;
	DIR *dir;
	dir = opendir("dec_spectra");
	chdir("dec_spectra");
	while(d = readdir(dir))
	{
		if(d->d_name[0]  == '.')
			continue;
		remove(d->d_name);
	}
	chdir("..");
	rmdir("dec_spectra");
	mkdir("dec_spectra",S_IRWXU|S_IRWXG|S_IRWXO); //create a subdirectory for writing output files

	if (!strcmp(wapp,"multibeam"))
	{
		numDays = numDays * 7;
		multibeam = 1;
	}
	else
		multibeam = 0;

	float decgrain = (dec_max-dec_min)/num_bins;
	int beam;
	int filedes;
	if(multibeam)
	{
		for(beam = 0;beam <7;beam++)
		{
			sprintf(filename,"dec_spectra/I_beam%d_dec_spectra.dat",beam);
			datafile = fopen(filename,"w");
			fprintf(datafile,"#bin->");
			for (i=0; i<num_bins; i++)
			{
				fprintf(datafile,"%2.8f ",dec_min+i*decgrain);
			}
			fprintf(datafile,"\n");
			fclose(datafile);
			filedes = open(filename,0x666);
			fchmod(filedes,S_IRWXU|S_IRWXG|S_IRWXO);
			close(filedes);
			sprintf(filename,"dec_spectra/Q_beam%d_dec_spectra.dat",beam);
			datafile = fopen(filename,"w");
			fprintf(datafile,"#bin->");
			for (i=0; i<num_bins; i++)
			{
				fprintf(datafile,"%2.8f ",dec_min+i*decgrain);
			}
			fprintf(datafile,"\n");
			fclose(datafile);
			filedes = open(filename,0x666);
			fchmod(filedes,S_IRWXU|S_IRWXG|S_IRWXO);
			close(filedes);
			sprintf(filename,"dec_spectra/U_beam%d_dec_spectra.dat",beam);
			datafile = fopen(filename,"w");
			fprintf(datafile,"#bin->");
			for (i=0; i<num_bins; i++)
			{
				fprintf(datafile,"%2.8f ",dec_min+i*decgrain);
			}
			fprintf(datafile,"\n");
			fclose(datafile);
			filedes = open(filename,0x666);
			fchmod(filedes,S_IRWXU|S_IRWXG|S_IRWXO);
			close(filedes);
			sprintf(filename,"dec_spectra/V_beam%d_dec_spectra.dat",beam);
			datafile = fopen(filename,"w");
			fprintf(datafile,"#bin->");
			for (i=0; i<num_bins; i++)
			{
				fprintf(datafile,"%2.8f ",dec_min+i*decgrain);
			}
			fprintf(datafile,"\n");
			fclose(datafile);
			filedes = open(filename,0x666);
			fchmod(filedes,S_IRWXU|S_IRWXG|S_IRWXO);
			close(filedes);
		}
	}
	else
	{
		beam = atoi(&wapp[4]);
		sprintf(filename,"dec_spectra/I_beam%d_dec_spectra.dat",beam);
		datafile = fopen(filename,"w");
		fprintf(datafile,"#bin->");
		for (i=0; i<num_bins; i++)
		{
			fprintf(datafile,"%2.8f ",dec_min+i*decgrain);
		}
		fprintf(datafile,"\n");
		fclose(datafile);
		filedes = open(filename,0x666);
		fchmod(filedes,S_IRWXU|S_IRWXG|S_IRWXO);
		close(filedes);
		sprintf(filename,"dec_spectra/Q_beam%d_dec_spectra.dat",beam);
		datafile = fopen(filename,"w");
		fprintf(datafile,"#bin->");
		for (i=0; i<num_bins; i++)
		{
			fprintf(datafile,"%2.8f ",dec_min+i*decgrain);
		}
		fprintf(datafile,"\n");
		fclose(datafile);
		filedes = open(filename,0x666);
		fchmod(filedes,S_IRWXU|S_IRWXG|S_IRWXO);
		close(filedes);
		sprintf(filename,"dec_spectra/U_beam%d_dec_spectra.dat",beam);
		datafile = fopen(filename,"w");
		fprintf(datafile,"#bin->");
		for (i=0; i<num_bins; i++)
		{
			fprintf(datafile,"%2.8f ",dec_min+i*decgrain);
		}
		fprintf(datafile,"\n");
		fclose(datafile);
		filedes = open(filename,0x666);
		fchmod(filedes,S_IRWXU|S_IRWXG|S_IRWXO);
		close(filedes);
		sprintf(filename,"dec_spectra/V_beam%d_dec_spectra.dat",beam);
		datafile = fopen(filename,"w");
		fprintf(datafile,"#bin->");
		for (i=0; i<num_bins; i++)
		{
			fprintf(datafile,"%2.8f ",dec_min+i*decgrain);
		}
		fprintf(datafile,"\n");
		fclose(datafile);
		filedes = open(filename,0x666);
		fchmod(filedes,S_IRWXU|S_IRWXG|S_IRWXO);
		close(filedes);
	}
	sprintf(filename,"Days.list");
	datafile = fopen(filename,"r");
	if(datafile != NULL)
	{
		printf("Found file: \"Days.list\"\n");
		numDays = jsd_line_count(datafile);
		files = (char **) malloc(sizeof(char*) * numDays);
		char tempstr[15];
		for(i=0;i<numDays;i++)
		{
			fscanf(datafile,"%s",tempstr);
			files[i] = (char *) malloc(sizeof(char) * strlen(tempstr));
			strcpy(files[i],tempstr);
		}
		fclose(datafile);
	}
	else
	{
		numDays = get_date_dirs("./", &files);
		if (numDays <= 0)
		{
			printf("ERROR: could not find any date dirs\nExiting\n");
			return EXIT_FAILURE;
		}
	}

	printf("The following days will be processed:\n");
	for(i=0;i<numDays;i++)
		printf("%s,",files[i]);
	printf("\n");
	printf("Processing data...\n");
	//Determine the total number of points in dataset per beam per bin
	for(i=0;i<numDays;i++)
	{
		sprintf(filename,"%s/beam0/fluxtime%04i.dat",files[i],lowchan);
		datafile = fopen(filename,"r");
		if(datafile == NULL)
		{
			printf("ERROR: Could not open the data file %s\n",filename);
			return(EXIT_FAILURE);
		}
		binsize+= jsd_line_count(datafile);
		fclose(datafile);
	}
	printf("Approximate number of data points per bin: %d\n",binsize/num_bins);
	binsize = binsize*1.2/num_bins; //to be safe

	// allocate and initialize the wapp data days
	wappdata = fluxwappdata_alloc(wapp, files, numDays);

	for(chan=lowchan;chan<=highchan;chan++)
	{
		//read channel data files for all the days
		fluxwappdata_readchan(wappdata, chan, BASKETWEAVE);

		if(chan==lowchan)
		{
			maxDEC = -90.0;
			minDEC = 90.0;
			//determine the actual DEC range in the data and print it for users sake
			for(i=0;i<wappdata->daydata[0].numRecords;i++)
			{
				minDEC = (wappdata->daydata[0].records[i].DEC < minDEC) ? wappdata->daydata[0].records[i].DEC : minDEC;
				maxDEC = (wappdata->daydata[0].records[i].DEC > maxDEC) ? wappdata->daydata[0].records[i].DEC : maxDEC;
			}
			printf("Actual DEC range in data [%f:%f] degrees\n",minDEC,maxDEC);
		}

		printf("Channel: %i\n", chan);
		if(multibeam)
		{
			analyze_channel(wappdata, 0, dec_min, dec_max, num_bins, nsigma, chan, binsize);
			analyze_channel(wappdata, 1, dec_min, dec_max, num_bins, nsigma, chan, binsize);
			analyze_channel(wappdata, 2, dec_min, dec_max, num_bins, nsigma, chan, binsize);
			analyze_channel(wappdata, 3, dec_min, dec_max, num_bins, nsigma, chan, binsize);
			analyze_channel(wappdata, 4, dec_min, dec_max, num_bins, nsigma, chan, binsize);
			analyze_channel(wappdata, 5, dec_min, dec_max, num_bins, nsigma, chan, binsize);
			analyze_channel(wappdata, 6, dec_min, dec_max, num_bins, nsigma, chan, binsize);
		}
		else
			analyze_channel(wappdata, atoi(&wapp[4]), dec_min, dec_max, num_bins, nsigma, chan, binsize);
	}

	printf("\nProcessing complete.\n");
	printf("\nPlease move the various spectra.dat files before running the program again to save your results.\n\n");
	free(files);
	fluxwappdata_free(wappdata);
	return EXIT_SUCCESS;
}
