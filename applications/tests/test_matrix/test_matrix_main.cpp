#include <data/matrix2d.h>
#include <iostream>
#include "../../../external/gtest-1.6.0/fused-src/gtest/gtest.h"
// MORE INFO HERE: http://code.google.com/p/googletest/wiki/AdvancedGuide
class MatrixTest : public ::testing::Test
{
protected:
    //init metadatas
    virtual void SetUp()
    {
        chdir(((String)(getXmippPath() + (String)"/resources/test")).c_str());
    	A.resize(3,3);
        A(0,0) =-0.9234482 ;
        A(0,1) =  -0.38372311   ;
        A(0,2) =0 ;

        A(1,0) =0.38372311 ;
        A(1,1) =-0.9234482;
        A(1,2) =-0 ;

        A(2,0) =0;
        A(2,1) =0;
        A(2,2) =1 ;
    }
    Matrix2D<double> A;

};



TEST_F( MatrixTest, inverse)
{
    Matrix2D<double> B(3,3);
    Matrix2D<double> auxA(3,3);
    B.initIdentity();
    EXPECT_EQ(B.inv(),B) << "MatrixTest_inverse: identity matrix failed";

    auxA(0,0) =-0.9234482 ;
    auxA(0,1) =  0.38372311   ;
    auxA(0,2) =0 ;

    auxA(1,0) =-0.38372311 ;
    auxA(1,1) =-0.9234482;
    auxA(1,2) =-0 ;

    auxA(2,0) =0;
    auxA(2,1) =0;
    auxA(2,2) =1 ;
    EXPECT_EQ(auxA,A.inv()) << "MatrixTest_inverse: random rotation  matrix failed";

    auxA(0,0) =  1.;
    auxA(0,1) =  0.;
    auxA(0,2) = -0.;

    auxA(1,0) = -0.;
    auxA(1,1) =  1.;
    auxA(1,2) =  0 ;

    auxA(2,0) = 0;
    auxA(2,1) = 0;
    auxA(2,2) = 1;
    EXPECT_EQ(auxA,auxA.inv()) << "MatrixTest_inverse: identitity with negative 0 matrix failed";

    Matrix2D<double> M(4,4), Minv;
    M(0,0)=1; M(0,1)=2; M(0,2)=3; M(0,3)=-4;
    M(1,0)=3; M(1,1)=-4; M(1,2)=5; M(1,3)=6;
    M(2,0)=5; M(2,1)=6; M(2,2)=7; M(2,3)=-8;
    M(3,0)=7; M(3,1)=-8; M(3,2)=9; M(3,3)=10;
    M.inv(Minv);

    M(0,0)=-0.437500; M(0,1)=-0.562500; M(0,2)=0.187500; M(0,3)=0.312500;
    M(1,0)=-0.500000; M(1,1)=0.625000; M(1,2)=0.250000; M(1,3)=-0.375000;
    M(2,0)=0.312500; M(2,1)=0.437500; M(2,2)=-0.062500; M(2,3)=-0.187500;
    M(3,0)=-0.375000; M(3,1)=0.500000; M(3,2)=0.125000; M(3,3)=-0.250000;
    EXPECT_EQ(M,Minv) << "MatrixTest_inverse: 4x4 matrix inverse failed";
}


TEST_F( MatrixTest, det3x3)
{
    Matrix2D<double> auxA(A);

    auxA(0,0) =1 ;
    auxA(0,1) =2;
    auxA(0,2) =3;

    auxA(1,0) =4;
    auxA(1,1) =5;
    auxA(1,2) =6;

    auxA(2,0) =7;
    auxA(2,1) =8;
    auxA(2,2) =11;
    double result = -6;
    EXPECT_DOUBLE_EQ(auxA.det3x3(),result) << "wrong det3x3";
    EXPECT_DOUBLE_EQ(auxA.det(),result) << "wrong det";

}

TEST_F( MatrixTest, solveLinearSystem)
{
	 PseudoInverseHelper pseudoInverter;
	 Matrix2D<double> &A = pseudoInverter.A;
	 Matrix1D<double> &b = pseudoInverter.b;
	 A.resizeNoCopy(4,3);
	 b.resizeNoCopy(4);

	 A(0,0) = 1;
	 A(0,1) = -2;
	 A(0,2) = -3;
	 A(1,0) = 4;
	 A(1,1) = 5;
	 A(1,2) = -6;
	 A(2,0) = -7;
	 A(2,1) = -8;
	 A(2,2) = -9;
	 A(3,0) = 10;
	 A(3,1) = -11;
	 A(3,2) = -12;

	 b(0) = 14;
	 b(1) = 32;
	 b(2) = 50;
	 b(3) = 68;

    Matrix1D<double> auxX(3), X(3);

    auxX(0) =  0.064431;
    auxX(1) = -0.183922;
    auxX(2) = -5.412896;

    solveLinearSystem(pseudoInverter, X);

    EXPECT_EQ(auxX,X) << "MatrixTest_solveLinearSystem failed";
}

TEST_F( MatrixTest, RANSAC)
{
	 WeightedLeastSquaresHelper h;
	 Matrix2D<double> &A = h.A;
	 Matrix1D<double> &b = h.b;
	 Matrix1D<double> &w = h.w;
	 A.resizeNoCopy(100,2);
	 b.resizeNoCopy(100);
	 w.resizeNoCopy(100);

	 int Nsteps=60;
	 double iNsteps=1.0/Nsteps;
	 for (int i=0; i<Nsteps; i++)
	 {
		 double x=i*iNsteps;
		 A(i,0)=x;
		 A(i,1)=1;
		 b(i)=0.5*x+1;
		 w(i)=1;
	 }
	 for (int i=Nsteps; i<VEC_XSIZE(b); i++)
	 {
		 double x=rnd_unif(0,1);
		 A(i,0)=x;
		 A(i,1)=1;
		 b(i)=rnd_unif(1,1.5);
		 w(i)=1;
	 }

    Matrix1D<double> auxX(2), X(2);

    auxX(0) =  0.5;
    auxX(1) =  1;

    ransacWeightedLeastSquares(h, X, 0.1, 10000, 0.5);

    EXPECT_NEAR(auxX(0),X(0),1e-2) << "MatrixTest_ransacWeightedLeastSquares failed";
    EXPECT_NEAR(auxX(1),X(1),1e-2) << "MatrixTest_ransacWeightedLeastSquares failed";
}

TEST_F( MatrixTest, initGaussian)
{
    Matrix2D<double> A;
    A.initGaussian(10,1);

    std::cerr << "DEBUG_ROB, A:" << A << std::endl; // is OK
    std::cerr << "DEBUG_ROB," << "ask coss I do not think th" << std::endl;
    ASSERT_TRUE( (dMij(A,5,5) - 1) < 1e-3);
    ASSERT_TRUE( (dMij(A,6,7) - 0.6065) < 1e-3);
    ASSERT_TRUE( (dMij(A,7,6) - 0.6065) < 1e-3);
    ASSERT_TRUE( (dMij(A,5,6) - 0.6065) < 1e-3);
}

GTEST_API_ int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
