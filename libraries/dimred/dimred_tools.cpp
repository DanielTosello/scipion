/***************************************************************************
 *
 * Authors:    Carlos Oscar Sanchez Sorzano      coss@cnb.csic.es (2013)
 *
 * Unidad de  Bioinformatica of Centro Nacional de Biotecnologia , CSIC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307  USA
 *
 *  All comments concerning this program package may be sent to the
 *  e-mail address 'xmipp@cnb.csic.es'
 ***************************************************************************/

#include "dimred_tools.h"

void GenerateData::generateNewDataset(const String& method, int N, double noise)
{
	label.resizeNoCopy(N);
	X.resizeNoCopy(N,3);
	t.resizeNoCopy(N,2);
	if (noise==0)
		init_random_generator(0);
	if (method=="swiss")
	{
		for (int i=0; i<N; ++i)
		{
			// Generate t
			MAT_ELEM(t,i,0)=(3 * PI / 2) * (1 + 2 * rnd_unif()); // t
			MAT_ELEM(t,i,1)=30*rnd_unif(); // height
			double localT=MAT_ELEM(t,i,0);
			double localHeight=MAT_ELEM(t,i,1);

			// Generate X
			double s,c;
			sincos(localT,&s,&c);
			MAT_ELEM(X,i,0)=localT*c+noise*rnd_gaus();
			MAT_ELEM(X,i,1)=localHeight+noise*rnd_gaus();
			MAT_ELEM(X,i,2)=localT*s+noise*rnd_gaus();

			// Generate label
			VEC_ELEM(label,i)=(unsigned char)(round(localT/2)+round(localHeight/12))%2;
		}
	}
	else if (method=="helix")
	{
		double iN=1.0/N;
		for (int i=0; i<N; ++i)
		{
			// Generate t
			MAT_ELEM(t,i,0)=2 * PI * i*iN;
			MAT_ELEM(t,i,1)=30*rnd_unif(); // height
			double localT=MAT_ELEM(t,i,0);

			// Generate X
			double s,c;
			sincos(localT,&s,&c);
			double s8,c8;
			sincos(8*localT,&s8,&c8);
			MAT_ELEM(X,i,0)=(2 + c8)*c+noise*rnd_gaus();
			MAT_ELEM(X,i,1)=(2 + c8)*s+noise*rnd_gaus();
			MAT_ELEM(X,i,2)=s8+noise*rnd_gaus();

			// Generate label
			VEC_ELEM(label,i)=(unsigned char)(round(localT * 1.5))%2;
		}
	}
	else if (method=="twinpeaks")
	{
        int actualN=round(sqrt(N));
    	X.resizeNoCopy(actualN*actualN,3);
    	t.resizeNoCopy(actualN*actualN,2);
    	label.resizeNoCopy(actualN*actualN);
        for (int ii=0; ii<actualN; ii++)
        {
        	for (int jj=0; jj<actualN; jj++)
        	{
        		int i=ii*actualN+jj;

            	// Generate t
        		double x = 1 - 2 * rnd_unif();
        		double y = 1 - 2 * rnd_unif();
        		MAT_ELEM(t,i,0)=x;
        		MAT_ELEM(t,i,1)=y;

    			// Generate X
    			MAT_ELEM(X,i,0)=x+noise*rnd_gaus();
    			MAT_ELEM(X,i,1)=y+noise*rnd_gaus();
    			double z=10*sin(PI * x) * tanh(3 * y);
    			MAT_ELEM(X,i,2)=z+noise*rnd_gaus();

    			// Generate label
    			VEC_ELEM(label,i)=(unsigned char)(round(0.1*(x+y+z-3)))%2;
        	}
        }
	}
	else if (method=="3d_clusters")
	{
		// Create centers
		std::vector<Matrix1D<double> > centers;
		Matrix1D<double> center(3);
		const int Nclusters=5;
		for (int i=0; i<Nclusters; i++)
		{
			FOR_ALL_ELEMENTS_IN_MATRIX1D(center)
				VEC_ELEM(center,i)=10*rnd_unif();
			centers.push_back(center);
		}

		// Measure the minimum distance between centers
		Matrix1D<double> diff;
		double minDistance=1e38;
		for (int i=0; i<Nclusters-1; ++i)
			for (int j=i+1; j<Nclusters; ++j)
			{
				diff=centers[i]-centers[j];
				double distance=diff.module();
				minDistance=std::min(minDistance,distance);
			}

		// Create clusters
		t.initZeros();
		double sigma=minDistance/sqrt(12);
		for (int n=0; n<N; ++n)
		{
			int i=(Nclusters*n)/N;
			const Matrix1D<double> &center=centers[i];
			MAT_ELEM(X,n,0)=XX(center)+(rnd_unif()-0.5)*sigma+noise*rnd_gaus();
			MAT_ELEM(X,n,1)=YY(center)+(rnd_unif()-0.5)*sigma+noise*rnd_gaus();
			MAT_ELEM(X,n,2)=ZZ(center)+(rnd_unif()-0.5)*sigma+noise*rnd_gaus();
		}
	}
	else if (method=="intersect")
	{
		double iN=1.0/N;
		for (int i=0; i<N; ++i)
		{
			// Generate t
			MAT_ELEM(t,i,0)=2 * PI * i*iN;
			MAT_ELEM(t,i,1)=5*rnd_unif();
			double localT=MAT_ELEM(t,i,0);
			double height=MAT_ELEM(t,i,1);

			// Generate X
			double s,c;
			sincos(localT,&s,&c);
			MAT_ELEM(X,i,0)=c+noise*rnd_gaus();
			MAT_ELEM(X,i,1)=c*s+noise*rnd_gaus();
			MAT_ELEM(X,i,2)=height+noise*rnd_gaus();

			// Generate label
			VEC_ELEM(label,i)=(unsigned char)(round(localT *0.5)+round(height*0.5))%2;
		}
	}
	else
		REPORT_ERROR(ERR_ARG_INCORRECT,"Incorrect method passed to generate data");
}

void insertNeighbour(Matrix2D<int> &idx, Matrix2D<double> &distance, int i1, int i2, double d)
{
	int K=MAT_XSIZE(idx);
	int kInsert=K;
	for (int k=K-1; k>=0; --k)
		if (MAT_ELEM(distance,i1,k)>d)
			kInsert=k;
		else
			break;
	if (kInsert<K)
	{
		for (int kp=K-1; kp>kInsert; --kp)
		{
			int kp_1=kp-1;
			MAT_ELEM(distance,i1,kp)=MAT_ELEM(distance,i1,kp_1);
			MAT_ELEM(idx,i1,kp)=MAT_ELEM(idx,i1,kp_1);
		}
		MAT_ELEM(distance,i1,kInsert)=d;
		MAT_ELEM(idx,i1,kInsert)=i2;
	}
}

void kNearestNeighbours(const Matrix2D<double> &X, int K, Matrix2D<int> &idx, Matrix2D<double> &distance, DimRedDistance2* f)
{
	idx.initConstant(MAT_YSIZE(X),K,-1);
	distance.initConstant(MAT_YSIZE(X),K,1e38);
	for (size_t i1=0; i1<MAT_YSIZE(X)-1; ++i1)
		for (size_t i2=i1+1; i2<MAT_YSIZE(X); ++i2)
		{
			// Compute the distance between i1 and i2
			double d=0;
			if (f==NULL)
				for (int j=0; j<MAT_XSIZE(X); ++j)
				{
					double diff=MAT_ELEM(X,i1,j)-MAT_ELEM(X,i2,j);
					d+=diff*diff;
				}
			else
				d=(*f)(X,i1,i2);

			// Check if they are nearest neighbours
			insertNeighbour(idx,distance,i1,i2,d);
			insertNeighbour(idx,distance,i2,i1,d);
		}
	FOR_ALL_ELEMENTS_IN_MATRIX2D(distance)
		MAT_ELEM(distance,i,j)=sqrt(MAT_ELEM(distance,i,j));
}

void computeDistance(const Matrix2D<double> &X, Matrix2D<double> &distance, DimRedDistance2* f)
{
	distance.initZeros(MAT_YSIZE(X),MAT_YSIZE(X));
	for (size_t i1=0; i1<MAT_YSIZE(X)-1; ++i1)
		for (size_t i2=i1+1; i2<MAT_YSIZE(X); ++i2)
		{
			// Compute the distance between i1 and i2
			double d=0;
			if (f==NULL)
				for (int j=0; j<MAT_XSIZE(X); ++j)
				{
					double diff=MAT_ELEM(X,i1,j)-MAT_ELEM(X,i2,j);
					d+=diff*diff;
				}
			else
				d=(*f)(X,i1,i2);

			MAT_ELEM(distance,i2,i1)=MAT_ELEM(distance,i1,i2)=sqrt(d);
		}
}

double intrinsicDimensionalityMLE(const Matrix2D<double> &X, DimRedDistance2* f)
{
	int k1=5;
	int k2=12;
	Matrix2D<int> idx;
	Matrix2D<double> distance;
	kNearestNeighbours(X,k2,idx,distance,f);

	// Estimate d
	double dsum=0;
	for (int i=0; i<MAT_YSIZE(distance); ++i)
	{
		double dist=log(MAT_ELEM(distance,i,0));
		double S=dist;
	    for (int k=1; k<MAT_XSIZE(distance); ++k)
	    {
	    	dist=log(MAT_ELEM(distance,i,k));
	    	S+=dist;
	    	if (k>=k1)
	    	{
	    		double d=(k-1)/(S-dist*(k+1));
	    		dsum+=d;
	    	}
	    }
	}
	return -dsum/((k2-k1)*MAT_YSIZE(distance));
}

// By correlation dimension
double intrinsicDimensionalityCorrDim(const Matrix2D<double> &X, DimRedDistance2* f)
{
	int K=5;
	Matrix2D<int> idx;
	Matrix2D<double> distance;
	kNearestNeighbours(X,K,idx,distance,f);

	// Compute median and maximum
	size_t N=MAT_XSIZE(distance)*MAT_YSIZE(distance);
	std::sort(MATRIX2D_ARRAY(distance),MATRIX2D_ARRAY(distance)+N);
	double median=MATRIX2D_ARRAY(distance)[N/2];
	double maxVal=MATRIX2D_ARRAY(distance)[N-1];
	median*=median; // Because we compute dist^2 instead of dist
	maxVal*=maxVal;

	// Compute the distance of all versus all
	double countLessMedianK=0, countLessMaxK=0;
	for (size_t i1=0; i1<MAT_YSIZE(X)-1; ++i1)
		for (size_t i2=i1+1; i2<MAT_YSIZE(X); ++i2)
		{
			// Compute the distance between i1 and i2
			double d=0;
			if (f==NULL)
				for (int j=0; j<MAT_XSIZE(X); ++j)
				{
					double diff=MAT_ELEM(X,i1,j)-MAT_ELEM(X,i2,j);
					d+=diff*diff;
				}
			else
				d=(*f)(X,i1,i2);

			// Check d
			if (d<=maxVal)
			{
				countLessMaxK+=1;
				if (d<=median)
					countLessMedianK+=1;
			}
		}
	double iCombinations=2.0/(MAT_YSIZE(X)*(MAT_YSIZE(X)-1));
	double probLessMedianK=countLessMedianK*iCombinations; // Probability of a distance being less than medianK
	double probLessMaxK=countLessMaxK*iCombinations; // Probability of a distance being less than maxK
	return 2*log(probLessMaxK/probLessMedianK)/log(maxVal/median);
}

double intrinsicDimensionality(Matrix2D<double> &X, const String &method, bool normalize, DimRedDistance2* f)
{
	if (normalize)
		normalizeColumns(X);

	if (method=="MLE")
		return intrinsicDimensionalityMLE(X,f);
	else if (method=="CorrDim")
		return intrinsicDimensionalityCorrDim(X,f);
	else
		REPORT_ERROR(ERR_ARG_INCORRECT,"Unknown dimensionality estimate method");

	// I do not implement NearNbDim because it gives a wrong answer on the swiss roll
	// I do not implement PackingNumbers because it is too slow on the swiss roll and gives a wrong answer
	// I do not implement EigValue because it only provides integer dimensions
	// I do not implement GMST because it is slower than MLE and CorrDim
}

DimRedAlgorithm::DimRedAlgorithm()
{
	X=NULL;
	distance=NULL;
}

void DimRedAlgorithm::setInputData(Matrix2D<double> &X)
{
	this->X=&X;
}

void DimRedAlgorithm::setOutputDimensionality(size_t outputDim)
{
	this->outputDim=outputDim;
}

const Matrix2D<double> &DimRedAlgorithm::getReducedData()
{
	return Y;
}
