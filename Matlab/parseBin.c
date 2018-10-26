/*
 * parseCsv.c - example in MATLAB External Interfaces
 *
 * Multiplies an input scalar (multiplier) 
 * times a 1xN matrix (inMatrix)
 * and outputs a 1xN matrix (outMatrix)
 *
 * The calling syntax is:
 *
 *		outMatrix = arrayProduct(multiplier, inMatrix)
 *
 * This is a MEX file for MATLAB.
*/

#include "mex.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "matrix.h"
#include <inttypes.h>

#define INITIAL_LENGTH 10000

void processVal(FILE *,long ing);

/* The gateway function */
void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{

    FILE *fid;
    FILE *fop;
    long int i=0,j=0,k=0;
    char line[256];
    uint8_t buff[2];
    uint16_t val;
    
if(nrhs != 1) {
    mexErrMsgIdAndTxt("MyToolbox:arrayProduct:nrhs",
                      "One inputs required.");
}


    if(nlhs != 6) {
    mexErrMsgIdAndTxt("MyToolbox:arrayProduct:nlhs",
                      "Six outputs required.");
}


size_t buflen = mxGetN(prhs[0])*sizeof(mxChar)+1;
    char *filename = mxMalloc(buflen); 
    mxGetString(prhs[0], filename, (mwSize)buflen);
    
fid = fopen(filename, "rb");
fop = fopen(strcat(filename,".txt"), "w");

       
if (fid == NULL || fop == NULL) 
{
    mexErrMsgIdAndTxt("MyToolbox:actionNotTaken",
                      "couldn't open file.");
}

int currLengthSpike= INITIAL_LENGTH, currLengthRec= INITIAL_LENGTH, currLengthEmon = INITIAL_LENGTH;
int currSpikeChan,currEmonChan=63;

double *valSpike = (double*) mxMalloc(INITIAL_LENGTH*sizeof(double));
double *valRec = (double*) mxMalloc(INITIAL_LENGTH*sizeof(double));
double *valEmon = (double*) mxMalloc(INITIAL_LENGTH*sizeof(double));

double *chanSpike = (double*) mxMalloc(INITIAL_LENGTH*sizeof(double));
double *chanRec = (double*) mxMalloc(INITIAL_LENGTH*sizeof(double));
double *chanEmon = (double*) mxMalloc(INITIAL_LENGTH*sizeof(double));

double * tempRec;
double * tempSpike;
double * tempEmon;

if (!(valSpike && valRec && valEmon && chanSpike && chanRec && chanEmon))
{
   return;
}


fseek( fid, 0, SEEK_SET );

while (fread(buff,1,2,fid) ==2) 
{
       val = (buff[0]<<8 | buff[1]);
        
//        processVal(fop,val);
        if (val>32767)
        {
            fprintf(fop,"recorded data channel: %d, data is %d\n",(val>>10)&31,val&1023);
            
            chanRec[i] = (double)(( val>>10) &31);
            valRec[i] = (double)(val&1023);
            i++;
           // if (i>=(INITIAL_LENGTH*2)) break;
        }    
        else if (val == 25546)
        {
            fprintf(fop,"ack signal\n");
        }
        else if (val > 20479)
        {
            fprintf(fop,"spike header, channel: %d, timestamp: %d\n",(val>>4)&31,val&15);
            currSpikeChan = (double)((val>>4)&31);
        }
        else if (val >16383)
        {
            fprintf(fop,"spike sample value: %d\n",val&1023);
            chanSpike[j] = (double)currSpikeChan;
            valSpike[j] = (double)(val&1023);
            j++;
            //if (j>=(INITIAL_LENGTH)) break;
        }
        else if (val >8191)
        {
            fprintf(fop,"emonitor value: %d\n",val &1023);
            chanEmon[k] = (double)currEmonChan;
            valEmon[k] = (double)(val&1023);
            k++;
            //if (k>=(INITIAL_LENGTH)) break;
        }
        else
        {
            fprintf(fop,"response to request id: %d, data value: %d\n", (val>>8)&31,val&255);   
        }

        
        if (i>=currLengthRec)
        {
            currLengthRec += INITIAL_LENGTH;   
            
            tempRec = mxRealloc(chanRec,currLengthRec*sizeof(double));
            if (tempRec) chanRec = tempRec; 
                else return;
            
            tempRec = mxRealloc(valRec,currLengthRec*sizeof(double));
            if (tempRec ) valRec = tempRec;
                else return;
           
            
            
        }
        
        if (j>=currLengthSpike)
        {
            currLengthSpike +=INITIAL_LENGTH;
            
            tempSpike = mxRealloc(chanSpike,currLengthSpike*sizeof(double));
            if (tempSpike) chanSpike = tempSpike; 
                else return;
            
            tempSpike = mxRealloc(valSpike,currLengthSpike*sizeof(double));
            if (tempSpike) valSpike = tempSpike;
                else return;
            
        }
        
        if (k>=currLengthEmon)
        {
            currLengthEmon +=INITIAL_LENGTH;
            
            tempEmon = mxRealloc(chanEmon,currLengthEmon*sizeof(double));
            if (tempEmon) chanEmon = tempEmon;
                else return;
            
            tempEmon = mxRealloc(valEmon,currLengthEmon*sizeof(double));
            if (tempEmon) valEmon = tempEmon;
                    else return;
            
        }
        
    
}

if (ferror(fid) )
{
    
     mexErrMsgIdAndTxt("MyToolbox:actionNotTaken",
                      "ferror.");   
}

if (!feof(fid)) 
{
    
     mexErrMsgIdAndTxt("MyToolbox:actionNotTaken",
                      "feof not reached.");   
}

fclose(fid);
fclose(fop);



currLengthRec = i;
currLengthSpike = j;
currLengthEmon = k;


plhs[0] = mxCreateDoubleMatrix((currLengthRec>0), currLengthRec, mxREAL);
plhs[1] = mxCreateDoubleMatrix((currLengthRec>0), currLengthRec, mxREAL);
plhs[2] = mxCreateDoubleMatrix((currLengthSpike>0), currLengthSpike, mxREAL);
plhs[3] = mxCreateDoubleMatrix((currLengthSpike>0), currLengthSpike, mxREAL);
plhs[4] = mxCreateDoubleMatrix((currLengthEmon>0), currLengthEmon, mxREAL);
plhs[5] = mxCreateDoubleMatrix((currLengthEmon>0), currLengthEmon, mxREAL);

memcpy(mxGetPr(plhs[0]), chanRec, currLengthRec*sizeof(double));
memcpy(mxGetPr(plhs[1]), valRec, currLengthRec*sizeof(double));
memcpy(mxGetPr(plhs[2]), chanSpike, currLengthSpike*sizeof(double));
memcpy(mxGetPr(plhs[3]), valSpike, currLengthSpike*sizeof(double));
memcpy(mxGetPr(plhs[4]), chanEmon, currLengthEmon*sizeof(double));
memcpy(mxGetPr(plhs[5]), valEmon, currLengthEmon*sizeof(double));



}
