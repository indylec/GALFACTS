#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "common.h"
#include "fluxdata.h"
//#include "jsd_futil.h"
#include "balance.h"
#include "grid.h"
#include "map.h"
#include "scan.h"
#include "decdependence.h"
//#include "bw_fit.h"
int multibeam; //SSG
static void print_usage(const char * prog)
{
	printf(
	"\n"
	"\twapp<n> - the beam number to create maps for (use multibeam for a multibeam map)\n"
	"\tfcenter - the center frequency of this wapp\n"
	"\tlowchan, highchan - the channel range to use\n"
	"\tramin, ramax - the Right assension range in degrees\n"
	"\tdecmin, decmax - the Declination range in degrees\n"
	"\tcell size - pixel size of map in arc minutes\n"
	"\tpatch radius - area in pixels to calculate point spread response (recommended 8)\n"
	"\tgridtype - upscans 1 downscans 2 both 3\n"
	"\tbalance day order - order of the polynomial fit in day-to-day balancing\n"
	"\tbalance scan order - order of the polynomial fit in scan-by-scan balancing\n"
	"\tbalance loopgain - the gain of each balance iteration (should be between 0 and 1)\n"
	"\tbalance epsilon - the percent change of simasq threshold where the iteraiton will stop\n"
	"\tshowprogress - 1 to create a basket weave progress cube, 0 otherwise\n"
	"\n");
}

/*
static void create_fits_map(FluxWappData * wappdata, MapMetaData * md, float fcen, int patch, int balorder, float balgain, float balepsilon, char * title)
{
	int n1, n2;
	float *dataI, *dataQ, *dataU, *dataV; 

	n1 = (int)(md->RArange/md->cellsize) + 1;
	n2 = (int)(md->DECrange/md->cellsize) + 1;

	printf("Map size: %d x %d\n", n1, n2);
	printf("Map centre: %.2f %.2f\n", md->RAcen, md->DECcen); //iterate over 
//for each 

	printf("Cell size: %7.5f\n", md->cellsize);
	printf("Channel range: (%i, %i]\n", md->lowchan, md->highchan);
	printf("Patch radius: %i\n", patch);
	printf("Center Frequency: %g\n", fcen); 

	dataI  = (float *) malloc (n1 * n2 * sizeof (float));
	dataQ  = (float *) malloc (n1 * n2 * sizeof (float));
	dataU  = (float *) malloc (n1 * n2 * sizeof (float));
	dataV  = (float *) malloc (n1 * n2 * sizeof (float));

	//read channel data files for all the days
	printf("reading data ...\n");
	fluxwappdata_readavg(wappdata);

	//perform balancing
	printf("performing balancing ...\n");
	balance_data(wappdata, balorder, md->decmin, md->decmax, balgain, balepsilon);

	//write out the balanced data
	printf("writing balanced data to file ...\n");
	fluxwappdata_writeavg(wappdata);

	//perform gridding
	printf("performing gridding ...\n");
	grid_data(wappdata, md, dataI, dataQ, dataU, dataV, n1, n2, patch, GRID_ALL_SCANS);

	printf("writing fits files ...");
	write_fits_maps(wappdata->wapp, md, dataI, dataQ, dataU, dataV);

	//free working memory
	free(dataV);
	free(dataI);
	free(dataQ);
	free(dataU);

	printf("done!\n");

}
*/

static void create_fits_cube(FluxWappData * wappdata, char * wapp, MapMetaData * md, int day_order, int scan_order, float balgain, float balepsilon, int show_progress)
{
	float *dataI, *dataQ, *dataU, *dataV, *weight; 
	int chan;
	int numbytes;

	//BWfit balfit;

	printf("Map size: %d x %d\n", md->n1, md->n2);
	printf("DEC range (degrees): (%f %f)\n", md->decmin, md->decmax);
	printf("RA range (degrees): (%f %f)\n", md->ramin, md->ramax);
	printf("Map centre: %.2f %.2f\n", md->RAcen, md->DECcen); 
	printf("Channel range: (%i, %i]\n", md->lowchan, md->highchan);
	printf("Channel count: %i\n", md->n3);
	printf("Patch radius: %i\n", md->patch);
	printf("Beamwidths: %i\n", md->beamwidths);
	printf("Cell Size: %f degrees\n", md->cellsize);
	printf("fwhm: %f arcmin\n", md->fwhm);
	printf("Grid Type: %i\n", md->gridtype);
	printf("Center Frequency: %g\n", md->fcen); 
	printf("Start Frequency: %g\n", md->fstart);
 	printf("Balance day Order: %i\n", day_order);
 	printf("Balance scan Order: %i\n", scan_order);
 	printf("Balance Loop Gain: %f\n", balgain);
 	printf("Balance Epsilon: %f\n", balepsilon);

	numbytes = md->n1 * md->n2 * sizeof (float);
	dataI  = (float *) malloc (numbytes);
	dataQ  = (float *) malloc (numbytes);
	dataU  = (float *) malloc (numbytes);
	dataV  = (float *) malloc (numbytes);
	weight  = (float *) malloc (numbytes);
	
	if (!dataI || !dataQ || !dataU || !dataV || !weight) {
		printf("ERROR: memory allocation of %i bytes failed !\n", numbytes);
		return;
	}

	init_psf_lookup_table(65537, 9.1);
	init_psf_map(md->fwhm, md->cellsize, md->beamwidths);

	if (show_progress) 
	{
		chan = md->lowchan;
		printf("Creating a progress cube \n");
		printf("Reading data\n");
		fluxwappdata_readchan(wappdata,md->lowchan,BASKETWEAVE);
		printf("Removing the declination dependence\n");
	//	remove_dec_dependence(wappdata, md->decmin, md->decmax, 0.05, md->lowchan);
		calculate_dec_dependence(wappdata, md->decmin, md->decmax, 0.05, 1);
		printf("Determine scan lines\n");
		determine_scan_lines(wappdata, md->decmin, md->decmax);
		printf("Performing balancing\n");
		//balance_data(wappdata, md, day_order, scan_order, balgain, balepsilon, &balfit, show_progress);
	}
	else
	{

		start_fits_cubes(wapp, md);
		for (chan=md->lowchan; chan<md->highchan; chan++)
		{
			printf("Channel: %i \n", chan);

			//read channel data files for all the days
			printf("Reading data ...\n");
			//fluxwappdata_readchan(wappdata, chan, BASKETWEAVE);
			fluxwappdata_readchan(wappdata, chan, CLEAN);

			//remove declination dependence
			//printf("Removing the declination dependence ...\n");
			//remove_dec_dependence(wappdata, md->decmin, md->decmax, 0.05, chan);
			//calculate_dec_dependence(wappdata, md->decmin, md->decmax, 0.05, chan);

			//determine scan lines
			//printf("Determine scan lines ...\n");
			//determine_scan_lines(wappdata, md->decmin, md->decmax);

			//printf("Performing balancing ...\n");
			//balance_data(wappdata, md, day_order, scan_order, balgain, balepsilon, &balfit, show_progress);
			//balance_data(wappdata, md, day_order, scan_order, balgain, balepsilon, show_progress);
			//printf("Writing balanced data to file ...\n");
			//fluxwappdata_writechan(wappdata, chan);
			printf("Gridding ...\n");
			grid_data(wappdata, md, dataI, dataQ, dataU, dataV, weight);

			//write fits data
			printf("Writing fits data ...\n");
			write_fits_planes(dataI, dataQ, dataU, dataV, weight);
	
		}

		printf("Finishing fits files\n");
		finish_fits_cubes();
	}

	//free memory
	free(dataI);
	free(dataQ);
	free(dataU);
	free(dataV);

	printf("Done!\n");
}

int main(int argc, char * argv[])
{
	FluxWappData *wappdata;
	MapMetaData md;

	char ** files;
	int numDays;
	/* Inputs to this program */
	//TODO: it would be nice to make these defaults
	//at the moment the values have no effect

	char * wapp;
	int day_order;
	int scan_order;
	float balgain;
	float balepsilon;
	int showprogress;

	/* Process command line arguments */ 
	/* Convert args into more usable units were required */

	if (argc != 18) 
	{
		print_usage(argv[0]);
		return EXIT_FAILURE;
	} 
	else 
	{
		wapp = argv[1];
		md.fcen = (float) atof(argv[2]);
		md.lowchan = atoi(argv[3]);
		md.highchan = atoi(argv[4]);
		md.ramin = (float) atof(argv[5]); //convert to degrees ?
		md.ramax = (float) atof(argv[6]); //convert to degrees ?
		md.decmin = (float) atof(argv[7]);
		md.decmax = (float) atof(argv[8]);
		md.cellsize = (float) atof(argv[9]) / 60.0; //convert to degrees
		md.patch = atoi(argv[10]); //TODO: remove this
		md.beamwidths = atoi(argv[10]);
		md.gridtype = atoi(argv[11]);
		day_order = atoi(argv[12]);
		scan_order = atoi(argv[13]);
		balgain = (float) atof(argv[14]);
		balepsilon = (float) atof(argv[15]);
		showprogress = atoi(argv[16]);
		md.title = argv[17];

		md.fwhm = 2.0; //TODO: paramaterize this
		md.RAcen = (md.ramax + md.ramin)/2.0;
		md.DECcen = (md.decmax + md.decmin)/2.0;
		md.RArange =  md.ramax - md.ramin;
		md.DECrange = md.decmax - md.decmin;
		md.n1 = (int)(md.RArange/md.cellsize) + 1;
		md.n2 = (int)(md.DECrange/md.cellsize) + 1;
		md.n3 = md.highchan - md.lowchan ;
		//md.fstart = md.fcen + (md.lowchan-(MAX_CHANNELS/2-1)) * (100.00/MAX_CHANNELS);//watchout for the sign for MOCK
		md.fstart = md.fcen - (md.lowchan-(MAX_CHANNELS/2-1)) * (0.042);//watchout for the sign for MOCK needs to change for WAPP
		
	}
	numDays = get_date_dirs("./", &files);
	if (numDays <= 0) {
		printf("ERROR: could not find any date dirs\n");
		return EXIT_FAILURE;
	}
	//SG
	if (!strcmp(wapp,"multibeam"))
	{
		numDays = numDays * 7;
		multibeam = 1;
	}
	else
		multibeam = 0;
	//SG
	// allocate and initialize the wapp data days
	wappdata = fluxwappdata_alloc(wapp, files, numDays);

	printf("Creating a frequency cube\n");
	create_fits_cube(wappdata, wapp, &md, day_order, scan_order, balgain, balepsilon, showprogress);
	
	//printf("creating a avg map\n");
	//create_fits_map(wappdata, &md, fcen, patch, balorder, balgain, balepsilon, title);	
	
	free(files);
	//fluxwappdata_free(wappdata);
	//free(wappdata);

	return EXIT_SUCCESS;
}


