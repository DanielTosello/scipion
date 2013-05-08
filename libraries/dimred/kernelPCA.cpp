/***************************************************************************
 *
 * Authors:    Albrecht Wolf
 *
 ***************************************************************************/

#include "kernelPCA.h"

void KernelPCA::setSpecificParameters(double sigma) {
	this->sigma = sigma;
}

void KernelPCA::reduceDimensionality() {
	this->M = MAT_XSIZE(*X);						//get M dimension of X
	this->N = MAT_YSIZE(*X);						//get N dimension of X

	this->euclideanNorm();						//d=(v_1-v_2)^2
	this->gramMatrix();						    //k(v_1,v_2)=exp(-1/2*(d/sigma)^2)

	this->normMatrix();							//Z=1/det(Z)*Z

	this->getGreatestEigen();			//compute Eigen and get greates Eigen

	Y.initZeros(MAT_YSIZE(U_dR), MAT_XSIZE(U_dR));
	this->calcMapping();						//compute Mapping
}

void KernelPCA::euclideanNorm() {
	//std::cout << "Entering euclideanNorm" << std::endl;
	Z.initZeros(N, N);
	int i, j, m;
	for (j = 0; j < N; j++) {
		for (i = 0; i < N; i++) {
			for (m = 0; m < M; m++) {
				double diff = MAT_ELEM(*X,i,m)-MAT_ELEM(*X,j,m);
				MAT_ELEM(Z,i,j)+=diff*diff;
			}}}
}

void KernelPCA::gramMatrix() {
	//std::cout << "Entering gramMatrix" << std::endl;
	FOR_ALL_ELEMENTS_IN_MATRIX2D(Z)
		MAT_ELEM(Z,i,j)=exp(-MAT_ELEM(Z,i,j)/(2*sigma*sigma));

	}

void KernelPCA::normMatrix() {
	//std::cout << "Entering normMatrix" << std::endl;
	Matrix2D<double> A;	//A=sums of rows
	A.initZeros(1, N);
	double a = 0;

	int i, j;
	for (i = 0; i < N; i++) {
		for (j = 0; j < N; j++) {
			MAT_ELEM(A,0,i)+=MAT_ELEM(Z,j,i);	//sum of vector z_i (row)
		}
		MAT_ELEM(A,0,i)/=N; //sum_row/(dimension N)

		//std::cout << MAT_ELEM(A,0,i) << " " << i << std::endl;
		a+=MAT_ELEM(A,0,i);

	}
	a /= N;
	//A.write("dimred/test1_c.txt"); //write A in .txt
	//std::cout << "a= " << a << std::endl;

	for (i = 0; i < N; i++) {
		for (j = 0; j < N; j++) {
			MAT_ELEM(Z,j,i)=(MAT_ELEM(Z,j,i)-MAT_ELEM(A,0,j)-MAT_ELEM(A,0,i));
			MAT_ELEM(Z,j,i)=MAT_ELEM(Z,j,i)+a;
		}
	}
}

void KernelPCA::getGreatestEigen() {
	//std::cout << "Entering getGreatestEigen" << std::endl;
	//calc Eigen-Value/Vector
	Z.eigs(U, W, V, index);

	U_dR.initZeros(N, this->outputDim);
	W_dR.initZeros(this->outputDim);

	//search for and get i-th greates eigenvalue and associated eigenvalue
	for (size_t i = 0; i < this->outputDim; i++) {
		for (int j = 0; j < N; j++) {
			if (i == VEC_ELEM(index,j)){
			VEC_ELEM(W_dR,i)=VEC_ELEM(W,j);

			for(int n=0;n<N;n++)
			{	MAT_ELEM(U_dR,n,i)=MAT_ELEM(U,n,j);}

			j=N;
		}}}

			//correction of U[x y] = U[x -y]
	size_t NN = MAT_YSIZE(U_dR);
	for (size_t i = 0; i < NN; i++) {
		MAT_ELEM(U_dR,i,1)=-MAT_ELEM(U_dR,i,1);
	}
}

void KernelPCA::calcMapping() {
	Matrix1D<double> sqrtL(outputDim);
	for (size_t i = 0; i < this->outputDim; i++)
		VEC_ELEM(sqrtL,i)=sqrt(VEC_ELEM(W_dR,i));

	for (size_t i = 0; i < MAT_YSIZE(U_dR); i++)
	{
		MAT_ELEM(Y,i,0)=(MAT_ELEM(U_dR,i,0)*VEC_ELEM(sqrtL,0));
		MAT_ELEM(Y,i,1)=(MAT_ELEM(U_dR,i,1)*VEC_ELEM(sqrtL,1));
	}
}
